// Neuron Forge - ENSIM
//
// Test de fumée du niveau L_Lab, dans le jeu réel (contexte Client uniquement) :
//   UnrealEditor-Cmd.exe ProjetNeuro.uproject -game -nullrhi -nosound
//     -ExecCmds="Automation RunTests NeuronForge.Smoke; Quit" -TestExit="Automation Test Queue Empty"
//
// Il rejoue le parcours du joueur : visée d'un bouton de la console, configuration
// d'une couche Tanh, montée sur le bouton au sol, ouverture de la porte, sortie.

#include "Misc/AutomationTest.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/NFInteractorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Lab/NFConfigButton.h"
#include "Lab/NFExitTrigger.h"
#include "Lab/NFFloorButton.h"
#include "Lab/NFLabDoor.h"
#include "Lab/NFNetworkBuilder.h"
#include "Player/NFCharacter.h"
#include "Player/NFHUD.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace NFSmoke
{
	UWorld* FindGameWorld()
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE) && Context.World())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	template <typename ActorType>
	ActorType* FindActor(UWorld* World)
	{
		TActorIterator<ActorType> It(World);
		return It ? *It : nullptr;
	}

	ANFConfigButton* FindButton(UWorld* World, ENFConfigAction Action, int32 LayerIndex)
	{
		for (TActorIterator<ANFConfigButton> It(World); It; ++It)
		{
			if (It->Action == Action && It->LayerIndex == LayerIndex)
			{
				return *It;
			}
		}
		return nullptr;
	}

	/** Oriente la caméra du joueur vers l'acteur, met à jour la visée et interagit. */
	bool AimAndInteract(ANFCharacter* Character, AActor* Target)
	{
		APlayerController* PC = Cast<APlayerController>(Character->GetController());
		const FVector Eye = Character->GetFirstPersonCamera()->GetComponentLocation();
		PC->SetControlRotation((Target->GetActorLocation() - Eye).Rotation());

		// Le point de vue du joueur est mis en cache par le camera manager à chaque tick :
		// on le rafraîchit pour viser immédiatement depuis la nouvelle position
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->UpdateCamera(0.f);
		}
		Character->GetInteractor()->UpdateFocus();
		return Character->GetInteractor()->GetFocusedActor() == Target && Character->GetInteractor()->TryInteract();
	}

	void Teleport(ANFCharacter* Character, const FVector& Location)
	{
		Character->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFLabSmokeTest, "NeuronForge.Smoke.LabPlaythrough",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FNFLabSmokeTest::RunTest(const FString& Parameters)
{
	UWorld* World = NFSmoke::FindGameWorld();
	if (!TestNotNull(TEXT("Monde de jeu"), World))
	{
		return false;
	}
	TestEqual(TEXT("Niveau chargé"), World->GetMapName(), FString(TEXT("L_Lab")));

	ANFCharacter* Character = Cast<ANFCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	if (!TestNotNull(TEXT("Le joueur est un ANFCharacter"), Character))
	{
		return false;
	}
	TestNotNull(TEXT("HUD Neuron Forge"), Cast<ANFHUD>(UGameplayStatics::GetPlayerController(World, 0)->GetHUD()));

	ANFNetworkBuilder* Builder = NFSmoke::FindActor<ANFNetworkBuilder>(World);
	ANFLabDoor* Door = NFSmoke::FindActor<ANFLabDoor>(World);
	ANFFloorButton* FloorButton = NFSmoke::FindActor<ANFFloorButton>(World);
	ANFExitTrigger* Exit = NFSmoke::FindActor<ANFExitTrigger>(World);
	if (!TestNotNull(TEXT("Holotable"), Builder) || !TestNotNull(TEXT("Porte"), Door)
		|| !TestNotNull(TEXT("Bouton au sol"), FloorButton) || !TestNotNull(TEXT("Sortie"), Exit))
	{
		return false;
	}
	TestEqual(TEXT("Réseau de départ sans couche cachée"), Builder->GetNumHiddenLayers(), 0);
	TestEqual(TEXT("Porte fermée au départ"), Door->GetDoorState(), ENFDoorState::Closed);

	// 1) Devant la console : ajout d'une couche, puis activation Linear -> ReLU -> Sigmoid -> Tanh
	NFSmoke::Teleport(Character, FVector(-250.f, 380.f, 100.f));
	ANFConfigButton* AddLayer = NFSmoke::FindButton(World, ENFConfigAction::AddLayer, 0);
	ANFConfigButton* Activation = NFSmoke::FindButton(World, ENFConfigAction::NextActivation, 0);
	if (!TestNotNull(TEXT("Bouton + COUCHE"), AddLayer) || !TestNotNull(TEXT("Bouton activation couche 1"), Activation))
	{
		return false;
	}

	TestTrue(TEXT("Visée et appui sur + COUCHE"), NFSmoke::AimAndInteract(Character, AddLayer));
	TestTrue(TEXT("Le bouton d'activation est utilisable une fois la couche créée"), INFInteractable::Execute_CanInteract(Activation, Character));
	for (int32 i = 0; i < 3; ++i)
	{
		TestTrue(TEXT("Visée et appui sur le bouton d'activation"), NFSmoke::AimAndInteract(Character, Activation));
	}
	TestEqual(TEXT("Architecture configurée"), Builder->DescribeNetwork(), FString(TEXT("2 - 4 Tanh - 1 Sigmoid")));

	// 2) Sur le bouton au sol : le trigger lance l'entraînement
	NFSmoke::Teleport(Character, FloorButton->GetActorLocation() + FVector(0.f, 0.f, 100.f));

	const double StartTime = FPlatformTime::Seconds();
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Builder, FloorButton, StartTime]()
	{
		if (Builder->GetTrainingState() != ENFTrainingState::Idle)
		{
			TestTrue(TEXT("Le bouton au sol est enfoncé"), FloorButton->IsPressed());
			return true;
		}
		if (FPlatformTime::Seconds() - StartTime > 5.0)
		{
			AddError(TEXT("Le bouton au sol n'a pas lancé l'entraînement"));
			return true;
		}
		return false;
	}));

	// 3) Attente de la réussite et de l'ouverture complète de la porte
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Builder, Door, StartTime]()
	{
		if (Door->IsOpen())
		{
			TestEqual(TEXT("Entraînement réussi"), Builder->GetTrainingState(), ENFTrainingState::Succeeded);
			AddInfo(FString::Printf(TEXT("Porte ouverte après %.1f s (%d epochs)"),
				FPlatformTime::Seconds() - StartTime, Builder->GetTrainer()->GetProgress().Epoch));
			return true;
		}
		if (FPlatformTime::Seconds() - StartTime > 60.0)
		{
			AddError(FString::Printf(TEXT("Porte toujours fermée (état d'entraînement %d)"), static_cast<int32>(Builder->GetTrainingState())));
			return true;
		}
		return false;
	}));

	// 4) Passage de la porte jusqu'à la zone de sortie
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Character, Exit]()
	{
		NFSmoke::Teleport(Character, Exit->GetActorLocation());
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Exit, StartTime]()
	{
		if (Exit->IsCompleted())
		{
			AddInfo(TEXT("Niveau terminé"));
			return true;
		}
		if (FPlatformTime::Seconds() - StartTime > 70.0)
		{
			AddError(TEXT("La zone de sortie n'a pas détecté le joueur"));
			return true;
		}
		return false;
	}));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
