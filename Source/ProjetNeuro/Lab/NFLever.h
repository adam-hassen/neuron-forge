// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/NFInteractable.h"
#include "NFLever.generated.h"

class ANFNetworkBuilder;
class UMaterialInterface;
class USoundBase;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Levier du taux d'apprentissage : chaque traction le fait passer au cran suivant
 * (0,01 → 0,05 → 0,1 → 0,3 → 0,5 → 1 → 2, puis retour à 0,01).
 * La poignée bascule en douceur et un cadran affiche la valeur.
 */
UCLASS()
class PROJETNEURO_API ANFLever : public AActor, public INFInteractable
{
	GENERATED_BODY()

public:
	ANFLever();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Holotable pilotée. Si vide, la première holotable du niveau est utilisée. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Neuron Forge|Lever")
	TObjectPtr<ANFNetworkBuilder> Builder;

	/** Angle de la poignée au premier et au dernier cran, en degrés. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Lever")
	float MinAngle = -55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Lever")
	float MaxAngle = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Lever")
	TObjectPtr<UMaterialInterface> BodyMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Lever")
	TObjectPtr<UMaterialInterface> HandleMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Audio")
	TObjectPtr<USoundBase> PullSound;

protected:
	virtual bool CanInteract_Implementation(AActor* InstigatorActor) const override;
	virtual void Interact_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusBegin_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusEnd_Implementation(AActor* InstigatorActor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	float GetTargetAngle() const;
	int32 GetNextIndex() const;
	void UpdateDial();
	void SetHighlighted(bool bOn);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Base;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Handle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Knob;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> Dial;

private:
	float CurrentAngle = 0.f;
};
