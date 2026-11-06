// Neuron Forge - ENSIM
//
// Tests des acteurs du labo : configuration du réseau, boutons, bouton au sol,
// entraînement complet et ouverture de la porte.

#include "Misc/AutomationTest.h"
#include "GameFramework/DefaultPawn.h"
#include "Interaction/NFInteractable.h"
#include "Lab/NFConfigButton.h"
#include "Lab/NFFloorButton.h"
#include "Lab/NFLabDoor.h"
#include "Lab/NFNetworkBuilder.h"
#include "Tests/NFTestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace NFLabTests
{
	/** Crée une holotable avec une configuration donnée, entraînement accéléré pour les tests. */
	ANFNetworkBuilder* SpawnBuilder(UWorld* World, const TArray<FNFLayerConfig>& HiddenLayers, int32 MaxEpochs = 2000)
	{
		ANFNetworkBuilder* Builder = World->SpawnActorDeferred<ANFNetworkBuilder>(ANFNetworkBuilder::StaticClass(), FTransform::Identity);
		Builder->HiddenLayers = HiddenLayers;
		Builder->LearningRateIndex = 4;	// 0.5
		Builder->GetTrainer()->EpochsPerSecond = 6000.f;	// 100 epochs par frame à 60 fps
		Builder->GetTrainer()->MaxEpochs = MaxEpochs;
		Builder->FinishSpawning(FTransform::Identity);
		return Builder;
	}

	ANFLabDoor* SpawnDoor(UWorld* World, ANFNetworkBuilder* Builder)
	{
		const FTransform Transform(FVector(500.f, 0.f, 0.f));
		ANFLabDoor* Door = World->SpawnActorDeferred<ANFLabDoor>(ANFLabDoor::StaticClass(), Transform);
		Door->Builder = Builder;
		Door->FinishSpawning(Transform);
		return Door;
	}
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFBuilderEditingTest, "NeuronForge.Lab.BuilderEditing", NFTestUtils::Flags)

bool FNFBuilderEditingTest::RunTest(const FString& Parameters)
{
	NFTestUtils::FScopedTestWorld TestWorld(TEXT("NFBuilderEditingWorld"));
	ANFNetworkBuilder* Builder = NFLabTests::SpawnBuilder(TestWorld.World, {});

	// Départ sans couche cachée
	TestEqual(TEXT("Architecture de départ"), Builder->DescribeNetwork(), FString(TEXT("2 - 1 Sigmoid")));
	TestFalse(TEXT("Impossible de retirer une couche inexistante"), Builder->CanApplyAction(ENFConfigAction::RemoveLayer));
	TestFalse(TEXT("Impossible d'ajouter un neurone à une couche absente"), Builder->CanApplyAction(ENFConfigAction::AddNeuron, 0));
	TestFalse(TEXT("Impossible de changer l'activation d'une couche absente"), Builder->CanApplyAction(ENFConfigAction::NextActivation, 0));

	// Ajout d'une couche : modèle de couche Linear à 4 neurones
	TestTrue(TEXT("Ajout d'une couche"), Builder->ApplyAction(ENFConfigAction::AddLayer));
	TestEqual(TEXT("Nouvelle couche Linear"), Builder->DescribeNetwork(), FString(TEXT("2 - 4 Linear - 1 Sigmoid")));

	// Bornes du nombre de neurones
	for (int32 i = 0; i < 10; ++i)
	{
		Builder->ApplyAction(ENFConfigAction::AddNeuron, 0);
	}
	TestEqual(TEXT("Neurones plafonnés à 8"), Builder->GetHiddenLayers()[0].NumNeurons, 8);
	for (int32 i = 0; i < 10; ++i)
	{
		Builder->ApplyAction(ENFConfigAction::RemoveNeuron, 0);
	}
	TestEqual(TEXT("Au moins 1 neurone"), Builder->GetHiddenLayers()[0].NumNeurons, 1);

	// Cycle d'activation
	Builder->ApplyAction(ENFConfigAction::NextActivation, 0);
	TestEqual(TEXT("Linear -> ReLU"), Builder->GetHiddenLayers()[0].Activation, ENFActivation::ReLU);

	// Nombre de couches plafonné
	for (int32 i = 0; i < 5; ++i)
	{
		Builder->ApplyAction(ENFConfigAction::AddLayer);
	}
	TestEqual(TEXT("Couches plafonnées à 3"), Builder->GetNumHiddenLayers(), 3);

	// Taux d'apprentissage
	for (int32 i = 0; i < 10; ++i)
	{
		Builder->ApplyAction(ENFConfigAction::IncreaseLearningRate);
	}
	TestEqual(TEXT("Taux maximal"), Builder->GetLearningRate(), 2.f);
	for (int32 i = 0; i < 10; ++i)
	{
		Builder->ApplyAction(ENFConfigAction::DecreaseLearningRate);
	}
	TestEqual(TEXT("Taux minimal"), Builder->GetLearningRate(), 0.01f);
	TestEqual(TEXT("Le trainer reçoit le taux"), Builder->GetTrainer()->GetLearningRate(), 0.01f);

	// Configuration verrouillée pendant l'entraînement
	TestTrue(TEXT("Démarrage de l'entraînement"), Builder->StartTraining());
	TestFalse(TEXT("Configuration verrouillée pendant l'entraînement"), Builder->CanApplyAction(ENFConfigAction::RemoveLayer));
	TestFalse(TEXT("Pas de second démarrage"), Builder->StartTraining());
	Builder->GetTrainer()->StopTraining();
	TestTrue(TEXT("Configuration déverrouillée après l'arrêt"), Builder->CanApplyAction(ENFConfigAction::RemoveLayer));

	// Bouton de configuration lié à la couche 2
	Builder->ApplyAction(ENFConfigAction::RemoveLayer);
	Builder->ApplyAction(ENFConfigAction::RemoveLayer);
	TestEqual(TEXT("Une seule couche"), Builder->GetNumHiddenLayers(), 1);

	ANFConfigButton* Button = TestWorld.World->SpawnActorDeferred<ANFConfigButton>(ANFConfigButton::StaticClass(), FTransform::Identity);
	Button->Builder = Builder;
	Button->Action = ENFConfigAction::AddNeuron;
	Button->LayerIndex = 1;
	Button->FinishSpawning(FTransform::Identity);

	TestFalse(TEXT("Bouton de la couche 2 inactif sans couche 2"), INFInteractable::Execute_CanInteract(Button, nullptr));
	Builder->ApplyAction(ENFConfigAction::AddLayer);
	TestTrue(TEXT("Bouton de la couche 2 actif quand la couche existe"), INFInteractable::Execute_CanInteract(Button, nullptr));

	INFInteractable::Execute_Interact(Button, nullptr);
	TestEqual(TEXT("Le bouton a ajouté un neurone à la couche 2"), Builder->GetHiddenLayers()[1].NumNeurons, 5);
	TestFalse(TEXT("Le bouton a un texte d'aide"), INFInteractable::Execute_GetInteractionPrompt(Button).IsEmpty());
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFFloorButtonTest, "NeuronForge.Lab.FloorButtonStartsTraining", NFTestUtils::Flags)

bool FNFFloorButtonTest::RunTest(const FString& Parameters)
{
	NFTestUtils::FScopedTestWorld TestWorld(TEXT("NFFloorButtonWorld"));
	ANFNetworkBuilder* Builder = NFLabTests::SpawnBuilder(TestWorld.World, { { 4, ENFActivation::Tanh } });
	Builder->GetTrainer()->EpochsPerSecond = 1.f;	// on ne veut pas finir l'entraînement ici

	const FVector ButtonLocation(0.f, 400.f, 0.f);
	ANFFloorButton* FloorButton = TestWorld.World->SpawnActorDeferred<ANFFloorButton>(ANFFloorButton::StaticClass(), FTransform(ButtonLocation));
	FloorButton->Builder = Builder;
	FloorButton->FinishSpawning(FTransform(ButtonLocation));

	// Un pawn loin du bouton, puis déplacé dessus
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADefaultPawn* Pawn = TestWorld.World->SpawnActor<ADefaultPawn>(FVector(0.f, -2000.f, 100.f), FRotator::ZeroRotator, Params);
	TestWorld.Step();
	TestFalse(TEXT("Bouton relâché au départ"), FloorButton->IsPressed());
	TestEqual(TEXT("Pas d'entraînement au départ"), Builder->GetTrainingState(), ENFTrainingState::Idle);

	Pawn->SetActorLocation(ButtonLocation + FVector(0.f, 0.f, 45.f));
	TestWorld.Step();
	TestTrue(TEXT("Le trigger détecte le pawn"), FloorButton->IsPressed());
	TestEqual(TEXT("Le bouton au sol lance l'entraînement"), Builder->GetTrainingState(), ENFTrainingState::Training);

	Pawn->SetActorLocation(FVector(0.f, -2000.f, 100.f));
	TestWorld.Step();
	TestFalse(TEXT("Le bouton se relâche quand le pawn part"), FloorButton->IsPressed());
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFXORDoorOpensTest, "NeuronForge.Lab.XORSuccessOpensDoor", NFTestUtils::Flags)

bool FNFXORDoorOpensTest::RunTest(const FString& Parameters)
{
	NFTestUtils::FScopedTestWorld TestWorld(TEXT("NFDoorSuccessWorld"));
	ANFNetworkBuilder* Builder = NFLabTests::SpawnBuilder(TestWorld.World, { { 4, ENFActivation::Tanh } });
	ANFLabDoor* Door = NFLabTests::SpawnDoor(TestWorld.World, Builder);

	TestEqual(TEXT("Porte fermée au départ"), Door->GetDoorState(), ENFDoorState::Closed);
	TestTrue(TEXT("Démarrage de l'entraînement"), Builder->StartTraining());


	const bool bFinished = TestWorld.StepUntil([Builder]() { return !Builder->GetTrainer()->IsTraining(); }, 300);
	TestTrue(TEXT("L'entraînement se termine"), bFinished);
	TestEqual(TEXT("2 - 4 Tanh - 1 Sigmoid réussit XOR"), Builder->GetTrainingState(), ENFTrainingState::Succeeded);
	AddInfo(FString::Printf(TEXT("Réussi en %d epochs, perte %.4f"), Builder->GetTrainer()->GetProgress().Epoch, Builder->GetTrainer()->GetProgress().Loss));

	TestEqual(TEXT("La porte commence à s'ouvrir"), Door->GetDoorState(), ENFDoorState::Opening);
	const bool bOpened = TestWorld.StepUntil([Door]() { return Door->IsOpen(); }, 300);
	TestTrue(TEXT("La porte finit par être ouverte"), bOpened);
	TestEqual(TEXT("Ouverture complète"), Door->GetOpenAlpha(), 1.f, 0.01f);
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFLinearDoorClosedTest, "NeuronForge.Lab.LinearModelKeepsDoorClosed", NFTestUtils::Flags)

bool FNFLinearDoorClosedTest::RunTest(const FString& Parameters)
{
	NFTestUtils::FScopedTestWorld TestWorld(TEXT("NFDoorFailureWorld"));
	ANFNetworkBuilder* Builder = NFLabTests::SpawnBuilder(TestWorld.World, {}, /*MaxEpochs*/ 400);
	ANFLabDoor* Door = NFLabTests::SpawnDoor(TestWorld.World, Builder);

	TestTrue(TEXT("Démarrage de l'entraînement"), Builder->StartTraining());
	const bool bFinished = TestWorld.StepUntil([Builder]() { return !Builder->GetTrainer()->IsTraining(); }, 300);
	TestTrue(TEXT("L'entraînement se termine"), bFinished);
	TestEqual(TEXT("Sans couche cachée, l'entraînement échoue"), Builder->GetTrainingState(), ENFTrainingState::Failed);

	TestWorld.Step();
	TestEqual(TEXT("La porte reste fermée"), Door->GetDoorState(), ENFDoorState::Closed);
	TestEqual(TEXT("Battants en position fermée"), Door->GetOpenAlpha(), 0.f);
	TestTrue(TEXT("Le conseil suggère d'ajouter une couche"), Builder->GetHintText().Contains(TEXT("Ajoutez une couche")));

	// Après échec, la configuration est de nouveau modifiable
	TestTrue(TEXT("Configuration modifiable après échec"), Builder->ApplyAction(ENFConfigAction::AddLayer));
	TestEqual(TEXT("Retour à l'état Idle après modification"), Builder->GetTrainingState(), ENFTrainingState::Idle);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
