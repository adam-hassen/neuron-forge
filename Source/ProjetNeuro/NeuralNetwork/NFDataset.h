// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"

/**
 * Dataset supervisé stocké à plat : NumSamples exemples de NumInputs entrées
 * et NumOutputs cibles chacun.
 *
 * Les entrées vues par le réseau (Inputs) peuvent être normalisées ; les valeurs
 * d'origine, lisibles par le joueur, sont conservées dans RawInputs.
 */
struct PROJETNEURO_API FNFDataset
{
	FString Name;
	int32 NumInputs = 0;
	int32 NumOutputs = 0;

	/** Noms affichés des entrées (« Eau », « Lumière »...) et de la sortie (« Survie »). */
	TArray<FString> InputNames;
	FString OutputName;

	/** Plage des valeurs brutes de chaque entrée (pour les graphiques et la normalisation). */
	float RawMin = 0.f;
	float RawMax = 1.f;

	/** Entrées normalisées données au réseau : [Sample * NumInputs + i] */
	TArray<float> Inputs;

	/** Entrées brutes, dans l'unité du problème : [Sample * NumInputs + i] */
	TArray<float> RawInputs;

	/** Cibles, échantillon par échantillon : [Sample * NumOutputs + o] */
	TArray<float> Targets;

	int32 Num() const { return NumInputs > 0 ? Inputs.Num() / NumInputs : 0; }
	bool IsValid() const;

	const float* GetInput(int32 Sample) const { return Inputs.GetData() + Sample * NumInputs; }
	const float* GetRawInput(int32 Sample) const { return RawInputs.GetData() + Sample * NumInputs; }
	const float* GetTarget(int32 Sample) const { return Targets.GetData() + Sample * NumOutputs; }

	/** Valeur brute -> valeur vue par le réseau. Identité si RawMin = 0 et RawMax = 1. */
	float Normalize(float RawValue) const;

	/** Ajoute un exemple dont les entrées sont déjà dans l'unité du réseau (brutes = normalisées). */
	void AddSample(std::initializer_list<float> InInputs, std::initializer_list<float> InTargets);

	/** Ajoute un exemple à partir de valeurs brutes, normalisées automatiquement. */
	void AddRawSample(const TArray<float>& InRawInputs, const TArray<float>& InTargets);

	/** Proportion d'exemples dont la première sortie vaut 1. */
	float GetPositiveRate() const;

	/** Le OU exclusif : 4 exemples, 2 entrées, 1 sortie binaire. Non linéairement séparable. */
	static FNFDataset MakeXOR();

	// --- La serre du labo ---

	/**
	 * Règle « vraie » de la serre : la plante survit seulement dans une zone ovale
	 * au milieu (ni trop ni pas assez d'eau, ni trop ni pas assez de lumière).
	 * Eau et lumière entre 0 et 10.
	 */
	static bool GreenhouseSurvives(float Water, float Light);

	/**
	 * Expériences de la serre : eau et lumière tirées entre 0 et 10, résultat donné
	 * par la règle, puis bruit de mesure sur les entrées (quelques points trompeurs
	 * près de la frontière). Entrées normalisées entre -1 et 1 pour le réseau.
	 */
	static FNFDataset MakeGreenhouse(int32 NumSamples = 160, int32 Seed = 2026, float MeasurementNoise = 0.35f);

	/** Paramètres de la zone de survie (centre et rayons de l'ovale). */
	static constexpr float GreenhouseWaterCenter = 5.f;
	static constexpr float GreenhouseLightCenter = 5.5f;
	static constexpr float GreenhouseWaterRadius = 3.3f;
	static constexpr float GreenhouseLightRadius = 3.1f;
};
