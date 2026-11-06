// Neuron Forge - ENSIM

#include "Lab/NFLabDoor.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Lab/NFNetworkBuilder.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UStaticMeshComponent* MakeDoorPart(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(Mesh);
		Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		return Part;
	}
}

ANFLabDoor::ANFLabDoor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	FrameLeft = MakeDoorPart(this, SceneRoot, TEXT("FrameLeft"), CubeMesh.Object);
	FrameRight = MakeDoorPart(this, SceneRoot, TEXT("FrameRight"), CubeMesh.Object);
	FrameTop = MakeDoorPart(this, SceneRoot, TEXT("FrameTop"), CubeMesh.Object);
	PanelLeft = MakeDoorPart(this, SceneRoot, TEXT("PanelLeft"), CubeMesh.Object);
	PanelRight = MakeDoorPart(this, SceneRoot, TEXT("PanelRight"), CubeMesh.Object);

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(SceneRoot);
	StatusLight->SetIntensityUnits(ELightUnits::Candelas);
	StatusLight->SetIntensity(40.f);
	StatusLight->SetAttenuationRadius(350.f);
	StatusLight->SetCastShadows(false);

	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText"));
	StatusText->SetupAttachment(SceneRoot);
	StatusText->SetHorizontalAlignment(EHTA_Center);
	StatusText->SetVerticalAlignment(EVRTA_TextBottom);
	StatusText->SetWorldSize(16.f);
	StatusText->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetLocked(true);
}

void ANFLabDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutDoor();
	ApplyOpenAlpha(OpenAlpha);
}

void ANFLabDoor::LayoutDoor()
{
	// Le cube de base fait 100 cm et son pivot est au centre
	const float Depth = 30.f;
	const float HalfWidth = DoorWidth * 0.5f;

	FrameLeft->SetRelativeLocation(FVector(0.f, HalfWidth + FrameThickness * 0.5f, DoorHeight * 0.5f));
	FrameLeft->SetRelativeScale3D(FVector(Depth, FrameThickness, DoorHeight) / 100.f);

	FrameRight->SetRelativeLocation(FVector(0.f, -HalfWidth - FrameThickness * 0.5f, DoorHeight * 0.5f));
	FrameRight->SetRelativeScale3D(FVector(Depth, FrameThickness, DoorHeight) / 100.f);

	FrameTop->SetRelativeLocation(FVector(0.f, 0.f, DoorHeight + FrameThickness * 0.5f));
	FrameTop->SetRelativeScale3D(FVector(Depth, DoorWidth + 2.f * FrameThickness, FrameThickness) / 100.f);

	PanelLeftClosed = FVector(0.f, HalfWidth * 0.5f, DoorHeight * 0.5f);
	PanelRightClosed = FVector(0.f, -HalfWidth * 0.5f, DoorHeight * 0.5f);
	PanelLeft->SetRelativeScale3D(FVector(PanelThickness, HalfWidth, DoorHeight) / 100.f);
	PanelRight->SetRelativeScale3D(FVector(PanelThickness, HalfWidth, DoorHeight) / 100.f);

	StatusLight->SetRelativeLocation(FVector(Depth, 0.f, DoorHeight + FrameThickness * 0.5f));
	StatusText->SetRelativeLocation(FVector(Depth * 0.5f + 1.f, 0.f, DoorHeight + FrameThickness + 4.f));
}

void ANFLabDoor::BeginPlay()
{
	Super::BeginPlay();

	LayoutDoor();

	// Courbe ease-in-out par défaut
	UCurveFloat* Curve = OpenCurve;
	if (!Curve)
	{
		RuntimeCurve = NewObject<UCurveFloat>(this, TEXT("DoorOpenCurve"));
		const FKeyHandle Start = RuntimeCurve->FloatCurve.AddKey(0.f, 0.f);
		const FKeyHandle End = RuntimeCurve->FloatCurve.AddKey(1.f, 1.f);
		RuntimeCurve->FloatCurve.SetKeyInterpMode(Start, RCIM_Cubic);
		RuntimeCurve->FloatCurve.SetKeyInterpMode(End, RCIM_Cubic);
		Curve = RuntimeCurve;
	}

	FOnTimelineFloat ProgressFunction;
	ProgressFunction.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ANFLabDoor, HandleTimelineProgress));
	DoorTimeline.AddInterpFloat(Curve, ProgressFunction);

	FOnTimelineEvent FinishedFunction;
	FinishedFunction.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ANFLabDoor, HandleTimelineFinished));
	DoorTimeline.SetTimelineFinishedFunc(FinishedFunction);
	DoorTimeline.SetTimelineLength(1.f);
	DoorTimeline.SetPlayRate(1.f / OpenDuration);

	if (!Builder)
	{
		for (TActorIterator<ANFNetworkBuilder> It(GetWorld()); It; ++It)
		{
			Builder = *It;
			break;
		}
	}
	if (Builder && Builder->GetTrainer())
	{
		Builder->GetTrainer()->OnStateChanged.AddUniqueDynamic(this, &ANFLabDoor::HandleTrainingStateChanged);
	}

	SetLocked(true);
	ApplyOpenAlpha(0.f);
}

void ANFLabDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	DoorTimeline.TickTimeline(DeltaSeconds);
}

void ANFLabDoor::HandleTrainingStateChanged(ENFTrainingState NewState)
{
	if (NewState == ENFTrainingState::Succeeded && bOpenOnTrainingSuccess)
	{
		if (ValidationSound)
		{
			UGameplayStatics::PlaySound2D(this, ValidationSound);
		}
		Open();
	}
}

void ANFLabDoor::Open()
{
	if (DoorState == ENFDoorState::Open || DoorState == ENFDoorState::Opening)
	{
		return;
	}

	SetLocked(false);
	if (DoorMoveSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DoorMoveSound, GetActorLocation());
	}
	SetDoorState(ENFDoorState::Opening);
	DoorTimeline.Play();
}

void ANFLabDoor::Close()
{
	if (DoorState == ENFDoorState::Closed || DoorState == ENFDoorState::Closing)
	{
		return;
	}

	if (DoorMoveSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DoorMoveSound, GetActorLocation());
	}
	SetDoorState(ENFDoorState::Closing);
	DoorTimeline.Reverse();
}

void ANFLabDoor::HandleTimelineProgress(float Value)
{
	ApplyOpenAlpha(Value);
}

void ANFLabDoor::HandleTimelineFinished()
{
	if (DoorState == ENFDoorState::Opening)
	{
		SetDoorState(ENFDoorState::Open);
	}
	else if (DoorState == ENFDoorState::Closing)
	{
		SetDoorState(ENFDoorState::Closed);
		SetLocked(true);
	}
}

void ANFLabDoor::ApplyOpenAlpha(float Alpha)
{
	OpenAlpha = FMath::Clamp(Alpha, 0.f, 1.f);

	// Chaque battant glisse de sa propre largeur vers l'extérieur, derrière le cadre
	const float Slide = DoorWidth * 0.5f * OpenAlpha;
	PanelLeft->SetRelativeLocation(PanelLeftClosed + FVector(0.f, Slide, 0.f));
	PanelRight->SetRelativeLocation(PanelRightClosed - FVector(0.f, Slide, 0.f));
}

void ANFLabDoor::SetDoorState(ENFDoorState NewState)
{
	if (DoorState != NewState)
	{
		DoorState = NewState;
		OnDoorStateChanged.Broadcast(NewState);
	}
}

void ANFLabDoor::SetLocked(bool bLocked)
{
	const FLinearColor Color = bLocked ? LockedColor : UnlockedColor;
	StatusLight->SetLightColor(Color);
	StatusText->SetText(FText::FromString(bLocked ? TEXT("ACCÈS VERROUILLÉ") : TEXT("ACCÈS AUTORISÉ")));
	StatusText->SetTextRenderColor(Color.ToFColor(true));
}
