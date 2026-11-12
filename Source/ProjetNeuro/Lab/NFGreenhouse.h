// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NFGreenhouse.generated.h"

class ANFGreenhousePlant;
class ANFNetworkBuilder;
class UAudioComponent;
class UBoxComponent;
class USoundBase;
class UTextRenderComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNFOnGreenhouseActivated);

/**
 * La serre derrière la porte du labo.
 *
 * Quand le joueur entre (trigger Box Collision), le modèle entraîné prédit le sort de
 * chaque plante de la grille ; les plantes prédites vivantes poussent et fleurissent,
 * les autres fanent, en vague depuis l'entrée. Un panneau affiche le bilan :
 * « Ton modèle a fait pousser X plantes sur Y, il s'est trompé sur Z ».
 */
UCLASS()
class PROJETNEURO_API ANFGreenhouse : public AActor
{
	GENERATED_BODY()

public:
	ANFGreenhouse();

	virtual void BeginPlay() override;

	/** Fait prédire le modèle sur toutes les plantes et lance l'animation. Faux si le modèle n'est pas validé. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Greenhouse")
	bool Activate();

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Greenhouse")
	bool IsActivated() const { return bActivated; }

	/** Plantes que le modèle a fait pousser (survie prédite). */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Greenhouse")
	int32 GetGrownCount() const { return GrownCount; }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Greenhouse")
	int32 GetPlantCount() const { return Plants.Num(); }

	/** Plantes dont la prédiction contredit la vraie serre. */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Greenhouse")
	int32 GetMistakeCount() const { return MistakeCount; }

	/** Plantes qui survivraient vraiment. */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Greenhouse")
	int32 GetReallyAliveCount() const { return ReallyAliveCount; }

	/** Texte du bilan (« Ton modèle a fait pousser... »). */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Greenhouse")
	FString GetSummaryText() const;

	const TArray<TObjectPtr<ANFGreenhousePlant>>& GetPlants() const { return Plants; }

	/** Holotable dont le modèle est utilisé. Si vide, la première holotable du niveau. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Neuron Forge|Greenhouse")
	TObjectPtr<ANFNetworkBuilder> Builder;

	/** Plantes de la serre. Si vide, toutes les plantes du niveau. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Neuron Forge|Greenhouse")
	TArray<TObjectPtr<ANFGreenhousePlant>> Plants;

	/** Délai par mètre depuis l'entrée : l'animation se propage en vague. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Greenhouse")
	float WaveDelayPerMeter = 0.18f;

	/** Son d'ambiance de la serre (boucle), joué à l'entrée du joueur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Audio")
	TObjectPtr<USoundBase> AmbientSound;

	/** Son joué au lancement de la pousse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Audio")
	TObjectPtr<USoundBase> GrowSound;

	UPROPERTY(BlueprintAssignable, Category = "Neuron Forge|Greenhouse")
	FNFOnGreenhouseActivated OnActivated;

protected:
	UFUNCTION()
	void HandleTriggerBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void RefreshBoard();

	/** Zone d'entrée (côté -X de l'acteur) qui déclenche la serre. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UBoxComponent> EntranceTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> BoardTitle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> BoardText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UAudioComponent> Ambience;

private:
	bool bActivated = false;
	int32 GrownCount = 0;
	int32 MistakeCount = 0;
	int32 ReallyAliveCount = 0;
};

/**
 * Lucioles : petites sphères lumineuses qui volettent doucement dans un volume
 * et scintillent (instances d'un mesh de base, animées en C++).
 */
UCLASS()
class PROJETNEURO_API ANFFireflies : public AActor
{
	GENERATED_BODY()

public:
	ANFFireflies();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Fireflies", meta = (ClampMin = "1", ClampMax = "200"))
	int32 Count = 50;

	/** Demi-dimensions du volume où volent les lucioles, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Fireflies")
	FVector Extent = FVector(800.f, 700.f, 120.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Fireflies")
	float Size = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Fireflies")
	TObjectPtr<UMaterialInterface> GlowMaterial;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UInstancedStaticMeshComponent> Swarm;

private:
	struct FFirefly
	{
		FVector Home;
		FVector Frequency;
		FVector Phase;
		float Blink;
	};
	TArray<FFirefly> Flies;
};

/**
 * Rampe d'arrosage d'une rangée de la serre : un tuyau et des gouttes qui tombent,
 * d'autant plus nombreuses que l'arrosage est fort.
 */
UCLASS()
class PROJETNEURO_API ANFSprinklerRow : public AActor
{
	GENERATED_BODY()

public:
	ANFSprinklerRow();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Arrosage de la rangée (0 à 10) : nombre de gouttes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Sprinkler", meta = (ClampMin = "0", ClampMax = "10"))
	float Water = 5.f;

	/** Longueur de la rampe, le long de l'axe Y local, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Sprinkler")
	float Length = 1200.f;

	/** Hauteur de chute des gouttes, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Sprinkler")
	float DropHeight = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Sprinkler")
	TObjectPtr<UMaterialInterface> PipeMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Sprinkler")
	TObjectPtr<UMaterialInterface> DropMaterial;

protected:
	void RebuildDrops();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Pipe;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UInstancedStaticMeshComponent> Drops;

private:
	TArray<FVector2D> DropSeeds;	// position le long de la rampe, phase de chute
};
