// Neuron Forge - ENSIM

#include "Lab/NFConfigButton.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ANFConfigButton::ANFConfigButton()
{
	PrimaryActorTick.bCanEverTick = true;

	// Bouton de 25 x 25 cm, 8 cm d'épaisseur, face avant vers +X
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	Mesh->SetStaticMesh(CubeMesh.Object);
	Mesh->SetRelativeScale3D(FVector(0.08f, 0.25f, 0.25f));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(SceneRoot);
	Label->SetRelativeLocation(FVector(4.5f, 0.f, 0.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(7.f);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANFConfigButton::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshButton();
}

void ANFConfigButton::BeginPlay()
{
	Super::BeginPlay();

	MeshRestLocation = Mesh->GetRelativeLocation();

	if (!Builder)
	{
		for (TActorIterator<ANFNetworkBuilder> It(GetWorld()); It; ++It)
		{
			Builder = *It;
			break;
		}
	}
	if (Builder)
	{
		Builder->OnNetworkChanged.AddUniqueDynamic(this, &ANFConfigButton::HandleNetworkChanged);
	}
	RefreshButton();
}

void ANFConfigButton::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Retour progressif du bouton après l'appui
	if (PressAlpha > 0.f)
	{
		PressAlpha = FMath::Max(0.f, PressAlpha - DeltaSeconds * 6.f);
		Mesh->SetRelativeLocation(MeshRestLocation - FVector(PressDepth * PressAlpha, 0.f, 0.f));
	}
}

bool ANFConfigButton::CanInteract_Implementation(AActor* InstigatorActor) const
{
	return Super::CanInteract_Implementation(InstigatorActor) && Builder && Builder->CanApplyAction(Action, LayerIndex);
}

FText ANFConfigButton::GetInteractionPrompt_Implementation() const
{
	return Builder ? Builder->DescribeAction(Action, LayerIndex) : Super::GetInteractionPrompt_Implementation();
}

void ANFConfigButton::HandleInteract(AActor* InstigatorActor)
{
	if (Builder && Builder->ApplyAction(Action, LayerIndex))
	{
		PressAlpha = 1.f;
		if (ClickSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ClickSound, GetActorLocation());
		}
	}
}

void ANFConfigButton::HandleNetworkChanged()
{
	RefreshButton();
}

void ANFConfigButton::RefreshButton()
{
	if (Label)
	{
		Label->SetText(FText::FromString(Builder ? Builder->GetActionLabel(Action, LayerIndex) : UEnum::GetDisplayValueAsText(Action).ToString()));
	}
	RefreshVisuals();
}

void ANFConfigButton::RefreshVisuals()
{
	Super::RefreshVisuals();

	if (Label)
	{
		const bool bAvailable = Builder ? Builder->CanApplyAction(Action, LayerIndex) : true;
		Label->SetTextRenderColor(bAvailable ? EnabledLabelColor : DisabledLabelColor);
	}
}
