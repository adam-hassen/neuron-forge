// Neuron Forge - ENSIM

#include "Interaction/NFInteractorComponent.h"
#include "Interaction/NFInteractable.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

UNFInteractorComponent::UNFInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UNFInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAutoUpdateFocus)
	{
		UpdateFocus();
	}
}

void UNFInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetFocusedActor(nullptr);
	Super::EndPlay(EndPlayReason);
}

bool UNFInteractorComponent::GetAimRay(FVector& OutStart, FVector& OutDirection) const
{
	// 1) Source explicite (contrôleur VR, pointeur...)
	if (TraceSource)
	{
		OutStart = TraceSource->GetComponentLocation();
		OutDirection = TraceSource->GetForwardVector();
		return true;
	}

	// 2) Point de vue du contrôleur du pawn (caméra du joueur)
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->GetController())
	{
		FRotator ViewRotation;
		Pawn->GetController()->GetPlayerViewPoint(OutStart, ViewRotation);
		OutDirection = ViewRotation.Vector();
		return true;
	}

	// 3) À défaut, les "yeux" du propriétaire
	if (const AActor* Owner = GetOwner())
	{
		FRotator EyesRotation;
		Owner->GetActorEyesViewPoint(OutStart, EyesRotation);
		OutDirection = EyesRotation.Vector();
		return true;
	}
	return false;
}

AActor* UNFInteractorComponent::FindInteractableAlong(const FVector& Start, const FVector& Direction) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector End = Start + Direction.GetSafeNormal() * InteractionDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(NFInteractionTrace), /*bTraceComplex*/ false);
	Params.AddIgnoredActor(GetOwner());

	FHitResult Hit;
	const bool bHit = (TraceRadius > 0.f)
		? World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(TraceRadius), Params)
		: World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);

	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
	return (HitActor && HitActor->Implements<UNFInteractable>()) ? HitActor : nullptr;
}

void UNFInteractorComponent::UpdateFocus()
{
	FVector Start;
	FVector Direction;
	AActor* NewFocus = GetAimRay(Start, Direction) ? FindInteractableAlong(Start, Direction) : nullptr;
	SetFocusedActor(NewFocus);
}

void UNFInteractorComponent::SetFocusedActor(AActor* NewFocus)
{
	AActor* OldFocus = FocusedActor.Get();
	if (OldFocus == NewFocus)
	{
		return;
	}

	FocusedActor = NewFocus;

	if (OldFocus)
	{
		INFInteractable::Execute_OnFocusEnd(OldFocus, GetOwner());
	}
	if (NewFocus)
	{
		INFInteractable::Execute_OnFocusBegin(NewFocus, GetOwner());
	}
	OnFocusChanged.Broadcast(NewFocus, OldFocus);
}

bool UNFInteractorComponent::CanInteractWithFocus() const
{
	AActor* Focus = FocusedActor.Get();
	return Focus && INFInteractable::Execute_CanInteract(Focus, GetOwner());
}

bool UNFInteractorComponent::TryInteract()
{
	return InteractWith(FocusedActor.Get());
}

bool UNFInteractorComponent::InteractWith(AActor* Target)
{
	if (!Target || !Target->Implements<UNFInteractable>())
	{
		return false;
	}
	if (!INFInteractable::Execute_CanInteract(Target, GetOwner()))
	{
		return false;
	}

	INFInteractable::Execute_Interact(Target, GetOwner());
	OnInteracted.Broadcast(Target);
	return true;
}
