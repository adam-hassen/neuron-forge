// Neuron Forge - ENSIM

#include "NeuralNetwork/NFNeuralNetwork.h"
#include "NeuralNetwork/NFDataset.h"

namespace
{
	constexpr float LossEpsilon = 1e-7f;

	void InitLayer(FNFDenseLayer& Layer, int32 InNumInputs, int32 InNumOutputs, ENFActivation Activation)
	{
		Layer.NumInputs = InNumInputs;
		Layer.NumOutputs = InNumOutputs;
		Layer.Activation = Activation;

		Layer.Weights.SetNumZeroed(InNumInputs * InNumOutputs);
		Layer.WeightGrads.SetNumZeroed(InNumInputs * InNumOutputs);
		Layer.Biases.SetNumZeroed(InNumOutputs);
		Layer.BiasGrads.SetNumZeroed(InNumOutputs);
		Layer.Z.SetNumZeroed(InNumOutputs);
		Layer.A.SetNumZeroed(InNumOutputs);
		Layer.Delta.SetNumZeroed(InNumOutputs);
	}
}

void FNFNeuralNetwork::Build(int32 InNumInputs, const TArray<FNFLayerConfig>& HiddenLayers, int32 InNumOutputs,
	ENFActivation OutputActivation, int32 Seed)
{
	check(InNumInputs > 0 && InNumOutputs > 0);

	NumInputs = InNumInputs;
	Layers.Reset();
	LastInput.SetNumZeroed(NumInputs);

	int32 PreviousSize = NumInputs;
	for (const FNFLayerConfig& Config : HiddenLayers)
	{
		const int32 Size = FMath::Max(1, Config.NumNeurons);
		InitLayer(Layers.AddDefaulted_GetRef(), PreviousSize, Size, Config.Activation);
		PreviousSize = Size;
	}
	InitLayer(Layers.AddDefaulted_GetRef(), PreviousSize, InNumOutputs, OutputActivation);

	// L'entropie croisée binaire n'a de sens qu'avec des sorties dans ]0, 1[
	Loss = (OutputActivation == ENFActivation::Sigmoid) ? ENFLoss::BinaryCrossEntropy : ENFLoss::MSE;

	InitializeWeights(Seed);
}

void FNFNeuralNetwork::InitializeWeights(int32 Seed)
{
	FRandomStream Rng(Seed);

	for (FNFDenseLayer& Layer : Layers)
	{
		// He (uniforme) pour ReLU, Xavier/Glorot (uniforme) pour les autres activations
		const float Limit = (Layer.Activation == ENFActivation::ReLU)
			? FMath::Sqrt(6.f / Layer.NumInputs)
			: FMath::Sqrt(6.f / (Layer.NumInputs + Layer.NumOutputs));

		for (float& Weight : Layer.Weights)
		{
			Weight = Rng.FRandRange(-Limit, Limit);
		}
		for (float& Bias : Layer.Biases)
		{
			Bias = 0.f;
		}
	}
}

const TArray<float>& FNFNeuralNetwork::Forward(const float* Input)
{
	check(IsValid());
	FMemory::Memcpy(LastInput.GetData(), Input, NumInputs * sizeof(float));

	const float* LayerInput = Input;
	for (FNFDenseLayer& Layer : Layers)
	{
		for (int32 o = 0; o < Layer.NumOutputs; ++o)
		{
			float Sum = Layer.Biases[o];
			const float* Row = Layer.Weights.GetData() + o * Layer.NumInputs;
			for (int32 i = 0; i < Layer.NumInputs; ++i)
			{
				Sum += Row[i] * LayerInput[i];
			}
			Layer.Z[o] = Sum;
			Layer.A[o] = NFActivation::Apply(Layer.Activation, Sum);
		}
		LayerInput = Layer.A.GetData();
	}
	return Layers.Last().A;
}

void FNFNeuralNetwork::Backward(const float* Target)
{
	check(IsValid());

	// 1) Delta de la couche de sortie : dL/dz
	FNFDenseLayer& OutputLayer = Layers.Last();
	for (int32 o = 0; o < OutputLayer.NumOutputs; ++o)
	{
		const float A = OutputLayer.A[o];
		if (Loss == ENFLoss::BinaryCrossEntropy)
		{
			// Avec sigmoid + entropie croisée, dL/dz se simplifie en (a - y)
			OutputLayer.Delta[o] = A - Target[o];
		}
		else
		{
			// MSE : dL/da = (a - y), puis règle de la chaîne
			OutputLayer.Delta[o] = (A - Target[o]) * NFActivation::Derivative(OutputLayer.Activation, OutputLayer.Z[o], A);
		}
	}

	// 2) Propagation des deltas vers les couches cachées
	for (int32 l = Layers.Num() - 2; l >= 0; --l)
	{
		FNFDenseLayer& Layer = Layers[l];
		const FNFDenseLayer& NextLayer = Layers[l + 1];

		for (int32 j = 0; j < Layer.NumOutputs; ++j)
		{
			float Sum = 0.f;
			for (int32 o = 0; o < NextLayer.NumOutputs; ++o)
			{
				Sum += NextLayer.GetWeight(o, j) * NextLayer.Delta[o];
			}
			Layer.Delta[j] = Sum * NFActivation::Derivative(Layer.Activation, Layer.Z[j], Layer.A[j]);
		}
	}

	// 3) Gradients des poids et des biais : dL/dW[o][i] = delta[o] * entrée[i]
	for (int32 l = 0; l < Layers.Num(); ++l)
	{
		FNFDenseLayer& Layer = Layers[l];
		const float* LayerInput = (l == 0) ? LastInput.GetData() : Layers[l - 1].A.GetData();

		for (int32 o = 0; o < Layer.NumOutputs; ++o)
		{
			const float Delta = Layer.Delta[o];
			float* GradRow = Layer.WeightGrads.GetData() + o * Layer.NumInputs;
			for (int32 i = 0; i < Layer.NumInputs; ++i)
			{
				GradRow[i] = Delta * LayerInput[i];
			}
			Layer.BiasGrads[o] = Delta;
		}
	}
}

void FNFNeuralNetwork::ApplyGradients(float LearningRate)
{
	for (FNFDenseLayer& Layer : Layers)
	{
		for (int32 k = 0; k < Layer.Weights.Num(); ++k)
		{
			Layer.Weights[k] -= LearningRate * Layer.WeightGrads[k];
		}
		for (int32 o = 0; o < Layer.Biases.Num(); ++o)
		{
			Layer.Biases[o] -= LearningRate * Layer.BiasGrads[o];
		}
	}
}

float FNFNeuralNetwork::TrainSample(const float* Input, const float* Target, float LearningRate)
{
	const TArray<float>& Output = Forward(Input);
	const float SampleLoss = ComputeLoss(Output.GetData(), Target);
	Backward(Target);
	ApplyGradients(LearningRate);
	return SampleLoss;
}

float FNFNeuralNetwork::TrainEpoch(const FNFDataset& Dataset, float LearningRate, FRandomStream& Rng)
{
	check(Dataset.NumInputs == NumInputs && Dataset.NumOutputs == GetNumOutputs());

	const int32 NumSamples = Dataset.Num();
	if (ShuffleOrder.Num() != NumSamples)
	{
		ShuffleOrder.SetNum(NumSamples);
		for (int32 s = 0; s < NumSamples; ++s)
		{
			ShuffleOrder[s] = s;
		}
	}

	// Mélange de Fisher-Yates
	for (int32 s = NumSamples - 1; s > 0; --s)
	{
		ShuffleOrder.Swap(s, Rng.RandRange(0, s));
	}

	float TotalLoss = 0.f;
	for (int32 Index : ShuffleOrder)
	{
		TotalLoss += TrainSample(Dataset.GetInput(Index), Dataset.GetTarget(Index), LearningRate);
	}
	return NumSamples > 0 ? TotalLoss / NumSamples : 0.f;
}

FNFEvaluation FNFNeuralNetwork::Evaluate(const FNFDataset& Dataset)
{
	check(Dataset.NumInputs == NumInputs && Dataset.NumOutputs == GetNumOutputs());

	FNFEvaluation Result;
	Result.NumSamples = Dataset.Num();

	float TotalLoss = 0.f;
	for (int32 s = 0; s < Result.NumSamples; ++s)
	{
		const TArray<float>& Output = Forward(Dataset.GetInput(s));
		TotalLoss += ComputeLoss(Output.GetData(), Dataset.GetTarget(s));
		if (IsPredictionCorrect(Output.GetData(), Dataset.GetTarget(s)))
		{
			++Result.NumCorrect;
		}
	}

	if (Result.NumSamples > 0)
	{
		Result.Loss = TotalLoss / Result.NumSamples;
		Result.Accuracy = static_cast<float>(Result.NumCorrect) / Result.NumSamples;
	}
	return Result;
}

float FNFNeuralNetwork::ComputeLoss(const float* Output, const float* Target) const
{
	const int32 NumOutputs = GetNumOutputs();
	float Sum = 0.f;

	for (int32 o = 0; o < NumOutputs; ++o)
	{
		if (Loss == ENFLoss::BinaryCrossEntropy)
		{
			const float A = FMath::Clamp(Output[o], LossEpsilon, 1.f - LossEpsilon);
			Sum -= Target[o] * FMath::Loge(A) + (1.f - Target[o]) * FMath::Loge(1.f - A);
		}
		else
		{
			const float Error = Output[o] - Target[o];
			Sum += 0.5f * Error * Error;
		}
	}
	return Sum;
}

bool FNFNeuralNetwork::IsPredictionCorrect(const float* Output, const float* Target) const
{
	const int32 NumOutputs = GetNumOutputs();

	if (NumOutputs == 1)
	{
		return (Output[0] >= 0.5f) == (Target[0] >= 0.5f);
	}

	int32 PredictedClass = 0;
	int32 TargetClass = 0;
	for (int32 o = 1; o < NumOutputs; ++o)
	{
		if (Output[o] > Output[PredictedClass])
		{
			PredictedClass = o;
		}
		if (Target[o] > Target[TargetClass])
		{
			TargetClass = o;
		}
	}
	return PredictedClass == TargetClass;
}

int32 FNFNeuralNetwork::GetNumParameters() const
{
	int32 Count = 0;
	for (const FNFDenseLayer& Layer : Layers)
	{
		Count += Layer.Weights.Num() + Layer.Biases.Num();
	}
	return Count;
}

FString FNFNeuralNetwork::Describe() const
{
	FString Result = FString::FromInt(NumInputs);
	for (const FNFDenseLayer& Layer : Layers)
	{
		Result += FString::Printf(TEXT(" - %d %s"), Layer.NumOutputs, *NFActivation::ToString(Layer.Activation));
	}
	return Result;
}
