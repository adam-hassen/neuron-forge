// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NFGrabberComponent.generated.h"

class ANFPiece;
class UNFSocketComponent;

/**
 * Permet à son propriétaire d'attraper, porter et poser les pièces de la Forge.
 *
 * La pièce tenue flotte devant le point de vue (caméra sur PC). Comme pour
 * UNFInteractorComponent, SetHoldSource() permet de la tenir dans une main VR.
 */
UCLASS(ClassGroup = (NeuronForge), meta = (BlueprintSpawnableComponent))
class PROJETNEURO_API UNFGrabberComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNFGrabberComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Attrape la pièce (si les mains sont libres et que la pièce l'accepte). */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Grab")
	bool TryGrab(ANFPiece* Piece);

	/** Pose la pièce tenue : elle s'emboîte si un emplacement compatible est à portée. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Grab")
	void Drop();

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Grab")
	bool IsHolding() const { return HeldPiece.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Grab")
	ANFPiece* GetHeldPiece() const { return HeldPiece.Get(); }

	/** Emplacement où la pièce tenue s'emboîterait si on la posait maintenant. */
	UNFSocketComponent* GetSnapTarget() const;

	/** Texte d'aide pendant le portage : « Insérer dans la fente 2 » ou « Poser ». */
	FText GetDropPrompt() const;

	/** Tenir la pièce depuis un composant (main VR) plutôt que devant la caméra. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Grab")
	void SetHoldSource(USceneComponent* InHoldSource) { HoldSource = InHoldSource; }

	/** Distance de la pièce tenue devant la caméra, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Grab")
	float HoldDistance = 90.f;

	/** Décalage vertical de la pièce tenue (négatif = plus bas que le regard). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Grab")
	float HoldHeightOffset = -22.f;

protected:
	bool GetHoldTransform(FVector& OutLocation, FRotator& OutRotation) const;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> HoldSource;

	TWeakObjectPtr<ANFPiece> HeldPiece;
};
