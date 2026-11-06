// Neuron Forge - ENSIM

#include "Interaction/NFInteractableActor.h"
#include "Components/StaticMeshComponent.h"

const FName ANFInteractableActor::HighlightParameterName(TEXT("Highlight"));

ANFInteractableActor::ANFInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Mesh->SetCustomDepthStencilValue(1);

	Prompt = NSLOCTEXT("NeuronForge", "InteractablePrompt", "Interagir");
}

void ANFInteractableActor::SetInteractionEnabled(bool bEnabled)
{
	if (bInteractionEnabled != bEnabled)
	{
		bInteractionEnabled = bEnabled;
		RefreshVisuals();
	}
}

bool ANFInteractableActor::CanInteract_Implementation(AActor* InstigatorActor) const
{
	return bInteractionEnabled;
}

void ANFInteractableActor::Interact_Implementation(AActor* InstigatorActor)
{
	++InteractionCount;
	HandleInteract(InstigatorActor);
	OnInteracted.Broadcast(this, InstigatorActor);
}

void ANFInteractableActor::OnFocusBegin_Implementation(AActor* InstigatorActor)
{
	bHighlighted = true;
	RefreshVisuals();
}

void ANFInteractableActor::OnFocusEnd_Implementation(AActor* InstigatorActor)
{
	bHighlighted = false;
	RefreshVisuals();
}

FText ANFInteractableActor::GetInteractionPrompt_Implementation() const
{
	return Prompt;
}

void ANFInteractableActor::RefreshVisuals()
{
	if (Mesh)
	{
		const bool bShowHighlight = bHighlighted && bInteractionEnabled;
		Mesh->SetRenderCustomDepth(bShowHighlight);
		Mesh->SetScalarParameterValueOnMaterials(HighlightParameterName, bShowHighlight ? HighlightIntensity : 0.f);
	}
}
