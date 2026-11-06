// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"

/**
 * Dataset supervisé stocké à plat : NumSamples exemples de NumInputs entrées
 * et NumOutputs cibles chacun.
 */
struct PROJETNEURO_API FNFDataset
{
	FString Name;
	int32 NumInputs = 0;
	int32 NumOutputs = 0;

	/** Entrées, échantillon par échantillon : [Sample * NumInputs + i] */
	TArray<float> Inputs;

	/** Cibles, échantillon par échantillon : [Sample * NumOutputs + o] */
	TArray<float> Targets;

	int32 Num() const { return NumInputs > 0 ? Inputs.Num() / NumInputs : 0; }
	bool IsValid() const;

	const float* GetInput(int32 Sample) const { return Inputs.GetData() + Sample * NumInputs; }
	const float* GetTarget(int32 Sample) const { return Targets.GetData() + Sample * NumOutputs; }

	/** Ajoute un exemple ; les tailles doivent correspondre à NumInputs / NumOutputs. */
	void AddSample(std::initializer_list<float> InInputs, std::initializer_list<float> InTargets);

	/** Le OU exclusif : 4 exemples, 2 entrées, 1 sortie binaire. Non linéairement séparable. */
	static FNFDataset MakeXOR();
};
