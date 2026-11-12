// Neuron Forge - ENSIM

#include "NeuralNetwork/NFDataset.h"

bool FNFDataset::IsValid() const
{
	return NumInputs > 0
		&& NumOutputs > 0
		&& Inputs.Num() > 0
		&& Inputs.Num() % NumInputs == 0
		&& RawInputs.Num() == Inputs.Num()
		&& Targets.Num() == Num() * NumOutputs;
}

float FNFDataset::Normalize(float RawValue) const
{
	if (RawMin == 0.f && RawMax == 1.f)
	{
		return RawValue;
	}
	// [RawMin, RawMax] -> [-1, 1]
	return 2.f * (RawValue - RawMin) / FMath::Max(KINDA_SMALL_NUMBER, RawMax - RawMin) - 1.f;
}

void FNFDataset::AddSample(std::initializer_list<float> InInputs, std::initializer_list<float> InTargets)
{
	check(static_cast<int32>(InInputs.size()) == NumInputs);
	check(static_cast<int32>(InTargets.size()) == NumOutputs);

	for (float Value : InInputs)
	{
		Inputs.Add(Value);
		RawInputs.Add(Value);
	}
	for (float Value : InTargets)
	{
		Targets.Add(Value);
	}
}

void FNFDataset::AddRawSample(const TArray<float>& InRawInputs, const TArray<float>& InTargets)
{
	check(InRawInputs.Num() == NumInputs);
	check(InTargets.Num() == NumOutputs);

	for (float Value : InRawInputs)
	{
		RawInputs.Add(Value);
		Inputs.Add(Normalize(Value));
	}
	Targets.Append(InTargets);
}

float FNFDataset::GetPositiveRate() const
{
	const int32 Count = Num();
	int32 Positives = 0;
	for (int32 s = 0; s < Count; ++s)
	{
		Positives += GetTarget(s)[0] >= 0.5f ? 1 : 0;
	}
	return Count > 0 ? static_cast<float>(Positives) / Count : 0.f;
}

FNFDataset FNFDataset::MakeXOR()
{
	FNFDataset Dataset;
	Dataset.Name = TEXT("XOR");
	Dataset.NumInputs = 2;
	Dataset.NumOutputs = 1;
	Dataset.InputNames = { TEXT("x1"), TEXT("x2") };
	Dataset.OutputName = TEXT("XOR");
	Dataset.AddSample({ 0.f, 0.f }, { 0.f });
	Dataset.AddSample({ 0.f, 1.f }, { 1.f });
	Dataset.AddSample({ 1.f, 0.f }, { 1.f });
	Dataset.AddSample({ 1.f, 1.f }, { 0.f });
	return Dataset;
}

bool FNFDataset::GreenhouseSurvives(float Water, float Light)
{
	const float W = (Water - GreenhouseWaterCenter) / GreenhouseWaterRadius;
	const float L = (Light - GreenhouseLightCenter) / GreenhouseLightRadius;
	return W * W + L * L <= 1.f;
}

FNFDataset FNFDataset::MakeGreenhouse(int32 NumSamples, int32 Seed, float MeasurementNoise)
{
	FNFDataset Dataset;
	Dataset.Name = TEXT("Serre");
	Dataset.NumInputs = 2;
	Dataset.NumOutputs = 1;
	Dataset.InputNames = { TEXT("Eau"), TEXT("Lumière") };
	Dataset.OutputName = TEXT("Survie");
	Dataset.RawMin = 0.f;
	Dataset.RawMax = 10.f;

	FRandomStream Rng(Seed);
	auto Gaussian = [&Rng]()
	{
		// Box-Muller
		const float U1 = FMath::Max(Rng.FRand(), 1e-6f);
		const float U2 = Rng.FRand();
		return FMath::Sqrt(-2.f * FMath::Loge(U1)) * FMath::Cos(2.f * PI * U2);
	};

	for (int32 s = 0; s < NumSamples; ++s)
	{
		// Conditions réelles de l'expérience et résultat observé
		const float Water = Rng.FRandRange(0.f, 10.f);
		const float Light = Rng.FRandRange(0.f, 10.f);
		const float Survived = GreenhouseSurvives(Water, Light) ? 1.f : 0.f;

		// Les capteurs mesurent l'eau et la lumière avec une petite erreur
		const float MeasuredWater = FMath::Clamp(Water + MeasurementNoise * Gaussian(), 0.f, 10.f);
		const float MeasuredLight = FMath::Clamp(Light + MeasurementNoise * Gaussian(), 0.f, 10.f);
		Dataset.AddRawSample({ MeasuredWater, MeasuredLight }, { Survived });
	}
	return Dataset;
}
