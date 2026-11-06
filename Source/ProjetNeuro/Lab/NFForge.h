// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NeuralNetwork/NFNeuralNetwork.h"
#include "NFForge.generated.h"

class ANFLayerModule;
class ANFNetworkBuilder;
class UMaterialInterface;
class UNFSocketComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * La Forge : rail d'assemblage du réseau de neurones.
 *
 *   [ENTRÉES] ══ [fente 1] ══ [fente 2] ══ [fente 3] ══ [SORTIE]
 *
 * Le joueur y insère des modules de couche. La Forge lit l'assemblage en continu
 * (modules présents, orbes-neurones et cristal de chacun) et envoie l'architecture
 * à l'holotable, qui reconstruit le réseau en direct.
 * Un module sans orbe est ignoré (il faut au moins un neurone par couche).
 */
UCLASS()
class PROJETNEURO_API ANFForge : public AActor
{
	GENERATED_BODY()

public:
	ANFForge();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Architecture décrite par les modules insérés, de la fente 1 à la fente 3. */
	void BuildConfiguration(TArray<FNFLayerConfig>& OutLayers) const;

	/** Relit l'assemblage et met à jour l'holotable si l'architecture a changé. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Forge")
	void RefreshAssembly();

	UNFSocketComponent* GetBay(int32 Index) const { return Bays.IsValidIndex(Index) ? Bays[Index] : nullptr; }
	ANFLayerModule* GetModuleInBay(int32 Index) const;

	static constexpr int32 NumBays = 3;

	/** Holotable pilotée. Si vide, la première holotable du niveau est utilisée. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Neuron Forge|Forge")
	TObjectPtr<ANFNetworkBuilder> Builder;

	/** Écart entre deux fentes, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Forge")
	float BaySpacing = 330.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Forge")
	TObjectPtr<UMaterialInterface> PlatformMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Forge")
	TObjectPtr<UMaterialInterface> RailMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Forge")
	TObjectPtr<UMaterialInterface> PylonMaterial;

protected:
	void LayoutForge();
	void UpdateLabels(const TArray<FNFLayerConfig>& HiddenLayers);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Rail;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> InputPylon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> OutputPylon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Platforms;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TArray<TObjectPtr<UNFSocketComponent>> Bays;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TArray<TObjectPtr<UTextRenderComponent>> BayLabels;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> InputLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> OutputLabel;

private:
	float RefreshTimer = 0.f;
};
