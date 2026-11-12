// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NFLabNotebook.generated.h"

class ANFNetworkBuilder;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;
class UTextRenderComponent;
class UTexture2D;

/**
 * « Carnet du labo » : grand écran qui montre les expériences de la serre
 * sur un graphique eau (horizontal) / lumière (vertical).
 *
 *  - chaque expérience est un point : vert si la plante a survécu, rouge sinon ;
 *  - dès que le modèle s'entraîne, le fond se colore : vert là où le modèle pense
 *    que la plante survit, rouge là où il pense qu'elle meurt (frontière de décision),
 *    la frontière elle-même est surlignée en jaune ;
 *  - les expériences que le modèle se trompe sont entourées de blanc.
 *
 * Le graphique est dessiné dans une texture mise à jour en direct (paramètre « Chart »).
 */
UCLASS()
class PROJETNEURO_API ANFLabNotebook : public AActor
{
	GENERATED_BODY()

public:
	ANFLabNotebook();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Redessine tout le graphique maintenant. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|Notebook")
	void Redraw();

	/** Le fond montre-t-il la prédiction du modèle (après le premier entraînement) ? */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Notebook")
	bool ShowsPrediction() const { return bShowPrediction; }

	/** Couleur du fond (sans les points) aux coordonnées eau / lumière, entre 0 et 10. */
	FColor GetBackgroundColorAt(float Water, float Light) const;

	/** Nombre d'expériences mal prédites lors du dernier dessin. */
	int32 GetMistakeCount() const { return MistakeCount; }

	UTexture2D* GetChartTexture() const { return ChartTexture; }

	/** Holotable observée. Si vide, la première holotable du niveau est utilisée. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Neuron Forge|Notebook")
	TObjectPtr<ANFNetworkBuilder> Builder;

	/** Material du graphique : lit la texture « Chart ». */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Notebook")
	TObjectPtr<UMaterialInterface> ChartMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Notebook")
	TObjectPtr<UMaterialInterface> BoardMaterial;

	/** Côté du graphique, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Notebook")
	float ChartSize = 400.f;

	/** Résolution de la texture, en pixels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Notebook", meta = (ClampMin = "64", ClampMax = "1024"))
	int32 TextureSize = 256;

	/** Fréquence de mise à jour pendant l'entraînement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Notebook", meta = (ClampMin = "1"))
	float UpdatesPerSecond = 6.f;

	/** Corrections d'orientation de la texture sur le panneau (le plan redressé inverse l'horizontale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Notebook")
	bool bFlipHorizontal = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Notebook")
	bool bFlipVertical = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Notebook")
	bool bTranspose = false;

protected:
	UFUNCTION()
	void HandleNetworkChanged();

	void LayoutTexts();
	void EnsureTexture();
	void UploadTexture();

	/** Pixel de la texture correspondant aux coordonnées logiques (u = eau, v = lumière, entre 0 et 1). */
	int32 PixelIndex(float U, float V) const;
	void PlotDisc(float U, float V, float RadiusPixels, const FColor& Color);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Board;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UStaticMeshComponent> Chart;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> Title;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> AxisWater;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> AxisLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TArray<TObjectPtr<UTextRenderComponent>> Ticks;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TArray<TObjectPtr<UTextRenderComponent>> Legend;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> Status;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> ChartTexture;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ChartInstance;

	TArray<FColor> Pixels;
	TArray<FColor> Background;
	bool bShowPrediction = false;
	bool bDirty = true;
	int32 MistakeCount = 0;
	float UpdateTimer = 0.f;
};
