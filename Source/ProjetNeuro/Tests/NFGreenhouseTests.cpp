// Neuron Forge - ENSIM
//
// Tests du dataset de la serre et mesures de calibrage (précision cible, epochs).

#include "Misc/AutomationTest.h"
#include "NeuralNetwork/NFDataset.h"
#include "NeuralNetwork/NFNeuralNetwork.h"
#include "Lab/NFLabDoor.h"
#include "Lab/NFLabNotebook.h"
#include "Lab/NFPlantPot.h"
#include "Lab/NFGreenhouse.h"
#include "Lab/NFGreenhousePlant.h"
#include "Interaction/NFInteractable.h"
#include "Lab/NFNetworkBuilder.h"
#include "Tests/NFTestWorld.h"
#include "Training/NFTrainerComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace NFGreenhouseTests
{
	struct FRunResult
	{
		int32 EpochsTo90 = -1;
		int32 EpochsTo95 = -1;
		float FinalAccuracy = 0.f;
		float FinalLoss = 0.f;
		float BestAccuracy = 0.f;
	};

	/** Même procédure que UNFTrainerComponent : poids (graine 42), mélange (graine 43), SGD exemple par exemple. */
	FRunResult Train(const FNFDataset& Dataset, const TArray<FNFLayerConfig>& Hidden, float LearningRate, int32 MaxEpochs)
	{
		FNFNeuralNetwork Network;
		Network.Build(Dataset.NumInputs, Hidden, Dataset.NumOutputs, ENFActivation::Sigmoid, 42);
		FRandomStream Rng(43);

		FRunResult Result;
		for (int32 Epoch = 1; Epoch <= MaxEpochs; ++Epoch)
		{
			Network.TrainEpoch(Dataset, LearningRate, Rng);
			const FNFEvaluation Evaluation = Network.Evaluate(Dataset);
			Result.FinalAccuracy = Evaluation.Accuracy;
			Result.FinalLoss = Evaluation.Loss;
			Result.BestAccuracy = FMath::Max(Result.BestAccuracy, Evaluation.Accuracy);
			if (Result.EpochsTo90 < 0 && Evaluation.Accuracy >= 0.90f)
			{
				Result.EpochsTo90 = Epoch;
			}
			if (Result.EpochsTo95 < 0 && Evaluation.Accuracy >= 0.95f)
			{
				Result.EpochsTo95 = Epoch;
			}
		}
		return Result;
	}
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFGreenhouseDatasetTest, "NeuronForge.Greenhouse.Dataset", NFTestUtils::Flags)

bool FNFGreenhouseDatasetTest::RunTest(const FString& Parameters)
{
	const FNFDataset Data = FNFDataset::MakeGreenhouse();
	const FNFDataset Again = FNFDataset::MakeGreenhouse();

	TestTrue(TEXT("Dataset valide"), Data.IsValid());
	TestEqual(TEXT("160 expériences"), Data.Num(), 160);
	TestEqual(TEXT("Même graine => mêmes expériences"), Data.RawInputs, Again.RawInputs);
	TestEqual(TEXT("Entrées nommées"), Data.InputNames, TArray<FString>{ TEXT("Eau"), TEXT("Lumière") });
	TestEqual(TEXT("Sortie nommée"), Data.OutputName, FString(TEXT("Survie")));

	float MinRaw = 100.f, MaxRaw = -100.f, MinNorm = 100.f, MaxNorm = -100.f;
	for (int32 k = 0; k < Data.RawInputs.Num(); ++k)
	{
		MinRaw = FMath::Min(MinRaw, Data.RawInputs[k]);
		MaxRaw = FMath::Max(MaxRaw, Data.RawInputs[k]);
		MinNorm = FMath::Min(MinNorm, Data.Inputs[k]);
		MaxNorm = FMath::Max(MaxNorm, Data.Inputs[k]);
	}
	TestTrue(TEXT("Valeurs brutes entre 0 et 10"), MinRaw >= 0.f && MaxRaw <= 10.f);
	TestTrue(TEXT("Entrées normalisées entre -1 et 1"), MinNorm >= -1.f && MaxNorm <= 1.f);

	// Expériences mesurées qui contredisent la règle : c'est le bruit de mesure
	int32 Noisy = 0;
	for (int32 s = 0; s < Data.Num(); ++s)
	{
		const float* Raw = Data.GetRawInput(s);
		Noisy += (FNFDataset::GreenhouseSurvives(Raw[0], Raw[1]) ? 1.f : 0.f) != Data.GetTarget(s)[0] ? 1 : 0;
	}
	const float PositiveRate = Data.GetPositiveRate();
	AddInfo(FString::Printf(TEXT("Serre : %d expériences, %.0f %% de survies, %d points trompeurs (bruit de mesure)"),
		Data.Num(), PositiveRate * 100.f, Noisy));
	TestTrue(TEXT("Classes assez équilibrées (25 à 50 % de survies)"), PositiveRate >= 0.25f && PositiveRate <= 0.5f);
	TestTrue(TEXT("Un peu de bruit, mais pas trop (moins de 10 %)"), Noisy > 0 && Noisy < Data.Num() / 10);

	TestTrue(TEXT("Centre de la zone : survie"), FNFDataset::GreenhouseSurvives(5.f, 5.5f));
	TestFalse(TEXT("Sans eau : la plante meurt"), FNFDataset::GreenhouseSurvives(0.f, 5.5f));
	TestFalse(TEXT("Trop de lumière : la plante meurt"), FNFDataset::GreenhouseSurvives(5.f, 10.f));
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFGreenhouseCalibrationTest, "NeuronForge.Greenhouse.Calibration", NFTestUtils::Flags)

bool FNFGreenhouseCalibrationTest::RunTest(const FString& Parameters)
{
	const FNFDataset Data = FNFDataset::MakeGreenhouse();
	constexpr int32 MaxEpochs = 1500;

	struct FCase
	{
		const TCHAR* Name;
		TArray<FNFLayerConfig> Hidden;
		float LearningRate;
	};
	const TArray<FCase> Cases = {
		{ TEXT("sans couche cachée"), {}, 0.5f },
		{ TEXT("8 Linear"), { { 8, ENFActivation::Linear } }, 0.5f },
		{ TEXT("8 Linear + 8 Linear"), { { 8, ENFActivation::Linear }, { 8, ENFActivation::Linear } }, 0.1f },
		{ TEXT("2 Tanh"), { { 2, ENFActivation::Tanh } }, 0.5f },
		{ TEXT("3 Tanh"), { { 3, ENFActivation::Tanh } }, 0.5f },
		{ TEXT("4 Tanh"), { { 4, ENFActivation::Tanh } }, 0.5f },
		{ TEXT("5 Tanh"), { { 5, ENFActivation::Tanh } }, 0.5f },
		{ TEXT("6 Tanh"), { { 6, ENFActivation::Tanh } }, 0.5f },
		{ TEXT("8 Tanh"), { { 8, ENFActivation::Tanh } }, 0.5f },
		{ TEXT("4 ReLU"), { { 4, ENFActivation::ReLU } }, 0.1f },
		{ TEXT("5 ReLU"), { { 5, ENFActivation::ReLU } }, 0.1f },
		{ TEXT("6 ReLU"), { { 6, ENFActivation::ReLU } }, 0.1f },
		{ TEXT("6 ReLU lr 0.3"), { { 6, ENFActivation::ReLU } }, 0.3f },
		{ TEXT("6 Sigmoid"), { { 6, ENFActivation::Sigmoid } }, 1.f },
		{ TEXT("5 Tanh lr 0.1"), { { 5, ENFActivation::Tanh } }, 0.1f },
		{ TEXT("5 Tanh lr 0.05"), { { 5, ENFActivation::Tanh } }, 0.05f },
		{ TEXT("5 Tanh lr 1.0"), { { 5, ENFActivation::Tanh } }, 1.f },
		{ TEXT("5 Tanh lr 2.0"), { { 5, ENFActivation::Tanh } }, 2.f },
		{ TEXT("5 Tanh lr 0.3"), { { 5, ENFActivation::Tanh } }, 0.3f },
		{ TEXT("4 Tanh + 4 Tanh"), { { 4, ENFActivation::Tanh }, { 4, ENFActivation::Tanh } }, 0.3f },
		{ TEXT("4 Tanh lr 0.1"), { { 4, ENFActivation::Tanh } }, 0.1f },
		{ TEXT("6 Tanh lr 0.1"), { { 6, ENFActivation::Tanh } }, 0.1f },
		{ TEXT("3 Tanh lr 0.1"), { { 3, ENFActivation::Tanh } }, 0.1f },
		{ TEXT("5 Tanh lr 0.01"), { { 5, ENFActivation::Tanh } }, 0.01f },
		{ TEXT("8 Linear lr 0.1"), { { 8, ENFActivation::Linear } }, 0.1f },
		{ TEXT("4 Tanh + 4 Tanh lr 0.1"), { { 4, ENFActivation::Tanh }, { 4, ENFActivation::Tanh } }, 0.1f },
	};

	for (const FCase& Case : Cases)
	{
		const NFGreenhouseTests::FRunResult R = NFGreenhouseTests::Train(Data, Case.Hidden, Case.LearningRate, MaxEpochs);
		AddInfo(FString::Printf(TEXT("[calibrage] %-22s lr %.2f : 90%% en %5d ep | 95%% en %5d ep | meilleur %.1f%% | final %.1f%% perte %.3f"),
			Case.Name, Case.LearningRate, R.EpochsTo90, R.EpochsTo95, R.BestAccuracy * 100.f, R.FinalAccuracy * 100.f, R.FinalLoss));
	}

	// Assertions : le modèle linéaire plafonne nettement sous la cible,
	// une couche cachée de 4 à 6 neurones Tanh ou ReLU (taux 0,1) l'atteint dans le budget du jeu.
	const FNFGreenhouseProfile Profile;
	for (const TArray<FNFLayerConfig>& Linear : { TArray<FNFLayerConfig>{}, TArray<FNFLayerConfig>{ { 8, ENFActivation::Linear } } })
	{
		const NFGreenhouseTests::FRunResult R = NFGreenhouseTests::Train(Data, Linear, 0.1f, Profile.MaxEpochs * 4);
		TestTrue(TEXT("Un modèle linéaire plafonne sous 80 %"), R.BestAccuracy < 0.80f);
	}
	for (const FNFLayerConfig& Layer : { FNFLayerConfig(4, ENFActivation::ReLU), FNFLayerConfig(5, ENFActivation::ReLU), FNFLayerConfig(6, ENFActivation::ReLU),
		FNFLayerConfig(4, ENFActivation::Tanh), FNFLayerConfig(5, ENFActivation::Tanh), FNFLayerConfig(6, ENFActivation::Tanh) })
	{
		const NFGreenhouseTests::FRunResult R = NFGreenhouseTests::Train(Data, { Layer }, 0.1f, Profile.MaxEpochs);
		TestTrue(FString::Printf(TEXT("%d %s (taux 0,1) atteint %.0f %% en moins de %d epochs"), Layer.NumNeurons,
			*NFActivation::ToString(Layer.Activation), Profile.TargetAccuracy * 100.f, Profile.MaxEpochs),
			R.BestAccuracy >= Profile.TargetAccuracy);
	}
	return true;
}

// ---------------------------------------------------------------------------
namespace NFGreenhouseTests
{
	ANFNetworkBuilder* SpawnGreenhouseBuilder(UWorld* World, const TArray<FNFLayerConfig>& Hidden)
	{
		ANFNetworkBuilder* Builder = World->SpawnActorDeferred<ANFNetworkBuilder>(ANFNetworkBuilder::StaticClass(), FTransform::Identity);
		Builder->Problem = ENFProblem::Greenhouse;
		Builder->HiddenLayers = Hidden;
		Builder->LearningRateIndex = 2;	// 0,1
		Builder->FinishSpawning(FTransform::Identity);
		Builder->GetTrainer()->EpochsPerSecond = 300.f;	// 5 epochs par frame pour le test
		return Builder;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFGreenhouseLabTest, "NeuronForge.Greenhouse.LabSuccessOpensDoor", NFTestUtils::Flags)

bool FNFGreenhouseLabTest::RunTest(const FString& Parameters)
{
	NFTestUtils::FScopedTestWorld TestWorld(TEXT("NFGreenhouseLabWorld"));
	ANFNetworkBuilder* Builder = NFGreenhouseTests::SpawnGreenhouseBuilder(TestWorld.World, { { 5, ENFActivation::ReLU } });

	UNFTrainerComponent* Trainer = Builder->GetTrainer();
	TestEqual(TEXT("Dataset de la serre chargé"), Trainer->GetDataset().Name, FString(TEXT("Serre")));
	TestEqual(TEXT("Précision cible de la serre"), Trainer->TargetAccuracy, 0.95f);
	TestEqual(TEXT("Taux de départ 0,1"), Builder->GetLearningRate(), 0.1f);

	const FTransform DoorTransform(FVector(800.f, 0.f, 0.f));
	ANFLabDoor* Door = TestWorld.World->SpawnActorDeferred<ANFLabDoor>(ANFLabDoor::StaticClass(), DoorTransform);
	Door->Builder = Builder;
	Door->FinishSpawning(DoorTransform);

	TestTrue(TEXT("Démarrage de l'entraînement"), Builder->StartTraining());
	TestWorld.StepUntil([Trainer]() { return !Trainer->IsTraining(); }, 300);
	TestEqual(TEXT("5 ReLU réussit la serre"), Builder->GetTrainingState(), ENFTrainingState::Succeeded);
	AddInfo(FString::Printf(TEXT("Serre réussie en %d epochs (%.1f %%), soit %.0f s de jeu"),
		Trainer->GetProgress().Epoch, Trainer->GetProgress().Accuracy * 100.f, Trainer->GetProgress().Epoch / FNFGreenhouseProfile().EpochsPerSecond));
	TestTrue(TEXT("Un bon modèle réussit en 10 à 30 secondes de jeu"),
		Trainer->GetProgress().Epoch / FNFGreenhouseProfile().EpochsPerSecond <= 30.f);
	TestTrue(TEXT("La porte s'ouvre"), TestWorld.StepUntil([Door]() { return Door->IsOpen(); }, 300));

	// Prédiction ponctuelle (phase de test du pot) : centre de la zone et sécheresse
	const FNFDataset& Data = Trainer->GetDataset();
	TestTrue(TEXT("Conditions idéales : survie prédite"), Trainer->Predict(Data.Normalize(5.f), Data.Normalize(5.5f)) > 0.5f);
	TestTrue(TEXT("Sans eau : mort prédite"), Trainer->Predict(Data.Normalize(0.5f), Data.Normalize(5.5f)) < 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFGreenhouseLinearTest, "NeuronForge.Greenhouse.LinearModelFails", NFTestUtils::Flags)

bool FNFGreenhouseLinearTest::RunTest(const FString& Parameters)
{
	NFTestUtils::FScopedTestWorld TestWorld(TEXT("NFGreenhouseLinearWorld"));
	ANFNetworkBuilder* Builder = NFGreenhouseTests::SpawnGreenhouseBuilder(TestWorld.World, {});

	TestTrue(TEXT("Démarrage de l'entraînement"), Builder->StartTraining());
	TestWorld.StepUntil([Builder]() { return !Builder->GetTrainer()->IsTraining(); }, 300);
	TestEqual(TEXT("Sans couche cachée, la serre échoue"), Builder->GetTrainingState(), ENFTrainingState::Failed);
	TestTrue(TEXT("Précision plafonnée sous 80 %"), Builder->GetTrainer()->GetProgress().Accuracy < 0.8f);
	TestTrue(TEXT("Conseil en mots simples : ligne droite"), Builder->GetHintText().Contains(TEXT("ligne droite")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFGreenhouseNotebookTest, "NeuronForge.Greenhouse.Notebook", NFTestUtils::Flags)

bool FNFGreenhouseNotebookTest::RunTest(const FString& Parameters)
{
	NFTestUtils::FScopedTestWorld TestWorld(TEXT("NFNotebookWorld"));
	ANFNetworkBuilder* Builder = NFGreenhouseTests::SpawnGreenhouseBuilder(TestWorld.World, { { 5, ENFActivation::ReLU } });

	const FTransform Transform(FVector(0.f, 1000.f, 300.f));
	ANFLabNotebook* Notebook = TestWorld.World->SpawnActorDeferred<ANFLabNotebook>(ANFLabNotebook::StaticClass(), Transform);
	Notebook->Builder = Builder;
	Notebook->FinishSpawning(Transform);
	Notebook->Redraw();

	TestNotNull(TEXT("Texture du graphique créée"), Notebook->GetChartTexture());
	TestFalse(TEXT("Avant l'entraînement : pas de zone prédite"), Notebook->ShowsPrediction());
	TestEqual(TEXT("Avant l'entraînement : fond neutre"), Notebook->GetBackgroundColorAt(4.6f, 5.4f), Notebook->GetBackgroundColorAt(0.5f, 0.5f));

	Builder->StartTraining();
	TestWorld.StepUntil([Builder]() { return !Builder->GetTrainer()->IsTraining(); }, 300);
	TestEqual(TEXT("Modèle validé"), Builder->GetTrainingState(), ENFTrainingState::Succeeded);
	Notebook->Redraw();

	TestTrue(TEXT("Après l'entraînement : zone prédite affichée"), Notebook->ShowsPrediction());
	const FColor Center = Notebook->GetBackgroundColorAt(4.6f, 5.4f);
	const FColor Corner = Notebook->GetBackgroundColorAt(0.5f, 9.5f);
	TestTrue(TEXT("Centre de la serre : zone verte (survie prédite)"), Center.G > Center.R);
	TestTrue(TEXT("Coin sec et trop éclairé : zone rouge (mort prédite)"), Corner.R > Corner.G);

	const int32 Expected = FMath::RoundToInt32((1.f - Builder->GetTrainer()->GetProgress().Accuracy) * Builder->GetTrainer()->GetDataset().Num());
	AddInfo(FString::Printf(TEXT("Carnet : %d erreurs affichées"), Notebook->GetMistakeCount()));
	TestEqual(TEXT("Erreurs entourées = expériences mal prédites"), Notebook->GetMistakeCount(), Expected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFGreenhousePlantPotTest, "NeuronForge.Greenhouse.PlantPot", NFTestUtils::Flags)

bool FNFGreenhousePlantPotTest::RunTest(const FString& Parameters)
{
	NFTestUtils::FScopedTestWorld TestWorld(TEXT("NFPlantPotWorld"));
	ANFNetworkBuilder* Builder = NFGreenhouseTests::SpawnGreenhouseBuilder(TestWorld.World, { { 5, ENFActivation::ReLU } });

	const FTransform PotTransform(FVector(500.f, 0.f, 0.f));
	ANFPlantPot* Pot = TestWorld.World->SpawnActorDeferred<ANFPlantPot>(ANFPlantPot::StaticClass(), PotTransform);
	Pot->Builder = Builder;
	Pot->FinishSpawning(PotTransform);

	const FTransform ControlTransform(FVector(400.f, -150.f, 50.f));
	ANFPlantControl* WaterControl = TestWorld.World->SpawnActorDeferred<ANFPlantControl>(ANFPlantControl::StaticClass(), ControlTransform);
	WaterControl->Pot = Pot;
	WaterControl->Setting = ENFPlantSetting::Water;
	WaterControl->FinishSpawning(ControlTransform);

	// Avant la validation : pot verrouillé
	TestFalse(TEXT("Pot verrouillé avant la validation"), Pot->IsUnlocked());
	TestFalse(TEXT("Commande inactive avant la validation"), INFInteractable::Execute_CanInteract(WaterControl, nullptr));
	TestTrue(TEXT("Pas de prédiction avant la validation"), Pot->GetPrediction() < 0.f);
	TestFalse(TEXT("Pas encore testé"), Pot->HasBeenTested());

	Builder->StartTraining();
	TestWorld.StepUntil([Builder]() { return !Builder->GetTrainer()->IsTraining(); }, 300);
	TestEqual(TEXT("Modèle validé"), Builder->GetTrainingState(), ENFTrainingState::Succeeded);
	TestTrue(TEXT("Pot actif après la validation"), Pot->IsUnlocked());

	// Conditions idéales : la plante va bien
	Pot->SetConditions(5, 5);
	TestTrue(TEXT("Eau 5 / lumière 5 : survie prédite"), Pot->GetPrediction() > 0.5f);
	TestTrue(TEXT("Eau 5 / lumière 5 : survie réelle"), Pot->WouldReallySurvive());

	// La commande d'arrosage avance d'un cran à chaque appui, de 10 elle revient à 0
	INFInteractable::Execute_Interact(WaterControl, nullptr);
	TestEqual(TEXT("Arrosage : 5 -> 6"), Pot->GetValue(ENFPlantSetting::Water), 6);
	TestTrue(TEXT("Le joueur a testé une plante"), Pot->HasBeenTested());
	Pot->SetConditions(10, 5);
	INFInteractable::Execute_Interact(WaterControl, nullptr);
	TestEqual(TEXT("Arrosage : 10 -> 0"), Pot->GetValue(ENFPlantSetting::Water), 0);
	TestTrue(TEXT("Sans eau : mort prédite"), Pot->GetPrediction() < 0.5f);
	TestFalse(TEXT("Sans eau : mort réelle"), Pot->WouldReallySurvive());

	// Sur toute la grille de réglages, le modèle validé est d'accord avec la vraie serre presque partout
	int32 Agree = 0;
	for (int32 W = 0; W <= 10; ++W)
	{
		for (int32 L = 0; L <= 10; ++L)
		{
			Pot->SetConditions(W, L);
			Agree += (Pot->GetPrediction() >= 0.5f) == Pot->WouldReallySurvive() ? 1 : 0;
		}
	}
	AddInfo(FString::Printf(TEXT("Pot : le modèle est d'accord avec la vraie serre sur %d réglages sur 121"), Agree));
	TestTrue(TEXT("Accord avec la vraie serre sur au moins 90 % des réglages"), Agree >= 109);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFGreenhouseRoomTest, "NeuronForge.Greenhouse.GreenhouseRoom", NFTestUtils::Flags)

bool FNFGreenhouseRoomTest::RunTest(const FString& Parameters)
{
	NFTestUtils::FScopedTestWorld TestWorld(TEXT("NFGreenhouseRoomWorld"));
	ANFNetworkBuilder* Builder = NFGreenhouseTests::SpawnGreenhouseBuilder(TestWorld.World, { { 5, ENFActivation::ReLU } });

	// Grille 6 x 6 comme dans le niveau : eau sur X, lumière sur Y
	TArray<TObjectPtr<ANFGreenhousePlant>> Plants;
	const float Values[] = { 0.f, 2.f, 4.f, 6.f, 8.f, 10.f };
	for (int32 i = 0; i < 6; ++i)
	{
		for (int32 j = 0; j < 6; ++j)
		{
			const FTransform Transform(FVector(3000.f + i * 180.f, -450.f + j * 180.f, 0.f));
			ANFGreenhousePlant* Plant = TestWorld.World->SpawnActorDeferred<ANFGreenhousePlant>(ANFGreenhousePlant::StaticClass(), Transform);
			Plant->Water = Values[i];
			Plant->Light = Values[j];
			Plant->FinishSpawning(Transform);
			Plants.Add(Plant);
		}
	}

	const FTransform GreenhouseTransform(FVector(2800.f, 0.f, 0.f));
	ANFGreenhouse* Greenhouse = TestWorld.World->SpawnActorDeferred<ANFGreenhouse>(ANFGreenhouse::StaticClass(), GreenhouseTransform);
	Greenhouse->Builder = Builder;
	Greenhouse->Plants = Plants;
	Greenhouse->FinishSpawning(GreenhouseTransform);

	TestFalse(TEXT("Sans modèle validé, la serre ne démarre pas"), Greenhouse->Activate());
	TestFalse(TEXT("Serre en attente"), Greenhouse->IsActivated());

	Builder->StartTraining();
	TestWorld.StepUntil([Builder]() { return !Builder->GetTrainer()->IsTraining(); }, 300);
	TestEqual(TEXT("Modèle validé"), Builder->GetTrainingState(), ENFTrainingState::Succeeded);
	TestTrue(TEXT("Avec le modèle validé, la serre démarre"), Greenhouse->Activate());

	// Le bilan correspond aux prédictions et à la vraie règle de la serre
	int32 Grown = 0, Mistakes = 0, Alive = 0;
	for (const ANFGreenhousePlant* Plant : Plants)
	{
		Grown += Plant->IsPredictedAlive() ? 1 : 0;
		Alive += Plant->ReallySurvives() ? 1 : 0;
		Mistakes += Plant->IsPredictedAlive() != Plant->ReallySurvives() ? 1 : 0;
		TestTrue(TEXT("Chaque plante a reçu son verdict"), Plant->IsRevealed());
	}
	TestEqual(TEXT("36 plantes"), Greenhouse->GetPlantCount(), 36);
	TestEqual(TEXT("Plantes poussées = survies prédites"), Greenhouse->GetGrownCount(), Grown);
	TestEqual(TEXT("Erreurs = prédictions contraires à la vraie serre"), Greenhouse->GetMistakeCount(), Mistakes);
	TestEqual(TEXT("Survies réelles"), Greenhouse->GetReallyAliveCount(), Alive);
	TestTrue(TEXT("Le modèle validé fait pousser des plantes"), Grown > 0 && Grown < 36);
	TestTrue(TEXT("Peu d'erreurs (au plus 4 sur 36)"), Mistakes <= 4);
	TestTrue(TEXT("Le bilan cite le nombre de plantes poussées"),
		Greenhouse->GetSummaryText().Contains(FString::Printf(TEXT("pousser %d plantes sur 36"), Grown)));
	AddInfo(Greenhouse->GetSummaryText().Replace(TEXT("\n"), TEXT(" ")));

	// L'animation se propage puis se termine pour toutes les plantes
	const bool bAllDone = TestWorld.StepUntil([&Plants]()
	{
		for (const ANFGreenhousePlant* Plant : Plants)
		{
			if (Plant->GetAnimationProgress() < 1.f)
			{
				return false;
			}
		}
		return true;
	}, 900);
	TestTrue(TEXT("Toutes les plantes ont fini de pousser ou de faner"), bAllDone);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS




