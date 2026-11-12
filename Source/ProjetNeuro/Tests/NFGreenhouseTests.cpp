// Neuron Forge - ENSIM
//
// Tests du dataset de la serre et mesures de calibrage (précision cible, epochs).

#include "Misc/AutomationTest.h"
#include "NeuralNetwork/NFDataset.h"
#include "NeuralNetwork/NFNeuralNetwork.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
