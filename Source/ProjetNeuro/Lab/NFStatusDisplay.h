// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Training/NFTrainerComponent.h"
#include "NFStatusDisplay.generated.h"

class ANFNetworkBuilder;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Écran d'état du labo (notion du cours : Text Render).
 *
 * Affiche la mission, l'architecture, le taux d'apprentissage, l'epoch,
 * la perte, la précision, les prédictions sur chaque exemple et un conseil.
 * Le texte est dans le monde 3D : il reste lisible en VR.
 */
UCLASS()
class PROJETNEURO_API ANFStatusDisplay : public AActor
{
	GENERATED_BODY()

public:
	ANFStatusDisplay();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Holotable observée. Si vide, la première holotable du niveau est utilisée. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Neuron Forge|Display")
	TObjectPtr<ANFNetworkBuilder> Builder;

	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Display")
	void Refresh();

	/** Titre de mission affiché au repos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Display")
	FString MissionTitle = TEXT("MISSION : RÉSOUDRE XOR");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Display")
	FColor IdleColor = FColor(120, 220, 255);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Display")
	FColor TrainingColor = FColor(255, 210, 80);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Display")
	FColor SuccessColor = FColor(80, 255, 120);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Display")
	FColor FailureColor = FColor(255, 90, 70);

protected:
	UFUNCTION()
	void HandleNetworkChanged();

	UFUNCTION()
	void HandleProgress(const FNFTrainingProgress& Progress);

	FString BuildBodyText() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Panneau de fond (matérial sombre). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> ScreenMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> TitleText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> BodyText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> HintText;
};
