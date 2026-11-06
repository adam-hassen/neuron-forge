// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "NFActivation.generated.h"

/** Fonctions d'activation disponibles pour une couche du réseau. */
UENUM(BlueprintType)
enum class ENFActivation : uint8
{
	Linear	UMETA(DisplayName = "Linear"),
	ReLU	UMETA(DisplayName = "ReLU"),
	Sigmoid	UMETA(DisplayName = "Sigmoid"),
	Tanh	UMETA(DisplayName = "Tanh"),

	Count	UMETA(Hidden)
};

namespace NFActivation
{
	/** f(z) */
	PROJETNEURO_API float Apply(ENFActivation Activation, float Z);

	/**
	 * f'(z), exprimée à partir de z et de a = f(z) pour éviter de recalculer
	 * l'exponentielle (sigmoid : a(1-a), tanh : 1-a²).
	 */
	PROJETNEURO_API float Derivative(ENFActivation Activation, float Z, float A);

	/** Nom court affiché dans le jeu ("ReLU", "Tanh"...). */
	PROJETNEURO_API FString ToString(ENFActivation Activation);

	/** Activation suivante dans le cycle Linear -> ReLU -> Sigmoid -> Tanh -> Linear. */
	PROJETNEURO_API ENFActivation Next(ENFActivation Activation);

	/** Couleur associée à l'activation (utilisée pour les neurones émissifs). */
	PROJETNEURO_API FLinearColor GetColor(ENFActivation Activation);
}
