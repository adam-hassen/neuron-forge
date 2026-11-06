// Neuron Forge - ENSIM

#include "Lab/NFPiece.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Interaction/NFGrabberComponent.h"
#include "Interaction/NFInteractableActor.h"
#include "Lab/NFNetworkBuilder.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "NeuronForgePiece"

ANFPiece::ANFPiece()
{
	PrimaryActorTick.bCanEverTick = true;

	// Le mesh est la racine : c'est lui que la physique déplace
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetLinearDamping(0.6f);
	Mesh->SetAngularDamping(0.8f);
	Mesh->SetCustomDepthStencilValue(1);
}

void ANFPiece::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyAppearance();
}

void ANFPiece::BeginPlay()
{
	Super::BeginPlay();
	HomeTransform = GetActorTransform();
	ApplyAppearance();

	// Une pièce placée dans le niveau directement sur un emplacement y est emboîtée
	if (!IsDocked())
	{
		if (UNFSocketComponent* Best = FindBestSocket())
		{
			if (FVector::Dist(Best->GetComponentLocation(), GetActorLocation()) < 5.f)
			{
				DockTo(Best);
			}
		}
	}
}

void ANFPiece::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Pièce tombée hors du labo : retour à sa place d'origine
	if (!bHeld && !IsDocked() && GetActorLocation().Z < RecallBelowZ)
	{
		Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		SetActorTransform(HomeTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void ANFPiece::ApplyAppearance()
{
	if (!Mesh)
	{
		return;
	}
	if (BodyMaterial)
	{
		Mesh->SetMaterial(0, BodyMaterial);
	}
	const FLinearColor Color = (PieceType == ENFPieceType::ActivationCrystal) ? NFActivation::GetColor(Activation) : GlowColor;
	Mesh->SetVectorParameterValueOnMaterials(TEXT("EmissiveColor"), FVector(Color));
	Mesh->SetScalarParameterValueOnMaterials(ANFInteractableActor::HighlightParameterName, (bFocused || bHeld) ? 1.f : 0.f);
	Mesh->SetRenderCustomDepth(bFocused || bHeld);
}

FText ANFPiece::GetDisplayName() const
{
	switch (PieceType)
	{
	case ENFPieceType::LayerModule:
		return LOCTEXT("Module", "module de couche");
	case ENFPieceType::ActivationCrystal:
		return FText::Format(LOCTEXT("Crystal", "cristal {0}"), FText::FromString(NFActivation::ToString(Activation)));
	case ENFPieceType::NeuronOrb:
	default:
		return LOCTEXT("Orb", "orbe-neurone");
	}
}

// ---------------------------------------------------------------------------
// INFInteractable

ANFNetworkBuilder* ANFPiece::FindBuilder() const
{
	if (!CachedBuilder.IsValid() && GetWorld())
	{
		for (TActorIterator<ANFNetworkBuilder> It(GetWorld()); It; ++It)
		{
			CachedBuilder = *It;
			break;
		}
	}
	return CachedBuilder.Get();
}

bool ANFPiece::IsLockedByTraining() const
{
	const ANFNetworkBuilder* Builder = FindBuilder();
	return IsDocked() && Builder && Builder->GetTrainingState() == ENFTrainingState::Training;
}

bool ANFPiece::CanInteract_Implementation(AActor* InstigatorActor) const
{
	return !bHeld && !IsLockedByTraining();
}

void ANFPiece::Interact_Implementation(AActor* InstigatorActor)
{
	if (UNFGrabberComponent* Grabber = InstigatorActor ? InstigatorActor->FindComponentByClass<UNFGrabberComponent>() : nullptr)
	{
		Grabber->TryGrab(this);
	}
}

void ANFPiece::OnFocusBegin_Implementation(AActor* InstigatorActor)
{
	bFocused = true;
	ApplyAppearance();
}

void ANFPiece::OnFocusEnd_Implementation(AActor* InstigatorActor)
{
	bFocused = false;
	ApplyAppearance();
}

FText ANFPiece::GetInteractionPrompt_Implementation() const
{
	if (IsLockedByTraining())
	{
		return LOCTEXT("Locked", "Assemblage verrouillé pendant l'entraînement");
	}
	return FText::Format(IsDocked() ? LOCTEXT("Remove", "Retirer : {0}") : LOCTEXT("Take", "Prendre : {0}"), GetDisplayName());
}

// ---------------------------------------------------------------------------
// Prise en main et emboîtement

void ANFPiece::OnGrabbed(UNFGrabberComponent* Grabber)
{
	Undock();
	bHeld = true;
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ApplyAppearance();
}

void ANFPiece::OnReleased()
{
	bHeld = false;
	if (UNFSocketComponent* Best = FindBestSocket())
	{
		DockTo(Best);
	}
	else
	{
		SetFree();
	}
	ApplyAppearance();
}

void ANFPiece::SetFree()
{
	Mesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetSimulatePhysics(true);
}

void ANFPiece::UpdateHeldTransform(const FVector& TargetLocation, const FRotator& TargetRotation, float DeltaSeconds)
{
	const FVector Location = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaSeconds, 18.f);
	const FRotator Rotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaSeconds, 10.f);
	SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
}

UNFSocketComponent* ANFPiece::FindBestSocket() const
{
	UNFSocketComponent* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	const FVector Location = GetActorLocation();

	for (TObjectIterator<UNFSocketComponent> It; It; ++It)
	{
		UNFSocketComponent* Candidate = *It;
		if (!IsValid(Candidate) || Candidate->IsTemplate() || Candidate->GetWorld() != GetWorld() || !Candidate->CanAccept(this))
		{
			continue;
		}
		// Une orbe ne s'emboîte pas sur une pièce qu'elle porte (ex. sur ses propres enfants)
		if (Candidate->GetOwner() && Candidate->GetOwner()->IsAttachedTo(this))
		{
			continue;
		}
		const float Distance = FVector::Dist(Candidate->GetComponentLocation(), Location);
		if (Distance <= Candidate->SnapRadius && Distance < BestDistance)
		{
			Best = Candidate;
			BestDistance = Distance;
		}
	}
	return Best;
}

void ANFPiece::DockTo(UNFSocketComponent* InSocket)
{
	if (!InSocket)
	{
		return;
	}
	Undock();

	Socket = InSocket;
	InSocket->SetPiece(this);

	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AttachToComponent(InSocket, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

void ANFPiece::Undock()
{
	if (UNFSocketComponent* Current = Socket.Get())
	{
		Current->ClearPiece();
	}
	Socket.Reset();
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

#undef LOCTEXT_NAMESPACE
