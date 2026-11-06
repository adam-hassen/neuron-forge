// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "Lab/NFPiece.h"
#include "NFLayerModule.generated.h"

class UTextRenderComponent;

/**
 * Module de couche cachée : un cube que le joueur insère dans une fente de la Forge.
 *  - 8 emplacements en couronne pour les orbes-neurones (une orbe = un neurone) ;
 *  - 1 emplacement sur le dessus pour un cristal d'activation (sans cristal : Linear).
 * Les orbes et le cristal restent attachés au module quand on le déplace.
 */
UCLASS()
class PROJETNEURO_API ANFLayerModule : public ANFPiece
{
	GENERATED_BODY()

public:
	ANFLayerModule();

	virtual void Tick(float DeltaSeconds) override;

	/** Nombre d'orbes emboîtées = nombre de neurones de la couche. */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Module")
	int32 GetNeuronCount() const;

	/** Activation du cristal emboîté, Linear sans cristal. */
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Module")
	ENFActivation GetLayerActivation() const;

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Module")
	bool HasCrystal() const;

	const TArray<TObjectPtr<UNFSocketComponent>>& GetOrbSockets() const { return OrbSockets; }
	UNFSocketComponent* GetCrystalSocket() const { return CrystalSocket; }

	static constexpr int32 MaxNeurons = 8;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TArray<TObjectPtr<UNFSocketComponent>> OrbSockets;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UNFSocketComponent> CrystalSocket;

	/** Résumé affiché au-dessus du module : « 4 neurones · Tanh ». */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Neuron Forge|Components")
	TObjectPtr<UTextRenderComponent> Summary;
};
