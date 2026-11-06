// Neuron Forge - ENSIM
//
// Tests du puzzle de la Forge : emboîtement des pièces, lecture de l'assemblage,
// prise en main et verrouillage pendant l'entraînement.

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Interaction/NFGrabberComponent.h"
#include "Interaction/NFInteractable.h"
#include "Lab/NFForge.h"
#include "Lab/NFLayerModule.h"
#include "Lab/NFNetworkBuilder.h"
#include "Lab/NFPiece.h"
#include "Tests/NFTestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace NFForgeTests
{
	ANFPiece* SpawnPiece(UWorld* World, ENFPieceType Type, const FVector& Location, ENFActivation Activation = ENFActivation::Tanh)
	{
		static UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		const FTransform Transform(Location);
		ANFPiece* Piece = World->SpawnActorDeferred<ANFPiece>(ANFPiece::StaticClass(), Transform);
		Piece->PieceType = Type;
		Piece->Activation = Activation;
		Piece->GetMesh()->SetStaticMesh(Sphere);
		Piece->SetActorScale3D(FVector(0.15f));
		Piece->FinishSpawning(Transform);
		return Piece;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFForgeAssemblyTest, "NeuronForge.Lab.ForgeAssembly", NFTestUtils::Flags)

bool FNFForgeAssemblyTest::RunTest(const FString& Parameters)
{
	NFTestUtils::FScopedTestWorld TestWorld(TEXT("NFForgeWorld"));
	UWorld* World = TestWorld.World;

	ANFNetworkBuilder* Builder = World->SpawnActor<ANFNetworkBuilder>(FVector(1000.f, 0.f, 0.f), FRotator::ZeroRotator);
	ANFForge* Forge = World->SpawnActorDeferred<ANFForge>(ANFForge::StaticClass(), FTransform::Identity);
	Forge->Builder = Builder;
	Forge->FinishSpawning(FTransform::Identity);

	ANFLayerModule* Module = World->SpawnActor<ANFLayerModule>(FVector(0.f, 2000.f, 300.f), FRotator::ZeroRotator);
	TestNotNull(TEXT("Module de couche"), Module);
	TestEqual(TEXT("Un module a 8 emplacements d'orbes"), Module->GetOrbSockets().Num(), ANFLayerModule::MaxNeurons);

	// Forge vide : réseau sans couche cachée
	Forge->RefreshAssembly();
	TestEqual(TEXT("Forge vide"), Builder->DescribeNetwork(), FString(TEXT("2 - 1 Sigmoid")));

	// Module inséré sans orbe : ignoré
	Module->DockTo(Forge->GetBay(0));
	Forge->RefreshAssembly();
	TestEqual(TEXT("Le module est dans la fente 1"), Forge->GetModuleInBay(0), Module);
	TestEqual(TEXT("Un module sans orbe est ignoré"), Builder->GetNumHiddenLayers(), 0);

	// 4 orbes + cristal Tanh → 2 - 4 Tanh - 1
	TArray<ANFPiece*> Orbs;
	for (int32 i = 0; i < 4; ++i)
	{
		ANFPiece* Orb = NFForgeTests::SpawnPiece(World, ENFPieceType::NeuronOrb, FVector(100.f * i, 3000.f, 300.f));
		Orb->DockTo(Module->GetOrbSockets()[i]);
		Orbs.Add(Orb);
	}
	Forge->RefreshAssembly();
	TestEqual(TEXT("4 orbes sans cristal : couche Linear"), Builder->DescribeNetwork(), FString(TEXT("2 - 4 Linear - 1 Sigmoid")));

	ANFPiece* Crystal = NFForgeTests::SpawnPiece(World, ENFPieceType::ActivationCrystal, FVector(0.f, 4000.f, 300.f), ENFActivation::Tanh);
	Crystal->DockTo(Module->GetCrystalSocket());
	Forge->RefreshAssembly();
	TestEqual(TEXT("Cristal Tanh planté"), Builder->DescribeNetwork(), FString(TEXT("2 - 4 Tanh - 1 Sigmoid")));

	// Emboîtement par proximité : une orbe relâchée près du module rejoint un emplacement libre
	ANFPiece* Extra = NFForgeTests::SpawnPiece(World, ENFPieceType::NeuronOrb, FVector(0.f, 5000.f, 300.f));
	Extra->SetActorLocation(Module->GetOrbSockets()[5]->GetComponentLocation() + FVector(0.f, 0.f, 20.f));
	TestNotNull(TEXT("Un emplacement est à portée"), Extra->FindBestSocket());
	Extra->OnReleased();
	TestTrue(TEXT("L'orbe relâchée près du module s'emboîte"), Extra->IsDocked());
	Forge->RefreshAssembly();
	TestEqual(TEXT("5 neurones"), Builder->GetHiddenLayers()[0].NumNeurons, 5);

	// Une orbe ne s'emboîte pas dans une fente de module, ni un cristal sur une orbe
	ANFPiece* Wrong = NFForgeTests::SpawnPiece(World, ENFPieceType::NeuronOrb, Forge->GetBay(1)->GetComponentLocation());
	TestNull(TEXT("Une orbe n'entre pas dans une fente de module"), Wrong->FindBestSocket());

	// Prise en main : retirer une orbe du module
	AActor* Player = World->SpawnActor<AActor>();
	UNFGrabberComponent* Grabber = NewObject<UNFGrabberComponent>(Player, TEXT("Grabber"));
	Grabber->RegisterComponent();
	TestTrue(TEXT("Attraper une orbe emboîtée"), Grabber->TryGrab(Orbs[0]));
	TestTrue(TEXT("L'orbe est tenue"), Orbs[0]->IsHeld() && !Orbs[0]->IsDocked());
	TestFalse(TEXT("Une seule pièce à la fois"), Grabber->TryGrab(Orbs[1]));
	Forge->RefreshAssembly();
	TestEqual(TEXT("Retirer une orbe retire un neurone"), Builder->GetHiddenLayers()[0].NumNeurons, 4);

	// On la repose loin de tout : elle redevient libre
	Orbs[0]->SetActorLocation(FVector(0.f, 8000.f, 300.f));
	Grabber->Drop();
	TestFalse(TEXT("Posée loin d'un emplacement, l'orbe reste libre"), Orbs[0]->IsDocked());

	// Pendant l'entraînement, l'assemblage est verrouillé
	TestTrue(TEXT("Démarrage de l'entraînement"), Builder->StartTraining());
	TestFalse(TEXT("Cristal verrouillé pendant l'entraînement"), INFInteractable::Execute_CanInteract(Crystal, Player));
	TestTrue(TEXT("Une pièce libre reste manipulable"), INFInteractable::Execute_CanInteract(Orbs[0], Player));
	TestFalse(TEXT("Impossible d'attraper une pièce verrouillée"), Grabber->TryGrab(Crystal));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
