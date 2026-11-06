// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NFInteractable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UNFInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface commune à tout objet avec lequel le joueur peut interagir.
 *
 * L'objet ne sait pas *comment* on interagit avec lui : sur PC, c'est le
 * UNFInteractorComponent (visée caméra + touche) qui appelle ces fonctions ;
 * en VR, un composant de main ou de contrôleur appellera exactement les mêmes.
 *
 * Appel depuis le C++ : INFInteractable::Execute_Interact(Objet, Instigateur)
 */
class PROJETNEURO_API INFInteractable
{
	GENERATED_BODY()

public:
	/** L'objet accepte-t-il une interaction maintenant ? (ex. bouton désactivé pendant l'entraînement) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Neuron Forge|Interaction")
	bool CanInteract(AActor* InstigatorActor) const;

	/** Déclenche l'interaction (appui, activation...). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Neuron Forge|Interaction")
	void Interact(AActor* InstigatorActor);

	/** L'objet devient la cible visée / touchée. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Neuron Forge|Interaction")
	void OnFocusBegin(AActor* InstigatorActor);

	/** L'objet n'est plus la cible. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Neuron Forge|Interaction")
	void OnFocusEnd(AActor* InstigatorActor);

	/** Texte d'aide affiché quand l'objet est visé, par exemple « Ajouter un neurone ». */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Neuron Forge|Interaction")
	FText GetInteractionPrompt() const;

protected:
	virtual bool CanInteract_Implementation(AActor* InstigatorActor) const { return true; }
	virtual void Interact_Implementation(AActor* InstigatorActor) {}
	virtual void OnFocusBegin_Implementation(AActor* InstigatorActor) {}
	virtual void OnFocusEnd_Implementation(AActor* InstigatorActor) {}
	virtual FText GetInteractionPrompt_Implementation() const { return NSLOCTEXT("NeuronForge", "DefaultPrompt", "Interagir"); }
};
