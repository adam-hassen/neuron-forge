// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/NFInteractable.h"
#include "NFPlantPot.generated.h"

class ANFNetworkBuilder;
class ANFPlantPot;
class UMaterialInterface;
class UPointLightComponent;
class USoundBase;
class UStaticMeshComponent;
class UTextRenderComponent;

/** Réglage piloté par une commande du pot. */
UENUM(BlueprintType)
enum class ENFPlantSetting : uint8
{
	Water	UMETA(DisplayName = "Arrosage"),
	Light	UMETA(DisplayName = "Lampe"),
};

/**
 * Commande du pot de plante (arrosage ou lampe) : chaque appui sur E
 * augmente la valeur d'un cran (0 à 10, puis retour à 0).
 */
UCLASS()
class PROJETNEURO_API ANFPlantControl : public AActor, public INFInteractable
{
	GENERATED_BODY()

public:
	ANFPlantControl();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Neuron Forge|Plant")
	TObjectPtr<ANFPlantPot> Pot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Plant")
	ENFPlantSetting Setting = ENFPlantSetting::Water;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Plant")
	TObjectPtr<UMaterialInterface> BodyMaterial;

	/** Material de la molette (paramètres EmissiveColor / Highlight). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Plant")
	TObjectPtr<UMaterialInterface> KnobMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Audio")
	TObjectPtr<USoundBase> ClickSound;

protected:
	virtual bool CanInteract_Implementation(AActor* InstigatorActor) const override;
	virtual void Interact_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusBegin_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusEnd_Implementation(AActor* InstigatorActor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	int32 GetValue() const;
	void RefreshLabel();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Base;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Knob;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> Label;

private:
	float KnobAngle = 0.f;
};

/**
 * Pot de plante de la phase de test : le joueur règle l'arrosage et la lampe,
 * le modèle entraîné prédit si la plante survivrait et la plante change à vue
 * (droite et verte, ou penchée et brune). La « vraie » règle de la serre est
 * affichée à côté pour discuter des erreurs du modèle.
 * Le pot ne s'active qu'une fois le modèle validé.
 */
UCLASS()
class PROJETNEURO_API ANFPlantPot : public AActor
{
	GENERATED_BODY()

public:
	ANFPlantPot();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Le modèle est-il validé (et donc utilisable pour le test) ? */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Plant")
	bool IsUnlocked() const;

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Plant")
	int32 GetValue(ENFPlantSetting Setting) const { return Setting == ENFPlantSetting::Water ? Water : Light; }

	/** Augmente un réglage d'un cran (0 à 10, puis retour à 0). Renvoie vrai si le pot est actif. */
	bool StepSetting(ENFPlantSetting Setting);

	/** Fixe directement l'arrosage et la lampe (tests, scénarios). */
	void SetConditions(int32 InWater, int32 InLight);

	/** Probabilité de survie prédite par le modèle (entre 0 et 1), -1 si le pot est verrouillé. */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Plant")
	float GetPrediction() const { return Prediction; }

	/** Réponse de la vraie serre pour les réglages actuels. */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Plant")
	bool WouldReallySurvive() const;

	/** Le joueur a-t-il déjà testé au moins une plante ? */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Plant")
	bool HasBeenTested() const { return bTested; }

	/** Holotable dont le modèle est testé. Si vide, la première holotable du niveau est utilisée. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Neuron Forge|Plant")
	TObjectPtr<ANFNetworkBuilder> Builder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Plant")
	TObjectPtr<UMaterialInterface> PlantMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Plant")
	TObjectPtr<UMaterialInterface> PotMaterial;

	/** Material du socle et de la lampe. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Plant")
	TObjectPtr<UMaterialInterface> PedestalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Plant", meta = (ClampMin = "0", ClampMax = "10"))
	int32 Water = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Plant", meta = (ClampMin = "0", ClampMax = "10"))
	int32 Light = 5;

protected:
	void UpdatePrediction();
	void ApplyPlantLook(float Health);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Pedestal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> PotMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Soil;

	/** Pivot de la plante, à la surface de la terre : la plante se penche autour de lui. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USceneComponent> PlantPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Stem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Leaves;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Flower;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Lamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UPointLightComponent> LampLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> Title;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> Readout;

private:
	float Prediction = -1.f;
	float DisplayedHealth = 0.5f;
	float RefreshTimer = 0.f;
	bool bTested = false;
};
