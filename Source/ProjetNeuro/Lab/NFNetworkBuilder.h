// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NeuralNetwork/NFNeuralNetwork.h"
#include "Training/NFTrainerComponent.h"
#include "NFNetworkBuilder.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class USoundBase;

/** Actions de configuration proposées par les boutons du labo. */
UENUM(BlueprintType)
enum class ENFConfigAction : uint8
{
	AddLayer				UMETA(DisplayName = "Ajouter une couche"),
	RemoveLayer				UMETA(DisplayName = "Retirer une couche"),
	AddNeuron				UMETA(DisplayName = "Ajouter un neurone"),
	RemoveNeuron			UMETA(DisplayName = "Retirer un neurone"),
	NextActivation			UMETA(DisplayName = "Changer d'activation"),
	IncreaseLearningRate	UMETA(DisplayName = "Augmenter le taux d'apprentissage"),
	DecreaseLearningRate	UMETA(DisplayName = "Diminuer le taux d'apprentissage"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNFOnNetworkChanged);

/**
 * Holotable du labo : le joueur y assemble son réseau de neurones et le voit en 3D.
 *
 *  - Configuration : couches cachées (nombre, neurones, activation) et taux d'apprentissage,
 *    modifiés via ApplyAction() par les boutons (ou tout autre mode d'interaction).
 *  - Entraînement : délégué à un UNFTrainerComponent.
 *  - Visualisation : un neurone = une sphère, un poids = un cylindre, en instances.
 *    Données par instance (PerInstanceCustomData 0..3) : couleur RVB + intensité émissive,
 *    lues par le material M_Neuron. Couleur du neurone = son activation, intensité = sa valeur ;
 *    couleur du poids = son signe, épaisseur = son amplitude.
 *
 * Les colonnes vont de l'entrée (à gauche vu de face, côté +Y) vers la sortie (côté -Y) ;
 * le joueur regarde l'holotable depuis son axe +X.
 */
UCLASS()
class PROJETNEURO_API ANFNetworkBuilder : public AActor
{
	GENERATED_BODY()

public:
	ANFNetworkBuilder();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// --- Configuration ---

	/** L'action est-elle possible maintenant ? LayerIndex : indice de couche cachée (0 = première). */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Network")
	bool CanApplyAction(ENFConfigAction Action, int32 LayerIndex = 0) const;

	/** Applique l'action. Renvoie vrai si la configuration a changé. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Network")
	bool ApplyAction(ENFConfigAction Action, int32 LayerIndex = 0);

	/** Texte d'aide décrivant l'action, par exemple « Couche 1 : ajouter un neurone (4 → 5) ». */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Network")
	FText DescribeAction(ENFConfigAction Action, int32 LayerIndex = 0) const;

	/** Libellé court pour un bouton : « + », « − », « Tanh »... */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Network")
	FString GetActionLabel(ENFConfigAction Action, int32 LayerIndex = 0) const;

	/**
	 * Données d'entraînement choisies par le joueur (salle des données) : le modèle apprend sur Train
	 * et la réussite se juge sur Validation. Interrompt un entraînement en cours.
	 */
	void SetTrainingData(const FNFDataset& Train, const FNFDataset& Validation);

	/** Vrai si les données viennent de la salle des données (et non du jeu par défaut). */
	bool HasCustomTrainingData() const { return bCustomTrainingData; }

	/** Remplace toute la configuration (tests, niveaux scénarisés). */
	void SetConfiguration(const TArray<FNFLayerConfig>& InHiddenLayers, int32 InLearningRateIndex);

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Network")
	bool CanEdit() const;

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Network")
	int32 GetNumHiddenLayers() const { return HiddenLayers.Num(); }

	const TArray<FNFLayerConfig>& GetHiddenLayers() const { return HiddenLayers; }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Network")
	float GetLearningRate() const;

	/** Description de l'architecture, par exemple « 2 - 4 Tanh - 1 Sigmoid ». */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Network")
	FString DescribeNetwork() const;

	/** Conseil pédagogique selon la configuration et le résultat du dernier entraînement. */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Network")
	FString GetHintText() const;

	// --- Entraînement ---

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Training")
	bool CanStartTraining() const;

	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Training")
	bool StartTraining();

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Training")
	ENFTrainingState GetTrainingState() const;

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Training")
	UNFTrainerComponent* GetTrainer() const { return Trainer; }

	/** Nombre d'entraînements lancés depuis le début du niveau. */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Training")
	int32 GetTrainingAttempts() const { return TrainingAttempts; }

	/** Le modèle a-t-il déjà atteint la cible pendant ce niveau ? */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Training")
	bool HasSucceeded() const { return bHasSucceeded; }

	/** Architecture, taux d'apprentissage et progression du modèle validé. */
	const FString& GetSuccessArchitecture() const { return SuccessArchitecture; }
	float GetSuccessLearningRate() const { return SuccessLearningRate; }
	const FNFTrainingProgress& GetSuccessProgress() const { return SuccessProgress; }

	/** Graine du découpage par défaut (sans salle des données) : 70 % entraînement, 30 % validation. */
	static constexpr int32 DefaultSplitSeed = 7;
	static constexpr float DefaultTrainFraction = 0.7f;

	/** Déclenché à chaque modification de la configuration ou changement d'état d'entraînement. */
	UPROPERTY(BlueprintAssignable, Category = "Neuron Forge|Network")
	FNFOnNetworkChanged OnNetworkChanged;

	// --- Réglages ---

	/** Problème à résoudre : XOR (tests) ou la serre (niveau jouable). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Network")
	ENFProblem Problem = ENFProblem::XOR;

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Network")
	bool IsGreenhouse() const { return Problem == ENFProblem::Greenhouse; }

	/** Configuration courante des couches cachées (configuration de départ dans l'éditeur). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Network")
	TArray<FNFLayerConfig> HiddenLayers;

	/** Couche ajoutée par « Ajouter une couche ». Linear par défaut : le joueur doit découvrir la non-linéarité. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Network")
	FNFLayerConfig NewLayerTemplate = FNFLayerConfig(4, ENFActivation::Linear);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Network", meta = (ClampMin = "0", ClampMax = "3"))
	int32 MaxHiddenLayers = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Network", meta = (ClampMin = "1"))
	int32 MinNeuronsPerLayer = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Network", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxNeuronsPerLayer = 8;

	/** Valeurs possibles du taux d'apprentissage, parcourues par les boutons. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Network")
	TArray<float> LearningRateSteps;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Network")
	int32 LearningRateIndex = 4;

	// --- Visualisation ---

	/** Centre du réseau affiché, relatif à l'acteur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization")
	FVector VisualOrigin = FVector(0.f, 0.f, 180.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization")
	float LayerSpacing = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization")
	float NeuronSpacing = 18.f;

	/** Diamètre d'un neurone, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization")
	float NeuronDiameter = 11.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization")
	float MinConnectionThickness = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization")
	float MaxConnectionThickness = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization")
	FLinearColor InputNeuronColor = FLinearColor(0.9f, 0.9f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization")
	FLinearColor PositiveWeightColor = FLinearColor(0.1f, 0.55f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization")
	FLinearColor NegativeWeightColor = FLinearColor(1.f, 0.25f, 0.1f);

	/** Impulsions lumineuses qui parcourent les poids : allers par seconde au repos et pendant l'entraînement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization", meta = (ClampMin = "0"))
	float PulseSpeedIdle = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization", meta = (ClampMin = "0"))
	float PulseSpeedTraining = 1.6f;

	/** Diamètre d'une impulsion, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization", meta = (ClampMin = "0"))
	float PulseDiameter = 3.5f;

	/** Durée d'affichage de chaque exemple du dataset sur l'holotable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization", meta = (ClampMin = "0.1"))
	float SampleDisplayDuration = 1.2f;

	/** Fréquence de rafraîchissement de la visualisation pendant l'entraînement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Visualization", meta = (ClampMin = "1"))
	float VisualUpdateRate = 30.f;

	// --- Sons ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Audio")
	TObjectPtr<USoundBase> TrainingStartSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Audio")
	TObjectPtr<USoundBase> TrainingFailedSound;

protected:
	UFUNCTION()
	void HandleTrainerStateChanged(ENFTrainingState NewState);

	/** Envoie la configuration au trainer, reconstruit la visualisation et notifie. */
	void ApplyConfigToTrainer();

	/** Charge le dataset et les réglages du problème si ce n'est pas déjà fait. */
	void EnsureProblemLoaded();

	/** Recrée les instances de neurones et de poids pour l'architecture courante. */
	void RebuildVisualization();

	/** Met à jour couleurs, intensités et épaisseurs selon les poids et activations actuels. */
	void UpdateVisualization();

	void UpdateLabels();

	/** Fait avancer les impulsions le long des poids (appelé à chaque frame). */
	void UpdatePulses(float DeltaSeconds);

	/** Taille de chaque colonne : entrée, couches cachées, sortie. */
	void GetColumnSizes(TArray<int32>& OutSizes) const;

	FVector GetNeuronLocation(int32 Column, int32 Index, int32 NumColumns, int32 ColumnSize) const;

	void ShowPlayerMessage(const FString& Text, const FLinearColor& Color) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Socle de l'holotable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> TableMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UInstancedStaticMeshComponent> NeuronInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UInstancedStaticMeshComponent> ConnectionInstances;

	/** Impulsions lumineuses qui circulent le long des poids, de l'entrée vers la sortie. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UInstancedStaticMeshComponent> PulseInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UNFTrainerComponent> Trainer;

	/** Libellé sous chaque colonne (entrée, couches cachées, sortie). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TArray<TObjectPtr<UTextRenderComponent>> ColumnLabels;

	/** Exemple actuellement propagé dans le réseau affiché. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> SampleLabel;

private:
	int32 TrainingAttempts = 0;
	bool bHasSucceeded = false;
	bool bCustomTrainingData = false;
	FString SuccessArchitecture;
	float SuccessLearningRate = 0.f;
	FNFTrainingProgress SuccessProgress;

	TArray<FVector> PulseStarts;
	TArray<FVector> PulseEnds;
	TArray<float> PulseOffsets;
	float PulsePhase = 0.f;
	float PulseSpeed = 0.35f;

	int32 DisplayedSample = 0;
	float SampleTimer = 0.f;
	float VisualTimer = 0.f;
};
