// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NFGreenhousePlant.generated.h"

class UMaterialInterface;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Plante de la serre, construite avec des formes de base (pot, tige, feuilles, fleur).
 * Elle attend sous forme de jeune pousse ; quand le modèle a rendu son verdict (Reveal),
 * elle pousse et fleurit (survie prédite) ou fane et se courbe (mort prédite), en direct.
 */
UCLASS()
class PROJETNEURO_API ANFGreenhousePlant : public AActor
{
	GENERATED_BODY()

public:
	ANFGreenhousePlant();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Lance l'animation après Delay secondes : pousse et fleurit, ou fane. */
	void Reveal(bool bInPredictedAlive, float InPrediction, float Delay);

	/** Remet la plante à l'état de jeune pousse. */
	void ResetPlant();

	/** Réponse de la vraie serre pour les conditions de cette plante. */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Greenhouse")
	bool ReallySurvives() const;

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Greenhouse")
	bool IsRevealed() const { return bRevealed; }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Greenhouse")
	bool IsPredictedAlive() const { return bPredictedAlive; }

	/** Avancement de l'animation, de 0 (pousse) à 1 (terminée). */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Greenhouse")
	float GetAnimationProgress() const { return Progress; }

	/** Quantité d'eau reçue par la plante (0 à 10). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Greenhouse", meta = (ClampMin = "0", ClampMax = "10"))
	float Water = 5.f;

	/** Quantité de lumière reçue par la plante (0 à 10). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Greenhouse", meta = (ClampMin = "0", ClampMax = "10"))
	float Light = 5.f;

	/** Durée de l'animation de pousse ou de flétrissement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Greenhouse", meta = (ClampMin = "0.1"))
	float GrowDuration = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Greenhouse")
	TObjectPtr<UMaterialInterface> PlantMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Greenhouse")
	TObjectPtr<UMaterialInterface> PotMaterial;

protected:
	/** Aspect de la plante : Growth 0 = pousse, 1 = adulte ; Health 0 = fanée, 1 = en pleine santé. */
	void ApplyLook(float Growth, float Health, float Bloom);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Pot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Soil;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USceneComponent> PlantPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Stem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Leaves;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Flower;

	/** Pétales autour de la fleur (n'apparaissent que si la plante fleurit). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Petals;

private:
	bool bRevealed = false;
	bool bPredictedAlive = false;
	float Prediction = 0.5f;
	float DelayRemaining = 0.f;
	float Progress = 0.f;
	float SwayPhase = 0.f;
};
