// Neuron Forge - ENSIM

#include "Player/NFResearcherArmsComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"

UNFResearcherArmsComponent::UNFResearcherArmsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);
}

void UNFResearcherArmsComponent::InitChains()
{
	const USkinnedAsset* Asset = GetSkinnedAsset();
	if (!Asset)
	{
		return;
	}
	const FReferenceSkeleton& RefSkeleton = Asset->GetRefSkeleton();

	// Transform de référence d'un os dans l'espace du composant (composition des transforms locaux)
	auto ComponentSpaceRef = [&RefSkeleton](FName Bone, FTransform& Out)
	{
		int32 Index = RefSkeleton.FindBoneIndex(Bone);
		if (Index == INDEX_NONE)
		{
			return false;
		}
		Out = FTransform::Identity;
		while (Index != INDEX_NONE)
		{
			Out = Out * RefSkeleton.GetRefBonePose()[Index];
			Index = RefSkeleton.GetParentIndex(Index);
		}
		return true;
	};

	auto Setup = [&ComponentSpaceRef](FArmChain& Chain, const TCHAR* Side, float OutwardSign)
	{
		Chain.Upper = *FString::Printf(TEXT("upperarm_%s"), Side);
		Chain.Lower = *FString::Printf(TEXT("lowerarm_%s"), Side);
		Chain.Hand = *FString::Printf(TEXT("hand_%s"), Side);
		Chain.bValid = ComponentSpaceRef(Chain.Upper, Chain.RefUpper)
			&& ComponentSpaceRef(Chain.Lower, Chain.RefLower)
			&& ComponentSpaceRef(Chain.Hand, Chain.RefHand);
		if (Chain.bValid)
		{
			Chain.UpperLength = FVector::Dist(Chain.RefUpper.GetLocation(), Chain.RefLower.GetLocation());
			Chain.LowerLength = FVector::Dist(Chain.RefLower.GetLocation(), Chain.RefHand.GetLocation());
			// Coudes vers le bas, l'extérieur et un peu l'arrière (le mannequin regarde vers +Y)
			Chain.Pole = FVector(0.6f * OutwardSign, -0.4f, -1.f).GetSafeNormal();
		}
	};

	Setup(Left, TEXT("l"), 1.f);
	Setup(Right, TEXT("r"), -1.f);
	bChainsReady = Left.bValid && Right.bValid;
}

void UNFResearcherArmsComponent::SolveArm(const FArmChain& Chain, const FVector& TargetComponentSpace)
{
	const FVector Shoulder = Chain.RefUpper.GetLocation();
	const FVector RefElbow = Chain.RefLower.GetLocation();
	const FVector RefHand = Chain.RefHand.GetLocation();
	const float L1 = Chain.UpperLength;
	const float L2 = Chain.LowerLength;

	// Cible ramenée à une distance atteignable
	FVector ToTarget = TargetComponentSpace - Shoulder;
	const FVector Direction = ToTarget.GetSafeNormal();
	const float Distance = FMath::Clamp(ToTarget.Size(), FMath::Abs(L1 - L2) + 1.f, L1 + L2 - 0.5f);
	const FVector Target = Shoulder + Direction * Distance;

	// Position du coude : intersection des deux sphères, côté du vecteur pôle
	const float Along = (L1 * L1 - L2 * L2 + Distance * Distance) / (2.f * Distance);
	const float Height = FMath::Sqrt(FMath::Max(0.f, L1 * L1 - Along * Along));
	FVector Bend = Chain.Pole - Direction * FVector::DotProduct(Chain.Pole, Direction);
	if (!Bend.Normalize())
	{
		Bend = FVector::UpVector;
	}
	const FVector Elbow = Shoulder + Direction * Along + Bend * Height;

	// Rotations : on fait tourner chaque os de sa pose de référence vers sa nouvelle direction
	const FQuat UpperDelta = FQuat::FindBetweenVectors(RefElbow - Shoulder, Elbow - Shoulder);
	const FQuat LowerDelta = FQuat::FindBetweenVectors(UpperDelta.RotateVector(RefHand - RefElbow), Target - Elbow) * UpperDelta;

	SetBoneTransformByName(Chain.Upper, FTransform(UpperDelta * Chain.RefUpper.GetRotation(), Shoulder, Chain.RefUpper.GetScale3D()), EBoneSpaces::ComponentSpace);
	SetBoneTransformByName(Chain.Lower, FTransform(LowerDelta * Chain.RefLower.GetRotation(), Elbow, Chain.RefLower.GetScale3D()), EBoneSpaces::ComponentSpace);
	SetBoneTransformByName(Chain.Hand, FTransform(LowerDelta * Chain.RefHand.GetRotation(), Target, Chain.RefHand.GetScale3D()), EBoneSpaces::ComponentSpace);
}

void UNFResearcherArmsComponent::UpdateArms(const FTransform& Camera, const AActor* HeldObject, float Speed, float DeltaSeconds)
{
	if (!bChainsReady)
	{
		InitChains();
		if (!bChainsReady)
		{
			return;
		}
	}

	// Balancement léger pendant la marche
	BobTime += DeltaSeconds * FMath::Lerp(1.5f, 9.f, FMath::Clamp(Speed / 600.f, 0.f, 1.f));
	const float WalkAmount = FMath::Clamp(Speed / 600.f, 0.f, 1.f);
	const float Bob = WalkBob * (0.35f + WalkAmount) * FMath::Sin(BobTime);

	for (FArmChain* Chain : { &Left, &Right })
	{
		const float Side = (Chain == &Right) ? 1.f : -1.f;

		// Cible désirée, exprimée dans le repère de la caméra (avant, droite, haut)
		FVector Desired;
		if (HeldObject)
		{
			// Les mains saisissent la pièce par les côtés : écart selon sa taille
			FVector Origin, Extent;
			HeldObject->GetActorBounds(false, Origin, Extent);
			const float Spread = FMath::Max(HoldSpread, FMath::Max(Extent.X, Extent.Y) + 4.f);
			const FVector Center = Camera.InverseTransformPosition(Origin);
			Desired = Center + FVector(-6.f, Side * Spread, -0.3f * Extent.Z);
		}
		else
		{
			Desired = FVector(RestOffset.X, Side * RestOffset.Y, RestOffset.Z + Bob * Side);
		}

		if (!Chain->bHasTarget)
		{
			Chain->SmoothedTarget = Desired;
			Chain->bHasTarget = true;
		}
		Chain->SmoothedTarget = FMath::VInterpTo(Chain->SmoothedTarget, Desired, DeltaSeconds, FollowSpeed);

		const FVector World = Camera.TransformPosition(Chain->SmoothedTarget);
		SolveArm(*Chain, GetComponentTransform().InverseTransformPosition(World));
	}
}

FVector UNFResearcherArmsComponent::GetHandLocation(bool bRight) const
{
	return GetSocketLocation(bRight ? FName(TEXT("hand_r")) : FName(TEXT("hand_l")));
}
