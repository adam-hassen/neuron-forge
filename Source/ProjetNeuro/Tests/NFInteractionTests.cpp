// Neuron Forge - ENSIM
//
// Tests du système d'interaction commun (PC aujourd'hui, VR plus tard).

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Interaction/NFInteractableActor.h"
#include "Interaction/NFInteractorComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace NFInteractionTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ClientContext
		| EAutomationTestFlags::ProductFilter;

	/** Monde de jeu temporaire, détruit à la fin du test. */
	struct FScopedTestWorld
	{
		UWorld* World = nullptr;

		FScopedTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false, TEXT("NFInteractionTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
		}

		~FScopedTestWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		/** Laisse le moteur enregistrer les nouveaux corps physiques pour les requêtes de collision. */
		void Step()
		{
			World->Tick(LEVELTICK_All, 1.f / 60.f);
		}
	};

	ANFInteractableActor* SpawnTarget(UWorld* World, const FVector& Location, UStaticMesh* CubeMesh)
	{
		ANFInteractableActor* Target = World->SpawnActor<ANFInteractableActor>(Location, FRotator::ZeroRotator);
		Target->GetMesh()->SetStaticMesh(CubeMesh);
		return Target;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFInteractorTest, "NeuronForge.Interaction.Interactor", NFInteractionTests::Flags)

bool FNFInteractorTest::RunTest(const FString& Parameters)
{
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Mesh /Engine/BasicShapes/Cube chargé"), CubeMesh))
	{
		return false;
	}

	NFInteractionTests::FScopedTestWorld TestWorld;
	UWorld* World = TestWorld.World;

	// Propriétaire placé à l'origine et regardant vers +X
	AActor* Owner = World->SpawnActor<AActor>();
	USceneComponent* OwnerRoot = NewObject<USceneComponent>(Owner, TEXT("Root"));
	Owner->SetRootComponent(OwnerRoot);
	OwnerRoot->RegisterComponent();
	Owner->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);

	UNFInteractorComponent* Interactor = NewObject<UNFInteractorComponent>(Owner, TEXT("Interactor"));
	Interactor->bAutoUpdateFocus = false;
	Interactor->InteractionDistance = 400.f;
	Interactor->RegisterComponent();

	ANFInteractableActor* FrontTarget = NFInteractionTests::SpawnTarget(World, FVector(200.f, 0.f, 0.f), CubeMesh);
	ANFInteractableActor* SideTarget = NFInteractionTests::SpawnTarget(World, FVector(0.f, 200.f, 0.f), CubeMesh);
	ANFInteractableActor* FarTarget = NFInteractionTests::SpawnTarget(World, FVector(0.f, -1000.f, 0.f), CubeMesh);
	TestWorld.Step();

	// --- Recherche par rayon ---
	TestEqual(TEXT("Le rayon vers +X touche la cible de face"),
		Interactor->FindInteractableAlong(FVector::ZeroVector, FVector::ForwardVector), static_cast<AActor*>(FrontTarget));
	TestNull(TEXT("Le rayon vers le haut ne touche rien"),
		Interactor->FindInteractableAlong(FVector::ZeroVector, FVector::UpVector));
	TestNull(TEXT("Une cible au-delà de la distance d'interaction est ignorée"),
		Interactor->FindInteractableAlong(FVector::ZeroVector, -FVector::RightVector));

	// --- Focus et surbrillance ---
	Interactor->UpdateFocus();
	TestEqual(TEXT("Le focus suit la visée du propriétaire"), Interactor->GetFocusedActor(), static_cast<AActor*>(FrontTarget));
	TestTrue(TEXT("La cible visée est en surbrillance"), FrontTarget->IsHighlighted());

	// --- Interaction ---
	TestTrue(TEXT("TryInteract réussit sur la cible visée"), Interactor->TryInteract());
	TestEqual(TEXT("La cible a reçu une interaction"), FrontTarget->GetInteractionCount(), 1);

	FrontTarget->SetInteractionEnabled(false);
	TestFalse(TEXT("TryInteract échoue sur une cible désactivée"), Interactor->TryInteract());
	TestEqual(TEXT("Aucune interaction supplémentaire"), FrontTarget->GetInteractionCount(), 1);
	TestFalse(TEXT("CanInteractWithFocus est faux sur une cible désactivée"), Interactor->CanInteractWithFocus());

	// --- Source de visée explicite (simule un contrôleur VR pointé vers +Y) ---
	USceneComponent* Pointer = NewObject<USceneComponent>(Owner, TEXT("Pointer"));
	Pointer->SetupAttachment(OwnerRoot);
	Pointer->RegisterComponent();
	Pointer->SetWorldRotation(FRotator(0.f, 90.f, 0.f));
	Interactor->SetTraceSource(Pointer);

	Interactor->UpdateFocus();
	TestEqual(TEXT("Avec une TraceSource, le focus suit le pointeur"), Interactor->GetFocusedActor(), static_cast<AActor*>(SideTarget));
	TestFalse(TEXT("L'ancienne cible perd la surbrillance"), FrontTarget->IsHighlighted());
	TestTrue(TEXT("La nouvelle cible est en surbrillance"), SideTarget->IsHighlighted());

	// --- Interaction directe (main VR qui touche l'objet) ---
	TestTrue(TEXT("InteractWith fonctionne sans rayon"), Interactor->InteractWith(FarTarget));
	TestEqual(TEXT("La cible lointaine a reçu l'interaction directe"), FarTarget->GetInteractionCount(), 1);
	TestFalse(TEXT("InteractWith(nullptr) échoue proprement"), Interactor->InteractWith(nullptr));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
