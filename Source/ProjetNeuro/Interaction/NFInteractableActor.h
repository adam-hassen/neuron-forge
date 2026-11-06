// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/NFInteractable.h"
#include "NFInteractableActor.generated.h"

class UStaticMeshComponent;
class ANFInteractableActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNFOnActorInteracted, ANFInteractableActor*, InteractedActor, AActor*, InstigatorActor);

/**
 * Acteur interactif de base : un mesh, un texte d'aide, une surbrillance
 * quand il est visé et un événement OnInteracted.
 *
 * Les objets du labo (boutons de configuration...) en héritent et redéfinissent HandleInteract().
 * La surbrillance passe par le paramètre scalaire "Highlight" des materials du mesh
 * et par le Custom Depth (contour possible en post-process).
 */
UCLASS(Blueprintable)
class PROJETNEURO_API ANFInteractableActor : public AActor, public INFInteractable
{
	GENERATED_BODY()

public:
	ANFInteractableActor();

	/** Nom du paramètre scalaire de material utilisé pour la surbrillance. */
	static const FName HighlightParameterName;

	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Interaction")
	void SetInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Interaction")
	bool IsInteractionEnabled() const { return bInteractionEnabled; }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Interaction")
	bool IsHighlighted() const { return bHighlighted; }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Interaction")
	int32 GetInteractionCount() const { return InteractionCount; }

	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Interaction")
	void SetPrompt(const FText& InPrompt) { Prompt = InPrompt; }

	UStaticMeshComponent* GetMesh() const { return Mesh; }

	UPROPERTY(BlueprintAssignable, Category = "Neuron Forge|Interaction")
	FNFOnActorInteracted OnInteracted;

protected:
	// --- INFInteractable ---
	virtual bool CanInteract_Implementation(AActor* InstigatorActor) const override;
	virtual void Interact_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusBegin_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusEnd_Implementation(AActor* InstigatorActor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	/** Comportement propre à chaque objet ; appelé par Interact quand l'interaction est acceptée. */
	virtual void HandleInteract(AActor* InstigatorActor) {}

	/** Met à jour l'apparence (surbrillance, état désactivé). */
	virtual void RefreshVisuals();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Texte affiché quand l'objet est visé. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Interaction")
	FText Prompt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Interaction")
	bool bInteractionEnabled = true;

	/** Valeur du paramètre "Highlight" quand l'objet est visé. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Interaction")
	float HighlightIntensity = 1.f;

private:
	bool bHighlighted = false;
	int32 InteractionCount = 0;
};
