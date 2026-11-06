// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Training/NFTrainerComponent.h"
#include "NFLabAmbience.generated.h"

class ANFNetworkBuilder;
class ULightComponent;
class UMaterialParameterCollection;

/**
 * Ambiance du laboratoire, synchronisée avec l'entraînement :
 *  - au repos : cyan calme ;
 *  - entraînement : orange pulsé ;
 *  - réussite : flash vert puis vert apaisé ;
 *  - échec : flash rouge puis retour au calme.
 *
 * La couleur est envoyée à la Material Parameter Collection (paramètres "MoodColor" et
 * "MoodIntensity"), lue par les liserés lumineux, la grille du sol et les hologrammes,
 * et teinte les lumières portant le tag LightTag.
 */
UCLASS()
class PROJETNEURO_API ANFLabAmbience : public AActor
{
	GENERATED_BODY()

public:
	ANFLabAmbience();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Couleur d'ambiance actuelle. */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Ambience")
	FLinearColor GetCurrentColor() const { return CurrentColor; }

	/** Holotable suivie. Si vide, la première holotable du niveau est utilisée. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Neuron Forge|Ambience")
	TObjectPtr<ANFNetworkBuilder> Builder;

	/** Collection lue par les materials du décor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Ambience")
	TObjectPtr<UMaterialParameterCollection> MoodCollection;

	/** Les lumières portant ce tag suivent l'ambiance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Ambience")
	FName LightTag = TEXT("NFMood");

	/** Part de la couleur d'ambiance dans la teinte des lumières (0 = aucune, 1 = totale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Ambience", meta = (ClampMin = "0", ClampMax = "1"))
	float LightTint = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Ambience")
	FLinearColor IdleColor = FLinearColor(0.10f, 0.65f, 1.00f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Ambience")
	FLinearColor TrainingColor = FLinearColor(1.00f, 0.55f, 0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Ambience")
	FLinearColor SuccessColor = FLinearColor(0.15f, 1.00f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Ambience")
	FLinearColor FailureColor = FLinearColor(1.00f, 0.12f, 0.08f);

	/** Vitesse de transition entre deux couleurs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Ambience", meta = (ClampMin = "0.1"))
	float BlendSpeed = 3.f;

protected:
	UFUNCTION()
	void HandleTrainingStateChanged(ENFTrainingState NewState);

	void ApplyMood(const FLinearColor& Color, float Intensity);

private:
	struct FLightState
	{
		TWeakObjectPtr<ULightComponent> Light;
		FLinearColor BaseColor;
		float BaseIntensity = 0.f;
	};

	TArray<FLightState> Lights;
	ENFTrainingState State = ENFTrainingState::Idle;
	FLinearColor CurrentColor;
	float Flash = 0.f;
	float FailureTimer = 0.f;
};
