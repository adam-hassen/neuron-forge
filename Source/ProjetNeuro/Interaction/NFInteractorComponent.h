// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NFInteractorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNFOnFocusChanged, AActor*, NewFocus, AActor*, OldFocus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNFOnInteracted, AActor*, Target);

/**
 * Composant qui permet à son propriétaire d'interagir avec les INFInteractable.
 *
 * À chaque frame, il lance un rayon depuis une source de visée et met à jour
 * l'objet ciblé (focus). TryInteract() interagit avec cet objet.
 *
 * Source de visée :
 *  - par défaut, le point de vue du contrôleur (caméra du joueur, PC) ;
 *  - si TraceSource est renseigné, la position et la direction de ce composant
 *    (par exemple un contrôleur de mouvement VR : pointeur laser).
 * InteractWith() permet aussi une interaction directe, sans rayon (main VR qui touche l'objet).
 */
UCLASS(ClassGroup = (NeuronForge), meta = (BlueprintSpawnableComponent))
class PROJETNEURO_API UNFInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNFInteractorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Relance le rayon et met à jour l'objet ciblé. Appelé automatiquement à chaque frame si bAutoUpdateFocus. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Interaction")
	void UpdateFocus();

	/** Interagit avec l'objet ciblé. Renvoie vrai si l'interaction a eu lieu. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Interaction")
	bool TryInteract();

	/** Interagit directement avec un acteur, sans rayon (contact d'une main VR, scripts...). */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Interaction")
	bool InteractWith(AActor* Target);

	/** Premier acteur INFInteractable touché par un rayon partant de Start dans la direction Direction. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Interaction")
	AActor* FindInteractableAlong(const FVector& Start, const FVector& Direction) const;

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	/** Vrai si l'objet ciblé accepte une interaction maintenant. */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Interaction")
	bool CanInteractWithFocus() const;

	/** Remplace la visée caméra par celle d'un composant (contrôleur VR). nullptr pour revenir à la caméra. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Interaction")
	void SetTraceSource(USceneComponent* InTraceSource) { TraceSource = InTraceSource; }

	/** Distance maximale d'interaction, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Interaction", meta = (ClampMin = "10"))
	float InteractionDistance = 350.f;

	/** Rayon du balayage (0 = rayon fin). Un petit rayon rend la visée plus tolérante. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Interaction", meta = (ClampMin = "0"))
	float TraceRadius = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Met à jour le focus à chaque frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Interaction")
	bool bAutoUpdateFocus = true;

	UPROPERTY(BlueprintAssignable, Category = "Neuron Forge|Interaction")
	FNFOnFocusChanged OnFocusChanged;

	UPROPERTY(BlueprintAssignable, Category = "Neuron Forge|Interaction")
	FNFOnInteracted OnInteracted;

protected:
	/** Point de départ et direction de la visée. Faux si aucune source n'est disponible. */
	bool GetAimRay(FVector& OutStart, FVector& OutDirection) const;

	void SetFocusedActor(AActor* NewFocus);

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Neuron Forge|Interaction")
	TObjectPtr<USceneComponent> TraceSource;

	TWeakObjectPtr<AActor> FocusedActor;
};
