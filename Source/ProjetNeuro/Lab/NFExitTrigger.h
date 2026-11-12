// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NFExitTrigger.generated.h"

class UBoxComponent;
class UTextRenderComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNFOnLevelCompleted, APawn*, Player);

/**
 * Zone de fin de niveau placée derrière la porte du labo (trigger Box Collision).
 * Quand le joueur y entre, le niveau est terminé.
 */
UCLASS()
class PROJETNEURO_API ANFExitTrigger : public AActor
{
	GENERATED_BODY()

public:
	ANFExitTrigger();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Exit")
	bool IsCompleted() const { return bCompleted; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Exit")
	FString CompletionMessage = TEXT("Niveau terminé ! Votre modèle est validé.");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Audio")
	TObjectPtr<USoundBase> CompletionSound;

	UPROPERTY(BlueprintAssignable, Category = "Neuron Forge|Exit")
	FNFOnLevelCompleted OnLevelCompleted;

protected:
	UFUNCTION()
	void HandleTriggerBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> Label;

private:
	bool bCompleted = false;
};
