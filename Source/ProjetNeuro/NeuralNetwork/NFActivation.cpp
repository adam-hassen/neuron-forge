// Neuron Forge - ENSIM

#include "NeuralNetwork/NFActivation.h"

namespace NFActivation
{
	float Apply(ENFActivation Activation, float Z)
	{
		switch (Activation)
		{
		case ENFActivation::ReLU:
			return Z > 0.f ? Z : 0.f;
		case ENFActivation::Sigmoid:
			// Formulation stable numériquement pour les grandes valeurs de |z|
			if (Z >= 0.f)
			{
				return 1.f / (1.f + FMath::Exp(-Z));
			}
			else
			{
				const float E = FMath::Exp(Z);
				return E / (1.f + E);
			}
		case ENFActivation::Tanh:
			return FMath::Tanh(Z);
		case ENFActivation::Linear:
		default:
			return Z;
		}
	}

	float Derivative(ENFActivation Activation, float Z, float A)
	{
		switch (Activation)
		{
		case ENFActivation::ReLU:
			return Z > 0.f ? 1.f : 0.f;
		case ENFActivation::Sigmoid:
			return A * (1.f - A);
		case ENFActivation::Tanh:
			return 1.f - A * A;
		case ENFActivation::Linear:
		default:
			return 1.f;
		}
	}

	FString ToString(ENFActivation Activation)
	{
		switch (Activation)
		{
		case ENFActivation::ReLU:		return TEXT("ReLU");
		case ENFActivation::Sigmoid:	return TEXT("Sigmoid");
		case ENFActivation::Tanh:		return TEXT("Tanh");
		case ENFActivation::Linear:
		default:						return TEXT("Linear");
		}
	}

	ENFActivation Next(ENFActivation Activation)
	{
		const uint8 Count = static_cast<uint8>(ENFActivation::Count);
		return static_cast<ENFActivation>((static_cast<uint8>(Activation) + 1) % Count);
	}

	FLinearColor GetColor(ENFActivation Activation)
	{
		switch (Activation)
		{
		case ENFActivation::ReLU:		return FLinearColor(1.0f, 0.45f, 0.05f);	// orange
		case ENFActivation::Sigmoid:	return FLinearColor(0.1f, 1.0f, 0.3f);	// vert
		case ENFActivation::Tanh:		return FLinearColor(0.75f, 0.2f, 1.0f);	// violet
		case ENFActivation::Linear:
		default:						return FLinearColor(0.2f, 0.6f, 1.0f);	// bleu
		}
	}
}
