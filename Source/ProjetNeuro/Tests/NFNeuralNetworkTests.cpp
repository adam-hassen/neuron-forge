// Neuron Forge - ENSIM
//
// Tests automatisés du réseau de neurones.
// Lancement : Session Frontend > Automation, filtre "NeuronForge",
// ou en ligne de commande : -ExecCmds="Automation RunTests NeuronForge; Quit"

#include "Misc/AutomationTest.h"
#include "NeuralNetwork/NFActivation.h"
#include "NeuralNetwork/NFDataset.h"
#include "NeuralNetwork/NFNeuralNetwork.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace NFTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ClientContext
		| EAutomationTestFlags::ProductFilter;

	/** Entraîne jusqu'à 100 % de précision ou MaxEpochs. Renvoie le nombre d'epochs utilisées (-1 si échec). */
	int32 TrainUntilSolved(FNFNeuralNetwork& Network, const FNFDataset& Dataset, float LearningRate,
		int32 MaxEpochs, FNFEvaluation& OutEvaluation, int32 Seed = 7)
	{
		FRandomStream Rng(Seed);
		for (int32 Epoch = 1; Epoch <= MaxEpochs; ++Epoch)
		{
			Network.TrainEpoch(Dataset, LearningRate, Rng);
			OutEvaluation = Network.Evaluate(Dataset);
			if (OutEvaluation.Accuracy >= 1.f && OutEvaluation.Loss < 0.1f)
			{
				return Epoch;
			}
		}
		return -1;
	}
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFActivationTest, "NeuronForge.NeuralNetwork.Activations", NFTests::Flags)

bool FNFActivationTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("ReLU(-2) = 0"), NFActivation::Apply(ENFActivation::ReLU, -2.f), 0.f);
	TestEqual(TEXT("ReLU(3) = 3"), NFActivation::Apply(ENFActivation::ReLU, 3.f), 3.f);
	TestEqual(TEXT("Sigmoid(0) = 0.5"), NFActivation::Apply(ENFActivation::Sigmoid, 0.f), 0.5f);
	TestTrue(TEXT("Sigmoid(-100) est fini et proche de 0"), NFActivation::Apply(ENFActivation::Sigmoid, -100.f) < 1e-6f);
	TestTrue(TEXT("Sigmoid(100) est proche de 1"), NFActivation::Apply(ENFActivation::Sigmoid, 100.f) > 1.f - 1e-6f);
	TestEqual(TEXT("Tanh(0) = 0"), NFActivation::Apply(ENFActivation::Tanh, 0.f), 0.f);
	TestEqual(TEXT("Linear(1.5) = 1.5"), NFActivation::Apply(ENFActivation::Linear, 1.5f), 1.5f);

	// Le cycle des activations passe par toutes les valeurs et revient au départ
	ENFActivation Activation = ENFActivation::Linear;
	for (int32 i = 0; i < static_cast<int32>(ENFActivation::Count); ++i)
	{
		Activation = NFActivation::Next(Activation);
	}
	TestEqual(TEXT("Next() boucle après Count étapes"), Activation, ENFActivation::Linear);
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFGradientCheckTest, "NeuronForge.NeuralNetwork.GradientCheck", NFTests::Flags)

bool FNFGradientCheckTest::RunTest(const FString& Parameters)
{
	// Compare les gradients de la rétropropagation aux dérivées numériques
	// (différences finies centrées), pour les deux fonctions de perte.
	struct FCase
	{
		const TCHAR* Name;
		TArray<FNFLayerConfig> Hidden;
		ENFActivation OutputActivation;
	};
	const TArray<FCase> Cases = {
		{ TEXT("Tanh + Sigmoid (BCE)"), { { 3, ENFActivation::Tanh }, { 2, ENFActivation::Sigmoid } }, ENFActivation::Sigmoid },
		{ TEXT("ReLU + Linear (MSE)"), { { 4, ENFActivation::ReLU } }, ENFActivation::Linear },
		{ TEXT("Tanh + Tanh (MSE)"), { { 3, ENFActivation::Tanh } }, ENFActivation::Tanh },
	};

	const float Input[2] = { 0.3f, -0.7f };
	const float Target[2] = { 1.f, 0.f };
	constexpr float Epsilon = 1e-3f;
	constexpr float Tolerance = 2e-3f;

	for (const FCase& Case : Cases)
	{
		FNFNeuralNetwork Network;
		Network.Build(2, Case.Hidden, 2, Case.OutputActivation, 123);

		Network.Forward(Input);
		Network.Backward(Target);

		float MaxError = 0.f;
		TArray<FNFDenseLayer>& Layers = Network.GetLayersMutable();
		for (FNFDenseLayer& Layer : Layers)
		{
			for (int32 k = 0; k < Layer.Weights.Num(); ++k)
			{
				const float Analytic = Layer.WeightGrads[k];
				const float Original = Layer.Weights[k];

				Layer.Weights[k] = Original + Epsilon;
				const float LossPlus = Network.ComputeLoss(Network.Forward(Input).GetData(), Target);
				Layer.Weights[k] = Original - Epsilon;
				const float LossMinus = Network.ComputeLoss(Network.Forward(Input).GetData(), Target);
				Layer.Weights[k] = Original;

				const float Numeric = (LossPlus - LossMinus) / (2.f * Epsilon);
				MaxError = FMath::Max(MaxError, FMath::Abs(Numeric - Analytic));
			}
		}

		AddInfo(FString::Printf(TEXT("%s : écart max analytique/numérique = %.6f"), Case.Name, MaxError));
		TestTrue(FString::Printf(TEXT("Gradients corrects pour %s"), Case.Name), MaxError < Tolerance);
	}
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFDeterminismTest, "NeuronForge.NeuralNetwork.Determinism", NFTests::Flags)

bool FNFDeterminismTest::RunTest(const FString& Parameters)
{
	FNFNeuralNetwork A;
	FNFNeuralNetwork B;
	A.Build(2, { { 4, ENFActivation::Tanh } }, 1, ENFActivation::Sigmoid, 42);
	B.Build(2, { { 4, ENFActivation::Tanh } }, 1, ENFActivation::Sigmoid, 42);

	TestEqual(TEXT("Même graine => mêmes poids"), A.GetLayers()[0].Weights, B.GetLayers()[0].Weights);
	TestEqual(TEXT("Nombre de paramètres de 2-4-1"), A.GetNumParameters(), 2 * 4 + 4 + 4 * 1 + 1);
	TestEqual(TEXT("Description"), A.Describe(), FString(TEXT("2 - 4 Tanh - 1 Sigmoid")));
	TestEqual(TEXT("Perte BCE avec sortie Sigmoid"), A.GetLoss(), ENFLoss::BinaryCrossEntropy);
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFXORSolvedTest, "NeuronForge.NeuralNetwork.XORSolved", NFTests::Flags)

bool FNFXORSolvedTest::RunTest(const FString& Parameters)
{
	const FNFDataset XOR = FNFDataset::MakeXOR();
	TestTrue(TEXT("Dataset XOR valide"), XOR.IsValid());
	TestEqual(TEXT("XOR a 4 exemples"), XOR.Num(), 4);

	// Configuration de référence du MVP : 2 - 4 Tanh - 1 Sigmoid doit résoudre XOR
	{
		FNFNeuralNetwork Network;
		Network.Build(2, { { 4, ENFActivation::Tanh } }, 1, ENFActivation::Sigmoid, 42);

		FNFEvaluation Evaluation;
		const int32 Epochs = NFTests::TrainUntilSolved(Network, XOR, 0.5f, 5000, Evaluation);
		AddInfo(FString::Printf(TEXT("%s : résolu en %d epochs (perte %.4f, précision %.0f %%)"),
			*Network.Describe(), Epochs, Evaluation.Loss, Evaluation.Accuracy * 100.f));
		TestTrue(TEXT("2 - 4 Tanh - 1 Sigmoid résout XOR en moins de 5000 epochs"), Epochs > 0);
	}

	// Autres configurations : informatif, sert à calibrer le gameplay
	struct FCase
	{
		TArray<FNFLayerConfig> Hidden;
		float LearningRate;
	};
	const TArray<FCase> Cases = {
		{ { { 2, ENFActivation::Tanh } }, 0.5f },
		{ { { 8, ENFActivation::ReLU } }, 0.1f },
		{ { { 4, ENFActivation::Sigmoid } }, 1.0f },
		{ { { 4, ENFActivation::Tanh }, { 4, ENFActivation::Tanh } }, 0.3f },
		{ { { 4, ENFActivation::Tanh } }, 0.01f },
	};
	for (const FCase& Case : Cases)
	{
		FNFNeuralNetwork Network;
		Network.Build(2, Case.Hidden, 1, ENFActivation::Sigmoid, 42);
		FNFEvaluation Evaluation;
		const int32 Epochs = NFTests::TrainUntilSolved(Network, XOR, Case.LearningRate, 5000, Evaluation);
		AddInfo(FString::Printf(TEXT("[calibrage] %s, lr %.2f : %s (perte %.4f, précision %.0f %%)"),
			*Network.Describe(), Case.LearningRate,
			Epochs > 0 ? *FString::Printf(TEXT("résolu en %d epochs"), Epochs) : TEXT("non résolu en 5000 epochs"),
			Evaluation.Loss, Evaluation.Accuracy * 100.f));
	}
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFXORLinearFailsTest, "NeuronForge.NeuralNetwork.XORLinearFails", NFTests::Flags)

bool FNFXORLinearFailsTest::RunTest(const FString& Parameters)
{
	// Notion clé du jeu : XOR n'est pas linéairement séparable.
	// Sans couche cachée non linéaire, la précision ne peut pas dépasser 75 %.
	const FNFDataset XOR = FNFDataset::MakeXOR();

	struct FCase
	{
		const TCHAR* Name;
		TArray<FNFLayerConfig> Hidden;
	};
	const TArray<FCase> Cases = {
		{ TEXT("sans couche cachée"), {} },
		{ TEXT("couche cachée Linear"), { { 8, ENFActivation::Linear } } },
	};

	for (const FCase& Case : Cases)
	{
		FNFNeuralNetwork Network;
		Network.Build(2, Case.Hidden, 1, ENFActivation::Sigmoid, 42);

		FNFEvaluation Evaluation;
		const int32 Epochs = NFTests::TrainUntilSolved(Network, XOR, 0.5f, 3000, Evaluation);
		AddInfo(FString::Printf(TEXT("%s (%s) : précision finale %.0f %%"),
			Case.Name, *Network.Describe(), Evaluation.Accuracy * 100.f));
		TestTrue(FString::Printf(TEXT("XOR non résolu %s"), Case.Name), Epochs < 0 && Evaluation.Accuracy <= 0.75f);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
