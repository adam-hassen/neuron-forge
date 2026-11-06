// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NFFloorButton.generated.h"

class ANFNetworkBuilder;
class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNFOnFloorButtonPressed, AActor*, PressingActor);

/**
 * Bouton au sol (plaque de pression) qui lance l'entraînement.
 *
 * Notion du cours : trigger Box Collision. Quand le joueur entre dans la boîte,
 * la plaque s'enfonce, s'illumine et l'holotable démarre l'entraînement.
 * Fonctionne tel quel en VR : il suffit de marcher dessus.
 */
UCLASS()
class PROJETNEURO_API ANFFloorButton : public AActor
{
	GENERATED_BODY()

public:
	ANFFloorButton();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Floor Button")
	bool IsPressed() const { return OverlappingPawns > 0; }

	/** Holotable à entraîner. Si vide, la première holotable du niveau est utilisée. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Neuron Forge|Floor Button")
	TObjectPtr<ANFNetworkBuilder> Builder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Floor Button")
	float PressDepth = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Floor Button")
	float PressSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Audio")
	TObjectPtr<USoundBase> PressSound;

	UPROPERTY(BlueprintAssignable, Category = "Neuron Forge|Floor Button")
	FNFOnFloorButtonPressed OnPressed;

protected:
	UFUNCTION()
	void HandleTriggerBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleTriggerEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	/** Appui effectif : lance l'entraînement ou explique pourquoi c'est impossible. */
	void Press(APawn* Pawn);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	/** Plaque mobile, matérial avec paramètre "Highlight". */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> PlateMesh;

	/** Zone de détection du joueur. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> Label;

private:
	int32 OverlappingPawns = 0;
	float PressAlpha = 0.f;
	FVector PlateRestLocation = FVector::ZeroVector;
};
