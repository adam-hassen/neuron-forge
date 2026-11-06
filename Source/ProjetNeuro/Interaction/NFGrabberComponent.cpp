// Neuron Forge - ENSIM

#include "Interaction/NFGrabberComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interaction/NFInteractable.h"
#include "Lab/NFPiece.h"

#define LOCTEXT_NAMESPACE "NeuronForgeGrab"

UNFGrabberComponent::UNFGrabberComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

bool UNFGrabberComponent::GetHoldTransform(FVector& OutLocation, FRotator& OutRotation) const
{
	if (HoldSource)
	{
		OutLocation = HoldSource->GetComponentLocation();
		OutRotation = HoldSource->GetComponentRotation();
		return true;
	}

	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->GetController())
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Pawn->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
	OutLocation = ViewLocation + ViewRotation.Vector() * HoldDistance + FVector(0.f, 0.f, HoldHeightOffset);
	OutRotation = FRotator(0.f, ViewRotation.Yaw, 0.f);	// la pièce reste droite
	return true;
}

void UNFGrabberComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ANFPiece* Piece = HeldPiece.Get();
	FVector Location;
	FRotator Rotation;
	if (Piece && GetHoldTransform(Location, Rotation))
	{
		Piece->UpdateHeldTransform(Location, Rotation, DeltaTime);
	}
}

void UNFGrabberComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Drop();
	Super::EndPlay(EndPlayReason);
}

bool UNFGrabberComponent::TryGrab(ANFPiece* Piece)
{
	if (!Piece || IsHolding() || !INFInteractable::Execute_CanInteract(Piece, GetOwner()))
	{
		return false;
	}
	HeldPiece = Piece;
	Piece->OnGrabbed(this);
	return true;
}

void UNFGrabberComponent::Drop()
{
	if (ANFPiece* Piece = HeldPiece.Get())
	{
		HeldPiece.Reset();
		Piece->OnReleased();
	}
}

UNFSocketComponent* UNFGrabberComponent::GetSnapTarget() const
{
	const ANFPiece* Piece = HeldPiece.Get();
	return Piece ? Piece->FindBestSocket() : nullptr;
}

FText UNFGrabberComponent::GetDropPrompt() const
{
	const ANFPiece* Piece = HeldPiece.Get();
	if (!Piece)
	{
		return FText::GetEmpty();
	}
	if (const UNFSocketComponent* Target = GetSnapTarget())
	{
		return FText::Format(LOCTEXT("Insert", "Insérer {0} dans {1}"), Piece->GetDisplayName(), Target->SocketName);
	}
	return FText::Format(LOCTEXT("Drop", "Poser {0}  (approchez-le d'un emplacement pour l'insérer)"), Piece->GetDisplayName());
}

#undef LOCTEXT_NAMESPACE
