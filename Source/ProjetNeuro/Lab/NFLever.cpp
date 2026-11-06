// Neuron Forge - ENSIM

#include "Lab/NFLever.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Interaction/NFInteractableActor.h"
#include "Kismet/GameplayStatics.h"
#include "Lab/NFNetworkBuilder.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "NeuronForgeLever"

namespace
{
	FString FormatRate(float Value)
	{
		return Value < 0.1f ? FString::Printf(TEXT("%.2f"), Value) : FString::Printf(TEXT("%.1f"), Value);
	}
}

ANFLever::ANFLever()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));

	// Socle : 60 x 40 x 100 cm, face avant vers +X
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	RootComponent = Base;
	Base->SetStaticMesh(CubeMesh.Object);
	Base->SetRelativeScale3D(FVector(0.4f, 0.6f, 1.f));

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(Base);
	Pivot->SetAbsolute(false, false, true);
	Pivot->SetRelativeLocation(FVector(0.f, 0.f, 50.f));	// haut du socle (échelle Z = 1)

	// Poignée : tige de 70 cm qui bascule d'avant en arrière autour de l'axe Y
	Handle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Handle"));
	Handle->SetupAttachment(Pivot);
	Handle->SetStaticMesh(CylinderMesh.Object);
	Handle->SetRelativeLocation(FVector(0.f, 0.f, 35.f));
	Handle->SetRelativeScale3D(FVector(0.07f, 0.07f, 0.7f));

	Knob = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Knob"));
	Knob->SetupAttachment(Pivot);
	Knob->SetStaticMesh(SphereMesh.Object);
	Knob->SetRelativeLocation(FVector(0.f, 0.f, 72.f));
	Knob->SetRelativeScale3D(FVector(0.18f));

	Dial = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Dial"));
	Dial->SetupAttachment(Base);
	Dial->SetAbsolute(false, false, true);
	Dial->SetRelativeLocation(FVector(55.f, 0.f, 5.f));	// devant la face avant (échelle X = 0,4)
	Dial->SetHorizontalAlignment(EHTA_Center);
	Dial->SetVerticalAlignment(EVRTA_TextCenter);
	Dial->SetWorldSize(9.f);
	Dial->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANFLever::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (BodyMaterial)
	{
		Base->SetMaterial(0, BodyMaterial);
	}
	if (HandleMaterial)
	{
		Handle->SetMaterial(0, HandleMaterial);
		Knob->SetMaterial(0, HandleMaterial);
	}
	UpdateDial();
}

void ANFLever::BeginPlay()
{
	Super::BeginPlay();
	if (!Builder)
	{
		for (TActorIterator<ANFNetworkBuilder> It(GetWorld()); It; ++It)
		{
			Builder = *It;
			break;
		}
	}
	CurrentAngle = GetTargetAngle();
	UpdateDial();
}

void ANFLever::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CurrentAngle = FMath::FInterpTo(CurrentAngle, GetTargetAngle(), DeltaSeconds, 8.f);
	Pivot->SetRelativeRotation(FRotator(CurrentAngle, 0.f, 0.f));
	UpdateDial();
}

float ANFLever::GetTargetAngle() const
{
	if (!Builder || Builder->LearningRateSteps.Num() < 2)
	{
		return 0.f;
	}
	const float Alpha = static_cast<float>(Builder->LearningRateIndex) / (Builder->LearningRateSteps.Num() - 1);
	return FMath::Lerp(MinAngle, MaxAngle, Alpha);
}

int32 ANFLever::GetNextIndex() const
{
	const int32 Count = Builder ? Builder->LearningRateSteps.Num() : 0;
	return Count > 0 ? (Builder->LearningRateIndex + 1) % Count : 0;
}

void ANFLever::UpdateDial()
{
	if (Dial)
	{
		const FString Value = Builder ? FormatRate(Builder->GetLearningRate()) : TEXT("--");
		Dial->SetText(FText::FromString(FString::Printf(TEXT("TAUX D'APPRENTISSAGE\n%s"), *Value)));
		Dial->SetTextRenderColor(FColor(255, 200, 90));
	}
}

bool ANFLever::CanInteract_Implementation(AActor* InstigatorActor) const
{
	return Builder && Builder->CanEdit();
}

void ANFLever::Interact_Implementation(AActor* InstigatorActor)
{
	if (!Builder || !Builder->CanEdit())
	{
		return;
	}
	Builder->SetConfiguration(Builder->GetHiddenLayers(), GetNextIndex());
	if (PullSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PullSound, GetActorLocation());
	}
}

void ANFLever::SetHighlighted(bool bOn)
{
	for (UStaticMeshComponent* Part : { Handle.Get(), Knob.Get() })
	{
		if (Part)
		{
			Part->SetScalarParameterValueOnMaterials(ANFInteractableActor::HighlightParameterName, bOn ? 1.f : 0.f);
			Part->SetRenderCustomDepth(bOn);
		}
	}
}

void ANFLever::OnFocusBegin_Implementation(AActor* InstigatorActor)
{
	SetHighlighted(true);
}

void ANFLever::OnFocusEnd_Implementation(AActor* InstigatorActor)
{
	SetHighlighted(false);
}

FText ANFLever::GetInteractionPrompt_Implementation() const
{
	if (!Builder)
	{
		return LOCTEXT("NoBuilder", "Levier");
	}
	if (!Builder->CanEdit())
	{
		return LOCTEXT("Locked", "Levier bloqué pendant l'entraînement");
	}
	const int32 Next = GetNextIndex();
	return FText::Format(LOCTEXT("Pull", "Tirer le levier : taux d'apprentissage {0} → {1}"),
		FText::FromString(FormatRate(Builder->GetLearningRate())),
		FText::FromString(FormatRate(Builder->LearningRateSteps.IsValidIndex(Next) ? Builder->LearningRateSteps[Next] : 0.f)));
}

#undef LOCTEXT_NAMESPACE
