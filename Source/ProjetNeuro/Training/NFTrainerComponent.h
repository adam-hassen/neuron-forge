// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NeuralNetwork/NFDataset.h"
#include "NeuralNetwork/NFNeuralNetwork.h"
#include "NFTrainerComponent.generated.h"

/** État de l'entraînement du modèle. */
UENUM(BlueprintType)
enum class ENFTrainingState : uint8
{
	/** Configuration en cours, modèle non entraîné */
	Idle		UMETA(DisplayName = "En attente"),
	Training	UMETA(DisplayName = "Entraînement"),
	/** Précision cible atteinte */
	Succeeded	UMETA(DisplayName = "Réussi"),
	/** Nombre maximal d'epochs atteint sans la précision cible */
	Failed		UMETA(DisplayName = "Échec"),
};

/** Instantané de la progression de l'entraînement. */
USTRUCT(BlueprintType)
struct PROJETNEURO_API FNFTrainingProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Neuron Forge")
	int32 Epoch = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Neuron Forge")
	float Loss = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Neuron Forge")
	float Accuracy = 0.f;
};

/** Problème à résoudre dans le labo. */
UENUM(BlueprintType)
enum class ENFProblem : uint8
{
	/** Le OU exclusif : 4 exemples (tests, démonstration). */
	XOR			UMETA(DisplayName = "XOR"),
	/** La serre : survie d'une plante selon l'eau et la lumière (niveau jouable). */
	Greenhouse	UMETA(DisplayName = "Serre"),
};

/** Réglages d'entraînement d'un problème. */
struct FNFTrainingProfile
{
	float TargetAccuracy = 1.f;
	float TargetLoss = 0.1f;
	int32 MaxEpochs = 2000;
	float EpochsPerSecond = 120.f;
	/** Cran du levier au départ (0,01 / 0,05 / 0,1 / 0,3 / 0,5 / 1 / 2). */
	int32 DefaultLearningRateIndex = 4;
};

/**
 * Réglages de la serre, calibrés par le test NeuronForge.Greenhouse.Calibration :
 * le modèle linéaire plafonne à 70 %, une couche de 4 à 6 neurones Tanh ou ReLU (taux 0,1)
 * dépasse 95 % en 10 à 33 epochs, soit 8 à 28 secondes de jeu à 1,2 epoch par seconde ;
 * un modèle qui échoue s'arrête après 40 epochs (33 secondes).
 * La perte cible est large : avec le bruit de mesure, elle ne descend pas sous ~0,08.
 */
struct FNFGreenhouseProfile : public FNFTrainingProfile
{
	FNFGreenhouseProfile()
	{
		TargetAccuracy = 0.95f;
		TargetLoss = 0.3f;
		MaxEpochs = 40;
		EpochsPerSecond = 1.2f;
		DefaultLearningRateIndex = 2;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNFOnTrainingStateChanged, ENFTrainingState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNFOnTrainingProgress, const FNFTrainingProgress&, Progress);

/**
 * Entraîne un FNFNeuralNetwork sur un dataset, progressivement, au fil des frames,
 * pour que le joueur voie le modèle apprendre.
 *
 * Le rythme est fixé en epochs par seconde ; l'entraînement s'arrête dès que la
 * précision et la perte cibles sont atteintes (Succeeded) ou après MaxEpochs (Failed).
 */
UCLASS(ClassGroup = (NeuronForge), meta = (BlueprintSpawnableComponent))
class PROJETNEURO_API UNFTrainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNFTrainerComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Reconstruit le réseau avec cette architecture ; interrompt un entraînement en cours et repasse en Idle. */
	void Configure(const TArray<FNFLayerConfig>& HiddenLayers, float InLearningRate);

	/** Remplace le dataset (XOR par défaut) et reconstruit le réseau. */
	void SetDataset(const FNFDataset& InDataset);

	/** Applique la précision, la perte, le nombre d'epochs et la vitesse d'un profil. */
	void ApplyProfile(const FNFTrainingProfile& Profile);

	/** Sortie du réseau (premier neurone) pour des entrées déjà normalisées. Ne modifie pas les poids. */
	float Predict(float InputA, float InputB);

	/** Réinitialise les poids et démarre l'entraînement. Faux si un entraînement est déjà en cours. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Training")
	bool StartTraining();

	/** Interrompt l'entraînement et repasse en Idle. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Training")
	void StopTraining();

	/** Exécute NumEpochs epochs complètes immédiatement (tests, accélération). */
	void Advance(int32 NumEpochs);

	/** Entraîne sur NumSamples exemples, en continuant l'epoch en cours (utilisé par Tick). */
	void AdvanceSamples(int32 NumSamples);

	/** Sortie du réseau (premier neurone de sortie) pour chaque exemple du dataset. */
	void GetPredictions(TArray<float>& OutPredictions);

	/** La précision et la perte cibles sont-elles atteintes ? */
	bool IsTargetReached(const FNFEvaluation& Evaluation) const;

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Training")
	ENFTrainingState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Training")
	bool IsTraining() const { return State == ENFTrainingState::Training; }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Training")
	const FNFTrainingProgress& GetProgress() const { return Progress; }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Training")
	float GetLearningRate() const { return LearningRate; }

	/** Perte à chaque epoch du dernier entraînement (pour une future courbe). */
	const TArray<float>& GetLossHistory() const { return LossHistory; }

	const FNFNeuralNetwork& GetNetwork() const { return Network; }
	FNFNeuralNetwork& GetMutableNetwork() { return Network; }
	const FNFDataset& GetDataset() const { return Dataset; }

	/** Vitesse de l'entraînement visible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Training", meta = (ClampMin = "1"))
	float EpochsPerSecond = 120.f;

	/** Au-delà, l'entraînement échoue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Training", meta = (ClampMin = "1"))
	int32 MaxEpochs = 2000;

	/** Précision à atteindre, entre 0 et 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Training", meta = (ClampMin = "0", ClampMax = "1"))
	float TargetAccuracy = 1.f;

	/** Perte maximale exigée en plus de la précision : le modèle doit être confiant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Training", meta = (ClampMin = "0"))
	float TargetLoss = 0.1f;

	/** Graine de l'initialisation des poids et du mélange des exemples. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Training")
	int32 Seed = 42;

	UPROPERTY(BlueprintAssignable, Category = "Neuron Forge|Training")
	FNFOnTrainingStateChanged OnStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Neuron Forge|Training")
	FNFOnTrainingProgress OnProgress;

private:
	void RebuildNetwork();
	void SetState(ENFTrainingState NewState);

	/** Entraîne sur l'exemple suivant de l'epoch ; termine l'epoch quand elle est complète. */
	void TrainNextSample();
	void FinishEpoch();

	FNFNeuralNetwork Network;
	FNFDataset Dataset;
	TArray<FNFLayerConfig> HiddenConfig;
	TArray<float> LossHistory;
	FRandomStream ShuffleRng;
	TArray<int32> ShuffleOrder;
	int32 SampleCursor = 0;
	FNFTrainingProgress Progress;
	ENFTrainingState State = ENFTrainingState::Idle;
	float LearningRate = 0.5f;
	float EpochAccumulator = 0.f;
};
