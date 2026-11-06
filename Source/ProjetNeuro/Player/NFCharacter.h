// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NFCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UNFInteractorComponent;
class USkeletalMeshComponent;
struct FInputActionValue;

/**
 * Personnage du joueur, en vue à la première personne.
 *
 * Reprend la configuration du personnage du template First Person d'UE 5.6
 * (bras FP + corps visible en ombre, caméra sur la tête, FOV et échelle "first person")
 * et réutilise ses assets : SKM_Manny_Simple, ABP_FP_Copy, ABP_Unarmed et les Input Actions.
 * Ajoute un UNFInteractorComponent et l'action « Interagir » (E / clic gauche / manette X).
 */
UCLASS(config = Game)
class PROJETNEURO_API ANFCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ANFCharacter();

	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	UNFInteractorComponent* GetInteractor() const { return Interactor; }

protected:
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Interact();

	/** Crée l'action « Interagir » et son mapping si aucun asset n'a été assigné. */
	void EnsureInteractionInput();

	/** Ajoute les Input Mapping Contexts au joueur local. */
	void AddMappingContexts();

	// --- Composants ---

	/** Bras visibles uniquement par le joueur. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UNFInteractorComponent> Interactor;

	// --- Entrées (Enhanced Input) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Regard à la manette */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Input")
	TObjectPtr<UInputAction> LookAction;

	/** Regard à la souris */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Input")
	TObjectPtr<UInputAction> MouseLookAction;

	/** Si vide, une action « Interagir » est créée au lancement (E, clic gauche, manette X). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Input")
	TObjectPtr<UInputAction> InteractAction;

	/** Mapping de l'action Interagir ; créé au lancement si InteractAction est vide. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Input")
	TObjectPtr<UInputMappingContext> InteractionMappingContext;
};
