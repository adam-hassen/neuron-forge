// Neuron Forge - ENSIM
//
// Tests de fumée du niveau L_Lab, dans le jeu réel (contexte Client uniquement) :
//   UnrealEditor-Cmd.exe ProjetNeuro.uproject -game -nullrhi -nosound
//     -ExecCmds="Automation RunTests NeuronForge.Smoke; Quit" -TestExit="Automation Test Queue Empty"
// (Smoke.Screenshots demande un vrai rendu : -RenderOffscreen au lieu de -nullrhi.)

#include "Misc/AutomationTest.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/NFGrabberComponent.h"
#include "Interaction/NFInteractorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Lab/NFExitTrigger.h"
#include "Lab/NFFloorButton.h"
#include "Lab/NFForge.h"
#include "Lab/NFLabDoor.h"
#include "Lab/NFLayerModule.h"
#include "Lab/NFLever.h"
#include "Lab/NFNetworkBuilder.h"
#include "Lab/NFPiece.h"
#include "Lab/NFPlantPot.h"
#include "Misc/Paths.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Player/NFCharacter.h"
#include "Player/NFHUD.h"
#include "Player/NFPlayerController.h"
#include "Player/NFResearcherArmsComponent.h"
#include "Tests/AutomationCommon.h"
#include "UnrealClient.h"

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

	/** Première pièce libre (non emboîtée, non tenue) du type demandé. */
	ANFPiece* FindFreePiece(UWorld* World, ENFPieceType Type, ENFActivation Activation = ENFActivation::Tanh)
	{
		for (TActorIterator<ANFPiece> It(World); It; ++It)
		{
			ANFPiece* Piece = *It;
			if (Piece->PieceType == Type && !Piece->IsDocked() && !Piece->IsHeld()
				&& (Type != ENFPieceType::ActivationCrystal || Piece->Activation == Activation))
			{
				return Piece;
			}
		}
		return nullptr;
	}

	void Teleport(ANFCharacter* Character, const FVector& Location)
	{
		Character->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	}

	void LookAt(ANFCharacter* Character, const FVector& Target)
	{
		APlayerController* PC = Cast<APlayerController>(Character->GetController());
		const FVector Eye = Character->GetFirstPersonCamera()->GetComponentLocation();
		PC->SetControlRotation((Target - Eye).Rotation());
		// Le point de vue est mis en cache par le camera manager à chaque tick : on le rafraîchit
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->UpdateCamera(0.f);
		}
	}

	/** Vise l'acteur avec la caméra du joueur et appuie sur « Interagir » (vrai rayon d'interaction). */
	bool AimAndInteract(ANFCharacter* Character, AActor* Target)
	{
		LookAt(Character, Target->GetActorLocation());
		Character->GetInteractor()->UpdateFocus();
		return Character->GetInteractor()->GetFocusedActor() == Target && Character->GetInteractor()->TryInteract();
	}

	/** Porte la pièce tenue jusqu'à l'emplacement et la pose : elle doit s'y emboîter. */
	bool CarryAndDrop(ANFCharacter* Character, UNFSocketComponent* Socket)
	{
		ANFPiece* Piece = Character->GetGrabber()->GetHeldPiece();
		if (!Piece || !Socket)
		{
			return false;
		}
		Piece->SetActorLocation(Socket->GetComponentLocation() + FVector(0.f, 0.f, 15.f));
		Character->GetGrabber()->Drop();
		return Piece->GetSocket() == Socket;
	}

	bool GrabAndPlace(ANFCharacter* Character, ANFPiece* Piece, UNFSocketComponent* Socket)
	{
		return Piece && Socket && Character->GetGrabber()->TryGrab(Piece) && CarryAndDrop(Character, Socket);
	}

	void Capture(const FString& Name)
	{
		const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots") / (TEXT("NF_") + Name + TEXT(".png")));
		FScreenshotRequest::RequestScreenshot(File, /*bShowUI*/ true, /*bAddFilenameSuffix*/ false);
	}
}

// ---------------------------------------------------------------------------
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
	ANFPlayerController* PC = Character ? Cast<ANFPlayerController>(Character->GetController()) : nullptr;
	if (!TestNotNull(TEXT("Le joueur est un ANFCharacter"), Character) || !TestNotNull(TEXT("Contrôleur Neuron Forge"), PC))
	{
		return false;
	}
	TestNotNull(TEXT("HUD Neuron Forge"), Cast<ANFHUD>(PC->GetHUD()));
	TestTrue(TEXT("Le briefing s'affiche au lancement"), PC->IsIntroOpen());
	TestTrue(TEXT("Le jeu est en pause pendant le briefing"), World->IsPaused());
	PC->CloseOverlays();
	TestFalse(TEXT("Le jeu reprend après le briefing"), World->IsPaused());

	ANFNetworkBuilder* Builder = NFSmoke::FindActor<ANFNetworkBuilder>(World);
	ANFForge* Forge = NFSmoke::FindActor<ANFForge>(World);
	ANFLever* Lever = NFSmoke::FindActor<ANFLever>(World);
	ANFLabDoor* Door = NFSmoke::FindActor<ANFLabDoor>(World);
	ANFFloorButton* FloorButton = NFSmoke::FindActor<ANFFloorButton>(World);
	ANFExitTrigger* Exit = NFSmoke::FindActor<ANFExitTrigger>(World);
	if (!TestNotNull(TEXT("Hologramme"), Builder) || !TestNotNull(TEXT("Forge"), Forge) || !TestNotNull(TEXT("Levier"), Lever)
		|| !TestNotNull(TEXT("Porte"), Door) || !TestNotNull(TEXT("Bouton au sol"), FloorButton) || !TestNotNull(TEXT("Sortie"), Exit))
	{
		return false;
	}
	TestEqual(TEXT("Réseau de départ sans couche cachée"), Builder->GetNumHiddenLayers(), 0);
	TestEqual(TEXT("Porte fermée au départ"), Door->GetDoorState(), ENFDoorState::Closed);

	// Laisse les pièces se poser sur les établis et la caméra se mettre en place
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));

	// 1) Assemblage sur la Forge : 1 module, 5 orbes, 1 cristal ReLU
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, World, Character, Forge, Builder]()
	{
		ANFLayerModule* Module = nullptr;
		for (TActorIterator<ANFLayerModule> It(World); It; ++It)
		{
			Module = *It;
			break;
		}
		if (!TestNotNull(TEXT("Un module sur l'établi"), Module))
		{
			return true;
		}

		// On attrape le module en le visant vraiment depuis l'atelier
		NFSmoke::Teleport(Character, Module->GetActorLocation() + FVector(-170.f, 0.f, 10.f));
		TestTrue(TEXT("Viser le module et appuyer sur E l'attrape"), NFSmoke::AimAndInteract(Character, Module));
		TestTrue(TEXT("Le module est tenu"), Character->GetGrabber()->GetHeldPiece() == Module);
		TestTrue(TEXT("Le module s'insère dans la fente 1"), NFSmoke::CarryAndDrop(Character, Forge->GetBay(0)));

		for (int32 i = 0; i < 5; ++i)
		{
			ANFPiece* Orb = NFSmoke::FindFreePiece(World, ENFPieceType::NeuronOrb);
			TestTrue(FString::Printf(TEXT("Orbe %d posée autour du module"), i + 1), NFSmoke::GrabAndPlace(Character, Orb, Module->GetOrbSockets()[i]));
		}
		ANFPiece* Crystal = NFSmoke::FindFreePiece(World, ENFPieceType::ActivationCrystal, ENFActivation::ReLU);
		TestTrue(TEXT("Cristal ReLU planté sur le module"), NFSmoke::GrabAndPlace(Character, Crystal, Module->GetCrystalSocket()));

		Forge->RefreshAssembly();
		TestEqual(TEXT("L'hologramme reflète l'assemblage"), Builder->DescribeNetwork(), FString(TEXT("2 - 5 ReLU - 1 Sigmoid")));
		return true;
	}));

	// 2) Levier : un tour complet des crans revient au taux de départ (0,1)
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Character, Lever, Builder]()
	{
		NFSmoke::Teleport(Character, Lever->GetActorLocation() + FVector(-190.f, 0.f, 50.f));
		const float Before = Builder->GetLearningRate();
		TestTrue(TEXT("Viser le levier et le tirer"), NFSmoke::AimAndInteract(Character, Lever));
		TestTrue(TEXT("Le levier augmente le taux d'apprentissage"), Builder->GetLearningRate() > Before);
		for (int32 i = 0; i < Builder->LearningRateSteps.Num() - 1; ++i)
		{
			NFSmoke::AimAndInteract(Character, Lever);
		}
		TestEqual(TEXT("Un tour complet du levier revient au taux de départ"), Builder->GetLearningRate(), Before);
		return true;
	}));

	// 3) Bouton au sol : le trigger lance l'entraînement
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Character, FloorButton]()
	{
		NFSmoke::Teleport(Character, FloorButton->GetActorLocation() + FVector(0.f, 0.f, 100.f));
		return true;
	}));

	const double StartTime = FPlatformTime::Seconds();
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Builder, FloorButton, StartTime]()
	{
		if (Builder->GetTrainingState() != ENFTrainingState::Idle)
		{
			TestTrue(TEXT("Le bouton au sol est enfoncé"), FloorButton->IsPressed());
			return true;
		}
		if (FPlatformTime::Seconds() - StartTime > 8.0)
		{
			AddError(TEXT("Le bouton au sol n'a pas lancé l'entraînement"));
			return true;
		}
		return false;
	}));

	// 4) Réussite et ouverture complète de la porte
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

	// 5) Phase de test : régler l'arrosage du pot de plante en visant sa commande
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, World, Character]()
	{
		ANFPlantPot* Pot = NFSmoke::FindActor<ANFPlantPot>(World);
		ANFPlantControl* WaterControl = nullptr;
		for (TActorIterator<ANFPlantControl> It(World); It; ++It)
		{
			if (It->Setting == ENFPlantSetting::Water)
			{
				WaterControl = *It;
			}
		}
		if (!TestNotNull(TEXT("Pot de plante"), Pot) || !TestNotNull(TEXT("Commande d'arrosage"), WaterControl))
		{
			return true;
		}
		TestTrue(TEXT("Le pot est actif après la validation"), Pot->IsUnlocked());
		const int32 Before = Pot->GetValue(ENFPlantSetting::Water);
		NFSmoke::Teleport(Character, WaterControl->GetActorLocation() + FVector(-170.f, 0.f, 45.f));
		TestTrue(TEXT("Viser la commande d'arrosage et appuyer sur E"), NFSmoke::AimAndInteract(Character, WaterControl));
		TestEqual(TEXT("L'arrosage augmente d'un cran"), Pot->GetValue(ENFPlantSetting::Water), (Before + 1) % 11);
		TestTrue(TEXT("Le modèle prédit la survie de la plante"), Pot->GetPrediction() >= 0.f);
		TestTrue(TEXT("Objectif « tester une plante » atteint"), Pot->HasBeenTested());
		AddInfo(FString::Printf(TEXT("Pot : eau %d, lumière %d, survie prédite %.0f %%"), Pot->GetValue(ENFPlantSetting::Water),
			Pot->GetValue(ENFPlantSetting::Light), Pot->GetPrediction() * 100.f));
		return true;
	}));

	// 6) Sortie puis écran de fin
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Character, Exit]()
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
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, PC, StartTime]()
	{
		if (PC->IsEndScreenOpen())
		{
			AddInfo(TEXT("Écran de fin affiché"));
			PC->CloseOverlays();
			return true;
		}
		if (FPlatformTime::Seconds() - StartTime > 80.0)
		{
			AddError(TEXT("L'écran de fin ne s'est pas affiché"));
			return true;
		}
		return false;
	}));
	return true;
}

// ---------------------------------------------------------------------------
// Captures d'écran de contrôle visuel. Images écrites dans Saved/Screenshots/NF_<vue>.png
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFLabScreenshotsTest, "NeuronForge.Smoke.Screenshots",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FNFLabScreenshotsTest::RunTest(const FString& Parameters)
{
	UWorld* World = NFSmoke::FindGameWorld();
	ANFCharacter* Character = World ? Cast<ANFCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
	ANFPlayerController* PC = Character ? Cast<ANFPlayerController>(Character->GetController()) : nullptr;
	ANFNetworkBuilder* Builder = World ? NFSmoke::FindActor<ANFNetworkBuilder>(World) : nullptr;
	if (!TestNotNull(TEXT("Joueur"), Character) || !TestNotNull(TEXT("Contrôleur"), PC) || !TestNotNull(TEXT("Hologramme"), Builder))
	{
		return false;
	}

	auto Shot = [](const FString& Name) { return [Name]() { NFSmoke::Capture(Name); return true; }; };

	// Briefing : les 4 pages
	for (int32 Page : { 0, 1, 2, 3 })
	{
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([PC, Page]() { PC->CloseOverlays(); PC->ShowIntro(Page); return true; }));
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.2f));
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand(Shot(FString::Printf(TEXT("Intro%d"), Page))));
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
	}

	// Retour au repos (ambiance cyan) : reconfigurer l'holotable la repasse en Idle
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([PC, Builder]()
	{
		PC->CloseOverlays();
		Builder->SetConfiguration(Builder->GetHiddenLayers(), Builder->LearningRateIndex);
		return true;
	}));

	struct FView
	{
		const TCHAR* Name;
		FVector Location;
		FVector LookAt;
	};
	const TArray<FView> Views = {
		{ TEXT("Entree"), FVector(-2000.f, 0.f, 100.f), FVector(900.f, 0.f, 350.f) },
		{ TEXT("Mains"), FVector(-1550.f, -650.f, 100.f), FVector(-1250.f, -1050.f, 60.f) },
		{ TEXT("MainsDevant"), FVector(-1550.f, -650.f, 100.f), FVector(-500.f, -650.f, 150.f) },
		{ TEXT("Atelier"), FVector(-1550.f, -350.f, 100.f), FVector(-850.f, -1050.f, 90.f) },
		{ TEXT("Forge"), FVector(-480.f, -250.f, 100.f), FVector(0.f, 0.f, 120.f) },
		{ TEXT("Hologramme"), FVector(-250.f, 350.f, 100.f), FVector(950.f, 0.f, 420.f) },
		{ TEXT("Porte"), FVector(1500.f, -600.f, 100.f), FVector(2250.f, 0.f, 300.f) },
		{ TEXT("Serveurs"), FVector(-900.f, 500.f, 100.f), FVector(-1500.f, 1500.f, 300.f) },
		{ TEXT("Aide"), FVector(-1950.f, 700.f, 100.f), FVector(-1650.f, 700.f, 200.f) },
		{ TEXT("Carnet"), FVector(150.f, 1080.f, 100.f), FVector(750.f, 1080.f, 360.f) },
		{ TEXT("Pot"), FVector(1330.f, 520.f, 100.f), FVector(1750.f, 520.f, 170.f) },
	};
	for (const FView& View : Views)
	{
		const FVector Location = View.Location;
		const FVector Target = View.LookAt;
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Character, Location, Target]()
		{
			NFSmoke::Teleport(Character, Location);
			NFSmoke::LookAt(Character, Target);
			return true;
		}));
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand(Shot(View.Name)));
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
	}

	// Pendant l'entraînement (ralenti pour la capture) : ambiance orange, impulsions rapides
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Character, Builder]()
	{
		NFSmoke::Teleport(Character, FVector(-650.f, 300.f, 100.f));
		NFSmoke::LookAt(Character, FVector(950.f, 0.f, 420.f));
		Builder->GetTrainer()->EpochsPerSecond = 2.f;
		Builder->StartTraining();
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand(Shot(TEXT("Entrainement"))));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	// Le carnet pendant l'entraînement : la zone prédite se forme
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Character]()
	{
		NFSmoke::Teleport(Character, FVector(150.f, 1080.f, 100.f));
		NFSmoke::LookAt(Character, FVector(750.f, 1080.f, 360.f));
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand(Shot(TEXT("CarnetEntrainement"))));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(6.f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand(Shot(TEXT("CarnetFin"))));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	// Pot après la validation : une plante en bonne santé puis une plante qui fane
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([World, Character]()
	{
		if (ANFPlantPot* Pot = NFSmoke::FindActor<ANFPlantPot>(World))
		{
			Pot->SetConditions(5, 6);
		}
		NFSmoke::Teleport(Character, FVector(1330.f, 520.f, 100.f));
		NFSmoke::LookAt(Character, FVector(1750.f, 520.f, 170.f));
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand(Shot(TEXT("PotSain"))));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.3f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([World]()
	{
		if (ANFPlantPot* Pot = NFSmoke::FindActor<ANFPlantPot>(World))
		{
			Pot->SetConditions(1, 9);
		}
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand(Shot(TEXT("PotFane"))));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.3f));

	// Écran de fin
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([PC]() { PC->HandleLevelCompleted(); return true; }));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand(Shot(TEXT("Fin"))));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([PC]() { PC->CloseOverlays(); return true; }));
	return true;
}

// ---------------------------------------------------------------------------
// Capture brute de la vue du joueur, quel que soit le personnage (diagnostic) : Saved/Screenshots/NF_Vue*.png
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFViewCaptureTest, "NeuronForge.Diagnostic.ViewCapture",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FNFViewCaptureTest::RunTest(const FString& Parameters)
{
	UWorld* World = NFSmoke::FindGameWorld();
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	if (!TestNotNull(TEXT("Contrôleur"), PC))
	{
		return false;
	}
	if (ANFPlayerController* NFPC = Cast<ANFPlayerController>(PC))
	{
		NFPC->CloseOverlays();
	}
	const float Pitches[] = { 0.f, -35.f };
	for (int32 i = 0; i < 2; ++i)
	{
		const float Pitch = Pitches[i];
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([PC, Pitch]()
		{
			FRotator Rotation = PC->GetControlRotation();
			Rotation.Pitch = Pitch;
			PC->SetControlRotation(Rotation);
			return true;
		}));
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([i]() { NFSmoke::Capture(FString::Printf(TEXT("Vue%d"), i)); return true; }));
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
	}
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, PC]()
	{
		if (const ACharacter* Character = Cast<ACharacter>(PC->GetPawn()))
		{
			for (UActorComponent* Component : Character->GetComponents())
			{
				if (const USkeletalMeshComponent* Skel = Cast<USkeletalMeshComponent>(Component))
				{
					const FVector Hand = Skel->GetSocketLocation(TEXT("hand_r"));
					const FVector Eye = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
					AddInfo(FString::Printf(TEXT("%s visible=%d ownerNoSee=%d onlyOwnerSee=%d anim=%s main droite relative à l'œil : %s"),
						*Skel->GetName(), Skel->IsVisible(), Skel->bOwnerNoSee, Skel->bOnlyOwnerSee,
						Skel->GetAnimInstance() ? *Skel->GetAnimInstance()->GetClass()->GetName() : TEXT("aucune"),
						*Character->GetActorTransform().InverseTransformVector(Hand - Eye).ToString()));
				}
			}
		}
		return true;
	}));
	return true;
}

// ---------------------------------------------------------------------------
// Mains du chercheur : visibles devant la caméra, et autour de la pièce tenue
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFResearcherHandsTest, "NeuronForge.Smoke.ResearcherHands",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FNFResearcherHandsTest::RunTest(const FString& Parameters)
{
	UWorld* World = NFSmoke::FindGameWorld();
	ANFCharacter* Character = World ? Cast<ANFCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
	ANFPlayerController* PC = Character ? Cast<ANFPlayerController>(Character->GetController()) : nullptr;
	if (!TestNotNull(TEXT("Joueur"), Character) || !TestNotNull(TEXT("Contrôleur"), PC) || !TestNotNull(TEXT("Bras du chercheur"), Character->GetResearcherArms()))
	{
		return false;
	}
	PC->CloseOverlays();

	// Angle entre l'axe de la caméra et la direction de chaque main, et distance devant l'œil
	auto CheckHands = [this, Character](const TCHAR* Context)
	{
		const FVector Eye = Character->GetFirstPersonCamera()->GetComponentLocation();
		const FVector Forward = Character->GetController()->GetControlRotation().Vector();
		for (bool bRight : { false, true })
		{
			const FVector ToHand = Character->GetResearcherArms()->GetHandLocation(bRight) - Eye;
			const float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(ToHand.GetSafeNormal(), Forward)));
			AddInfo(FString::Printf(TEXT("%s, main %s : %.0f cm devant, %.0f° de l'axe de vue"), Context, bRight ? TEXT("droite") : TEXT("gauche"),
				FVector::DotProduct(ToHand, Forward), Angle));
			TestTrue(FString::Printf(TEXT("%s : main %s visible (devant, dans le champ)"), Context, bRight ? TEXT("droite") : TEXT("gauche")),
				FVector::DotProduct(ToHand, Forward) > 15.f && Angle < 40.f);
		}
	};

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Character]()
	{
		NFSmoke::Teleport(Character, FVector(-1550.f, -650.f, 100.f));
		NFSmoke::LookAt(Character, FVector(-500.f, -650.f, 150.f));
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([CheckHands]() { CheckHands(TEXT("Au repos")); NFSmoke::Capture(TEXT("MainsRepos")); return true; }));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	// Prendre un module : les mains viennent l'encadrer
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, World, Character]()
	{
		ANFLayerModule* Module = nullptr;
		for (TActorIterator<ANFLayerModule> It(World); It; ++It)
		{
			if (!It->IsDocked())
			{
				Module = *It;
				break;
			}
		}
		TestTrue(TEXT("Un module est pris en main"), Module && Character->GetGrabber()->TryGrab(Module));
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Character, CheckHands]()
	{
		CheckHands(TEXT("Pièce en main"));
		if (const AActor* Held = Character->GetGrabber()->GetHeldPiece())
		{
			for (bool bRight : { false, true })
			{
				// Les mains sont au contact de la pièce : à moins de sa demi-taille + 25 cm de son centre
				FVector Origin, Extent;
				Held->GetActorBounds(false, Origin, Extent);
				const float Distance = FVector::Dist(Character->GetResearcherArms()->GetHandLocation(bRight), Origin);
				AddInfo(FString::Printf(TEXT("Main %s à %.0f cm du centre de la pièce (demi-taille %.0f cm)"),
					bRight ? TEXT("droite") : TEXT("gauche"), Distance, Extent.Size()));
				TestTrue(TEXT("Les mains tiennent la pièce"), Distance < Extent.Size() + 25.f);
			}
		}
		NFSmoke::Capture(TEXT("MainsPiece"));
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Character]() { Character->GetGrabber()->Drop(); return true; }));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS


