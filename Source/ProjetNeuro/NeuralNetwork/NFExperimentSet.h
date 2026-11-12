// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "NeuralNetwork/NFDataset.h"
#include "NFExperimentSet.generated.h"

/** Défaut d'une expérience brute, à repérer et à jeter pendant le nettoyage. */
UENUM(BlueprintType)
enum class ENFDataDefect : uint8
{
	None		UMETA(DisplayName = "Aucun"),
	/** Une valeur hors de l'échelle 0 à 10 (capteur déréglé). */
	Impossible	UMETA(DisplayName = "Valeur impossible"),
	/** Une valeur manquante (capteur en panne) ; un programme naïf la remplace par 0. */
	Missing		UMETA(DisplayName = "Valeur manquante"),
	/** La même expérience enregistrée deux fois. */
	Duplicate	UMETA(DisplayName = "Doublon"),
	/** Le résultat noté est faux (erreur de saisie) : il contredit ses voisines. */
	WrongLabel	UMETA(DisplayName = "Étiquette fausse"),
};

/** Une expérience brute de la serre, telle qu'elle a été enregistrée. */
struct PROJETNEURO_API FNFExperiment
{
	/** Numéro affiché sur la fiche. */
	int32 Id = 0;
	/** Valeurs enregistrées (peuvent sortir de 0..10 si le défaut est Impossible). */
	float Water = 0.f;
	float Light = 0.f;
	bool bWaterMissing = false;
	bool bLightMissing = false;
	/** Résultat enregistré (peut être faux si le défaut est WrongLabel). */
	bool bSurvived = false;
	ENFDataDefect Defect = ENFDataDefect::None;
	/** Pour un doublon : indice de l'expérience copiée. */
	int32 DuplicateOf = INDEX_NONE;

	bool IsDefective() const { return Defect != ENFDataDefect::None; }
	bool HasMissingValue() const { return bWaterMissing || bLightMissing; }

	/** Valeurs telles que le programme d'entraînement les lit : une valeur manquante devient 0. */
	float GetTrainingWater() const { return bWaterMissing ? 0.f : Water; }
	float GetTrainingLight() const { return bLightMissing ? 0.f : Light; }
};

/**
 * Les expériences brutes de la serre : 160 bonnes expériences (les mêmes que FNFDataset::MakeGreenhouse,
 * avec leur petit bruit de mesure naturel) et des expériences corrompues, de 4 types.
 * Le joueur nettoie, découpe, puis le modèle s'entraîne sur ce qui reste : les défauts oubliés
 * passent tels quels dans l'entraînement (valeur manquante lue comme 0, valeur impossible, etc.).
 */
struct PROJETNEURO_API FNFExperimentSet
{
	TArray<FNFExperiment> Experiments;

	int32 Num() const { return Experiments.Num(); }

	/**
	 * Génère les expériences : NumClean bonnes et DefectsPerType de chaque type de défaut, mélangées.
	 * Reproductible pour une graine donnée.
	 */
	static FNFExperimentSet MakeGreenhouse(int32 NumClean = 160, int32 DefectsPerType = 10, int32 Seed = 2026);

	/** Nombre d'expériences défectueuses parmi Indices. */
	int32 CountDefects(const TArray<int32>& Indices, ENFDataDefect Type = ENFDataDefect::None) const;

	/** Indices de toutes les expériences (pour partir d'un jeu complet). */
	TArray<int32> AllIndices() const;

	/** Indices des expériences sans défaut. */
	TArray<int32> CleanIndices() const;

	/** Dataset d'entraînement (entrées normalisées) construit à partir de ces expériences. */
	FNFDataset ToDataset(const TArray<int32>& Indices, const FString& Name = TEXT("Serre")) const;

	/** Étiquette fausse probable : la majorité de ses K plus proches voisines dit le contraire (scanner). */
	bool ContradictsNeighbors(int32 Index, const TArray<int32>& Among, int32 K = 5, int32 MinDisagree = 4) const;

	/**
	 * Mélange les expériences Kept et les répartit entre entraînement et validation.
	 * TrainFraction entre 0 et 1 ; si SmallBatch > 0, l'entraînement ne reçoit que SmallBatch expériences
	 * (« petit lot ») et la validation garde tout le reste.
	 */
	static void Split(const TArray<int32>& Kept, float TrainFraction, int32 Seed, TArray<int32>& OutTrain, TArray<int32>& OutValidation,
		int32 SmallBatch = 0);
};
