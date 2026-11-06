// Neuron Forge - ENSIM

#include "NeuralNetwork/NFDataset.h"

bool FNFDataset::IsValid() const
{
	return NumInputs > 0
		&& NumOutputs > 0
		&& Inputs.Num() > 0
		&& Inputs.Num() % NumInputs == 0
		&& Targets.Num() == Num() * NumOutputs;
}

void FNFDataset::AddSample(std::initializer_list<float> InInputs, std::initializer_list<float> InTargets)
{
	check(static_cast<int32>(InInputs.size()) == NumInputs);
	check(static_cast<int32>(InTargets.size()) == NumOutputs);

	for (float Value : InInputs)
	{
		Inputs.Add(Value);
	}
	for (float Value : InTargets)
	{
		Targets.Add(Value);
	}
}

FNFDataset FNFDataset::MakeXOR()
{
	FNFDataset Dataset;
	Dataset.Name = TEXT("XOR");
	Dataset.NumInputs = 2;
	Dataset.NumOutputs = 1;
	Dataset.AddSample({ 0.f, 0.f }, { 0.f });
	Dataset.AddSample({ 0.f, 1.f }, { 1.f });
	Dataset.AddSample({ 1.f, 0.f }, { 1.f });
	Dataset.AddSample({ 1.f, 1.f }, { 0.f });
	return Dataset;
}
