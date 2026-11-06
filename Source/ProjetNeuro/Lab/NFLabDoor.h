// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Actor.h"
#include "Training/NFTrainerComponent.h"
#include "NFLabDoor.generated.h"

class ANFNetworkBuilder;
class UCurveFloat;
class UPointLightComponent;
class USoundBase;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class ENFDoorState : uint8
{
	Closed,
	Opening,
	Open,
	Closing,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNFOnDoorStateChanged, ENFDoorState, NewState);

/**
 * Porte coulissante du labo (notion du cours : porte animée + son de validation).
 *
 * Deux battants glissent sur le côté grâce à une timeline (FTimeline + courbe).
 * La porte s'ouvre automatiquement quand l'holotable atteint la précision cible,
 * en jouant le son de validation ; un voyant passe du rouge au vert.
 */
UCLASS()
class PROJETNEURO_API ANFLabDoor : public AActor
{
	GENERATED_BODY()

public:
	ANFLabDoor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Door")
	void Open();

	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Door")
	void Close();

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Door")
	ENFDoorState GetDoorState() const { return DoorState; }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Door")
	bool IsOpen() const { return DoorState == ENFDoorState::Open; }

	/** Ouverture courante entre 0 (fermée) et 1 (ouverte). */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Door")
	float GetOpenAlpha() const { return OpenAlpha; }

	/** Holotable surveillée. Si vide, la première holotable du niveau est utilisée. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Neuron Forge|Door")
	TObjectPtr<ANFNetworkBuilder> Builder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Door")
	bool bOpenOnTrainingSuccess = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Door", meta = (ClampMin = "0.1"))
	float OpenDuration = 1.6f;

	/** Courbe d'ouverture (0 → 1 sur [0, 1]). Si vide, une courbe ease-in-out est créée. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Door")
	TObjectPtr<UCurveFloat> OpenCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Door|Layout", meta = (ClampMin = "50"))
	float DoorWidth = 240.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Door|Layout", meta = (ClampMin = "50"))
	float DoorHeight = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Door|Layout", meta = (ClampMin = "5"))
	float FrameThickness = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Door|Layout", meta = (ClampMin = "2"))
	float PanelThickness = 12.f;

	/** Son de validation joué quand le modèle réussit et que la porte s'ouvre. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Audio")
	TObjectPtr<USoundBase> ValidationSound;

	/** Son mécanique de la porte. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Audio")
	TObjectPtr<USoundBase> DoorMoveSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Door")
	FLinearColor LockedColor = FLinearColor(1.f, 0.1f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Door")
	FLinearColor UnlockedColor = FLinearColor(0.1f, 1.f, 0.25f);

	UPROPERTY(BlueprintAssignable, Category = "Neuron Forge|Door")
	FNFOnDoorStateChanged OnDoorStateChanged;

protected:
	UFUNCTION()
	void HandleTrainingStateChanged(ENFTrainingState NewState);

	UFUNCTION()
	void HandleTimelineProgress(float Value);

	UFUNCTION()
	void HandleTimelineFinished();

	/** Place cadre et battants selon DoorWidth / DoorHeight. */
	void LayoutDoor();

	void ApplyOpenAlpha(float Alpha);
	void SetDoorState(ENFDoorState NewState);
	void SetLocked(bool bLocked);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> FrameLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> FrameRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> FrameTop;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> PanelLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> PanelRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UPointLightComponent> StatusLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> StatusText;

private:
	FTimeline DoorTimeline;

	UPROPERTY(Transient)
	TObjectPtr<UCurveFloat> RuntimeCurve;

	ENFDoorState DoorState = ENFDoorState::Closed;
	float OpenAlpha = 0.f;
	FVector PanelLeftClosed = FVector::ZeroVector;
	FVector PanelRightClosed = FVector::ZeroVector;
};
