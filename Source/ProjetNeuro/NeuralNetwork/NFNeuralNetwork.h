// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "NeuralNetwork/NFActivation.h"
#include "NFNeuralNetwork.generated.h"

struct FNFDataset;

/** Configuration d'une couche cachée, telle que le joueur la règle dans le labo. */
USTRUCT(BlueprintType)
struct PROJETNEURO_API FNFLayerConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge", meta = (ClampMin = "1", ClampMax = "8"))
	int32 NumNeurons = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge")
	ENFActivation Activation = ENFActivation::Tanh;

	FNFLayerConfig() = default;
	FNFLayerConfig(int32 InNumNeurons, ENFActivation InActivation)
		: NumNeurons(InNumNeurons), Activation(InActivation) {}
};

/** Fonction de perte utilisée pour l'entraînement. */
UENUM(BlueprintType)
enum class ENFLoss : uint8
{
	/** Erreur quadratique : 1/2 * somme (a - y)² */
	MSE					UMETA(DisplayName = "MSE"),
	/** Entropie croisée binaire, uniquement avec une sortie Sigmoid */
	BinaryCrossEntropy	UMETA(DisplayName = "Binary Cross-Entropy"),
};

/** Résultat de l'évaluation d'un réseau sur un dataset. */
USTRUCT(BlueprintType)
struct PROJETNEURO_API FNFEvaluation
{
	GENERATED_BODY()

	/** Perte moyenne par exemple */
	UPROPERTY(BlueprintReadOnly, Category = "Neuron Forge")
	float Loss = 0.f;

	/** Proportion d'exemples correctement classés, entre 0 et 1 */
	UPROPERTY(BlueprintReadOnly, Category = "Neuron Forge")
	float Accuracy = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Neuron Forge")
	int32 NumCorrect = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Neuron Forge")
	int32 NumSamples = 0;
};

/**
 * Couche dense (entièrement connectée) : a = f(W·x + b).
 * Les poids sont stockés ligne par ligne : Weights[o * NumInputs + i] relie l'entrée i au neurone o.
 */
struct PROJETNEURO_API FNFDenseLayer
{
	int32 NumInputs = 0;
	int32 NumOutputs = 0;
	ENFActivation Activation = ENFActivation::Linear;

	TArray<float> Weights;
	TArray<float> Biases;

	// Valeurs calculées lors de la dernière propagation avant
	TArray<float> Z;		// pré-activations W·x + b
	TArray<float> A;		// activations f(z)

	// Valeurs calculées lors de la dernière rétropropagation
	TArray<float> Delta;	// dL/dz
	TArray<float> WeightGrads;
	TArray<float> BiasGrads;

	float GetWeight(int32 Output, int32 Input) const { return Weights[Output * NumInputs + Input]; }
	float& GetWeight(int32 Output, int32 Input) { return Weights[Output * NumInputs + Input]; }
};

/**
 * Perceptron multicouche en C++ pur, indépendant du gameplay.
 *
 * Entraînement par descente de gradient stochastique (un exemple à la fois,
 * ordre mélangé à chaque epoch). La perte est choisie automatiquement :
 * entropie croisée binaire si la sortie est Sigmoid, MSE sinon.
 */
class PROJETNEURO_API FNFNeuralNetwork
{
public:
	/**
	 * Construit le réseau et initialise ses poids.
	 * @param InNumInputs		taille de la couche d'entrée
	 * @param HiddenLayers		couches cachées (peut être vide)
	 * @param InNumOutputs		nombre de neurones de sortie
	 * @param OutputActivation	activation de la couche de sortie
	 * @param Seed				graine de l'initialisation (reproductibilité)
	 */
	void Build(int32 InNumInputs, const TArray<FNFLayerConfig>& HiddenLayers, int32 InNumOutputs,
		ENFActivation OutputActivation = ENFActivation::Sigmoid, int32 Seed = 42);

	/** Réinitialise les poids (Xavier pour Tanh/Sigmoid/Linear, He pour ReLU) et les biais à 0. */
	void InitializeWeights(int32 Seed);

	/** Propagation avant ; renvoie les activations de la couche de sortie. */
	const TArray<float>& Forward(const float* Input);

	/** Rétropropagation pour un exemple : remplit Delta, WeightGrads et BiasGrads. Forward doit avoir été appelé. */
	void Backward(const float* Target);

	/** Applique les gradients calculés par Backward : w <- w - lr * dL/dw */
	void ApplyGradients(float LearningRate);

	/** Forward + Backward + ApplyGradients sur un exemple. Renvoie la perte avant mise à jour. */
	float TrainSample(const float* Input, const float* Target, float LearningRate);

	/** Une epoch complète sur le dataset, dans un ordre mélangé. Renvoie la perte moyenne. */
	float TrainEpoch(const FNFDataset& Dataset, float LearningRate, FRandomStream& Rng);

	/** Évalue perte et précision sans modifier les poids. */
	FNFEvaluation Evaluate(const FNFDataset& Dataset);

	/** Perte d'un exemple selon la fonction de perte courante. */
	float ComputeLoss(const float* Output, const float* Target) const;

	/** Vrai si la sortie est considérée comme correcte (seuil 0.5 en binaire, argmax sinon). */
	bool IsPredictionCorrect(const float* Output, const float* Target) const;

	// --- Accesseurs ---

	bool IsValid() const { return Layers.Num() > 0; }
	int32 GetNumInputs() const { return NumInputs; }
	int32 GetNumOutputs() const { return Layers.Num() > 0 ? Layers.Last().NumOutputs : 0; }
	int32 GetNumParameters() const;
	ENFLoss GetLoss() const { return Loss; }

	/** Couches denses : les couches cachées puis la couche de sortie. */
	const TArray<FNFDenseLayer>& GetLayers() const { return Layers; }
	TArray<FNFDenseLayer>& GetLayersMutable() { return Layers; }

	/** Dernière entrée vue par Forward (utile pour la visualisation). */
	const TArray<float>& GetLastInput() const { return LastInput; }

	/** Description lisible, par exemple "2 - 4 Tanh - 1 Sigmoid". */
	FString Describe() const;

private:
	int32 NumInputs = 0;
	ENFLoss Loss = ENFLoss::MSE;
	TArray<FNFDenseLayer> Layers;
	TArray<float> LastInput;
	TArray<int32> ShuffleOrder;
};
