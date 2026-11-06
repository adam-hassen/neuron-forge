// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "NFSocketComponent.generated.h"

class ANFPiece;

/** Types de pièces du puzzle de la Forge. */
UENUM(BlueprintType)
enum class ENFPieceType : uint8
{
	/** Module de couche cachée : s'insère dans une fente de la Forge. */
	LayerModule			UMETA(DisplayName = "Module de couche"),
	/** Orbe-neurone : se pose autour d'un module (une orbe = un neurone). */
	NeuronOrb			UMETA(DisplayName = "Orbe-neurone"),
	/** Cristal d'activation : se plante sur le dessus d'un module. */
	ActivationCrystal	UMETA(DisplayName = "Cristal d'activation"),
};

/**
 * Emplacement qui accueille une pièce d'un type donné.
 * Une pièce relâchée à moins de SnapRadius d'un emplacement libre compatible s'y emboîte.
 */
UCLASS(ClassGroup = (NeuronForge), meta = (BlueprintSpawnableComponent))
class PROJETNEURO_API UNFSocketComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Socket")
	ANFPiece* GetPiece() const { return Piece.Get(); }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|Socket")
	bool IsFree() const { return !Piece.IsValid(); }

	/** L'emplacement est-il libre et du bon type pour cette pièce ? */
	bool CanAccept(const ANFPiece* Candidate) const;

	/** Appelés par la pièce elle-même lors de l'emboîtement / du retrait. */
	void SetPiece(ANFPiece* InPiece) { Piece = InPiece; }
	void ClearPiece() { Piece.Reset(); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Socket")
	ENFPieceType AcceptedType = ENFPieceType::NeuronOrb;

	/** Distance maximale d'emboîtement, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Socket", meta = (ClampMin = "1"))
	float SnapRadius = 70.f;

	/** Nom affiché dans l'aide (« fente 2 », « module »...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Socket")
	FText SocketName;

private:
	TWeakObjectPtr<ANFPiece> Piece;
};
