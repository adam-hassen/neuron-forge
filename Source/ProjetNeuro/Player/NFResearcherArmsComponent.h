// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "NFResearcherArmsComponent.generated.h"

/**
 * Bras du chercheur vus à la première personne.
 *
 * Dans le template, les bras « mains vides » pendent le long du corps et ne sont jamais
 * visibles. Ce composant (mesh posable, sans modifier les assets du template) place les
 * mains par cinématique inverse à deux os (épaule - coude - poignet) :
 *  - au repos, légèrement devant la caméra, en bas de l'écran, avec un balancement de marche ;
 *  - quand le joueur tient une pièce, de part et d'autre de la pièce.
 * Le material (peau / manche de blouse) masque tout ce qui n'est pas un bras.
 */
UCLASS(ClassGroup = (NeuronForge), meta = (BlueprintSpawnableComponent))
class PROJETNEURO_API UNFResearcherArmsComponent : public UPoseableMeshComponent
{
	GENERATED_BODY()

public:
	UNFResearcherArmsComponent();

	/**
	 * Place les mains pour cette frame.
	 * @param Camera		position et orientation du regard (monde)
	 * @param HeldObject	objet tenu (ou nullptr) : les mains l'encadrent
	 * @param Speed			vitesse horizontale du personnage (balancement de marche)
	 */
	void UpdateArms(const FTransform& Camera, const AActor* HeldObject, float Speed, float DeltaSeconds);

	/** Position monde du poignet (os hand_l / hand_r). */
	FVector GetHandLocation(bool bRight) const;

	/** Position de la main droite au repos, dans le repère de la caméra (avant, droite, haut), en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Arms")
	FVector RestOffset = FVector(40.f, 15.f, -16.f);

	/** Demi-écart entre les mains quand elles tiennent une pièce, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Arms")
	float HoldSpread = 15.f;

	/** Réactivité des mains (plus grand = plus rapide). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Arms")
	float FollowSpeed = 12.f;

	/** Amplitude du balancement de marche, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|Arms")
	float WalkBob = 1.6f;

private:
	struct FArmChain
	{
		FName Upper;
		FName Lower;
		FName Hand;
		FTransform RefUpper;	// transforms de la pose de référence, espace du composant
		FTransform RefLower;
		FTransform RefHand;
		float UpperLength = 0.f;
		float LowerLength = 0.f;
		FVector Pole = FVector::ZeroVector;	// direction préférée du coude
		FVector SmoothedTarget = FVector::ZeroVector;	// repère caméra
		bool bValid = false;
		bool bHasTarget = false;
	};

	void InitChains();
	void SolveArm(const FArmChain& Chain, const FVector& TargetComponentSpace);

	FArmChain Left;
	FArmChain Right;
	bool bChainsReady = false;
	float BobTime = 0.f;
};
