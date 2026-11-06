// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "Interaction/NFInteractableActor.h"
#include "Lab/NFNetworkBuilder.h"
#include "NFConfigButton.generated.h"

class UTextRenderComponent;
class USoundBase;

/**
 * Bouton mural de configuration du réseau : il applique une ENFConfigAction
 * sur l'holotable (ajouter un neurone à la couche 2, changer d'activation...).
 *
 * Le bouton n'est actif que si l'action est possible : il est grisé si la couche
 * n'existe pas, si une limite est atteinte ou pendant l'entraînement.
 */
UCLASS()
class PROJETNEURO_API ANFConfigButton : public ANFInteractableActor
{
	GENERATED_BODY()

public:
	ANFConfigButton();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Holotable pilotée. Si vide, la première holotable du niveau est utilisée. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Neuron Forge|Config")
	TObjectPtr<ANFNetworkBuilder> Builder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Config")
	ENFConfigAction Action = ENFConfigAction::AddNeuron;

	/** Couche cachée concernée (0 = première), pour les actions de couche. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Config", meta = (ClampMin = "0", ClampMax = "2"))
	int32 LayerIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Audio")
	TObjectPtr<USoundBase> ClickSound;

	/** Enfoncement du bouton lors de l'appui, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Config")
	float PressDepth = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Config")
	FColor EnabledLabelColor = FColor(220, 245, 255);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Config")
	FColor DisabledLabelColor = FColor(90, 90, 90);

	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Config")
	void RefreshButton();

protected:
	virtual bool CanInteract_Implementation(AActor* InstigatorActor) const override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual void HandleInteract(AActor* InstigatorActor) override;
	virtual void RefreshVisuals() override;

	UFUNCTION()
	void HandleNetworkChanged();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> Label;

private:
	float PressAlpha = 0.f;
	FVector MeshRestLocation = FVector::ZeroVector;
};
