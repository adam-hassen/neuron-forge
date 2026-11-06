// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NFAnimatedProp.generated.h"

class ANFNetworkBuilder;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

/**
 * Élément de décor animé : rotation continue, flottement vertical
 * et/ou couronne d'objets en orbite. Peut accélérer pendant l'entraînement.
 * Sert aux hologrammes et aux « cubes de données » autour de l'holotable.
 */
UCLASS()
class PROJETNEURO_API ANFAnimatedProp : public AActor
{
	GENERATED_BODY()

public:
	ANFAnimatedProp();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UStaticMeshComponent* GetMesh() const { return Mesh; }
	UInstancedStaticMeshComponent* GetOrbit() const { return Orbit; }

	/** Rotation du mesh principal, en degrés par seconde. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Animation")
	FRotator RotationRate = FRotator(0.f, 20.f, 0.f);

	/** Amplitude du flottement vertical, en cm (0 = aucun). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Animation")
	float BobAmplitude = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Animation")
	float BobFrequency = 0.4f;

	/** Nombre d'objets en orbite (0 = aucun). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Animation", meta = (ClampMin = "0", ClampMax = "128"))
	int32 OrbitCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Animation")
	float OrbitRadius = 150.f;

	/** Vitesse de rotation de l'orbite, en degrés par seconde. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Animation")
	float OrbitSpeed = 15.f;

	/** Ondulation verticale des objets en orbite, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Animation")
	float OrbitWave = 10.f;

	/** Taille des objets en orbite, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Animation")
	float OrbitItemSize = 6.f;

	/** Holotable suivie pour accélérer l'animation pendant l'entraînement (facultatif). */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Neuron Forge|Animation")
	TObjectPtr<ANFNetworkBuilder> Builder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Animation", meta = (ClampMin = "1"))
	float TrainingSpeedMultiplier = 3.f;

protected:
	void RebuildOrbit();
	void UpdateOrbit(float Angle, float Time);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UInstancedStaticMeshComponent> Orbit;

private:
	FVector MeshRestLocation = FVector::ZeroVector;
	float OrbitAngle = 0.f;
	float SpeedFactor = 1.f;
};
