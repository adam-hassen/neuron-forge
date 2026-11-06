// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/NFInteractable.h"
#include "Lab/NFSocketComponent.h"
#include "NeuralNetwork/NFActivation.h"
#include "NFPiece.generated.h"

class ANFNetworkBuilder;
class UMaterialInterface;
class UNFGrabberComponent;
class UStaticMeshComponent;

/**
 * Pièce physique du puzzle de la Forge : le joueur l'attrape (INFInteractable → grabber),
 * la porte et la relâche. Relâchée près d'un emplacement compatible, elle s'y emboîte ;
 * sinon elle tombe (physique). Les pièces emboîtées sont verrouillées pendant l'entraînement.
 */
UCLASS()
class PROJETNEURO_API ANFPiece : public AActor, public INFInteractable
{
	GENERATED_BODY()

public:
	ANFPiece();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// --- Prise en main (appelé par UNFGrabberComponent) ---

	void OnGrabbed(UNFGrabberComponent* Grabber);
	void OnReleased();
	void UpdateHeldTransform(const FVector& TargetLocation, const FRotator& TargetRotation, float DeltaSeconds);

	/** Emplacement libre et compatible le plus proche, à portée d'emboîtement (ou nullptr). */
	UNFSocketComponent* FindBestSocket() const;

	/** Emboîte la pièce dans l'emplacement (sans contrôle de distance). */
	void DockTo(UNFSocketComponent* Socket);

	/** Retire la pièce de son emplacement. */
	void Undock();

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Piece")
	bool IsHeld() const { return bHeld; }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Piece")
	UNFSocketComponent* GetSocket() const { return Socket.Get(); }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Piece")
	bool IsDocked() const { return Socket.IsValid(); }

	/** Vrai pendant l'entraînement pour les pièces emboîtées : l'assemblage est figé. */
	bool IsLockedByTraining() const;

	/** Nom affiché : « module de couche », « orbe-neurone », « cristal Tanh »... */
	FText GetDisplayName() const;

	UStaticMeshComponent* GetMesh() const { return Mesh; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Piece")
	ENFPieceType PieceType = ENFPieceType::NeuronOrb;

	/** Activation portée par un cristal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Piece")
	ENFActivation Activation = ENFActivation::Tanh;

	/** Material du corps de la pièce (paramètres BaseColor / EmissiveColor / Highlight). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Piece")
	TObjectPtr<UMaterialInterface> BodyMaterial;

	/** Couleur lumineuse de la pièce (les cristaux prennent la couleur de leur activation). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Piece")
	FLinearColor GlowColor = FLinearColor(0.3f, 0.85f, 1.f);

	/** En dessous de cette altitude, la pièce perdue revient à sa place d'origine. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Piece")
	float RecallBelowZ = -200.f;

protected:
	// --- INFInteractable ---
	virtual bool CanInteract_Implementation(AActor* InstigatorActor) const override;
	virtual void Interact_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusBegin_Implementation(AActor* InstigatorActor) override;
	virtual void OnFocusEnd_Implementation(AActor* InstigatorActor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	void ApplyAppearance();
	void SetFree();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	ANFNetworkBuilder* FindBuilder() const;

	TWeakObjectPtr<UNFSocketComponent> Socket;
	mutable TWeakObjectPtr<ANFNetworkBuilder> CachedBuilder;
	FTransform HomeTransform;
	bool bHeld = false;
	bool bFocused = false;
};
