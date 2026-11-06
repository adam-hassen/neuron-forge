// Neuron Forge - ENSIM

#include "Lab/NFFloorButton.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Interaction/NFInteractableActor.h"
#include "Kismet/GameplayStatics.h"
#include "Lab/NFNetworkBuilder.h"
#include "Player/NFPlayerController.h"
#include "UObject/ConstructorHelpers.h"

ANFFloorButton::ANFFloorButton()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));

	// Socle : disque de 160 cm de diamètre, 4 cm de haut
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(SceneRoot);
	BaseMesh->SetStaticMesh(CylinderMesh.Object);
	BaseMesh->SetRelativeLocation(FVector(0.f, 0.f, 2.f));
	BaseMesh->SetRelativeScale3D(FVector(1.6f, 1.6f, 0.04f));

	// Plaque : disque de 120 cm, dépasse de 6 cm au-dessus du sol
	PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
	PlateMesh->SetupAttachment(SceneRoot);
	PlateMesh->SetStaticMesh(CylinderMesh.Object);
	PlateMesh->SetRelativeLocation(FVector(0.f, 0.f, 4.f));
	PlateMesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.04f));

	// Box Collision : détecte le joueur debout sur la plaque
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(SceneRoot);
	Trigger->SetBoxExtent(FVector(55.f, 55.f, 40.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	Trigger->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->ShapeColor = FColor(0, 200, 255);

	// Texte posé à plat sur la plaque, lisible depuis +X
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(PlateMesh);
	Label->SetAbsolute(false, false, true);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 55.f));
	Label->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(14.f);
	Label->SetText(FText::FromString(TEXT("ENTRAÎNER")));
	Label->SetTextRenderColor(FColor(120, 220, 255));
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANFFloorButton::BeginPlay()
{
	Super::BeginPlay();

	PlateRestLocation = PlateMesh->GetRelativeLocation();

	if (!Builder)
	{
		for (TActorIterator<ANFNetworkBuilder> It(GetWorld()); It; ++It)
		{
			Builder = *It;
			break;
		}
	}

	Trigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ANFFloorButton::HandleTriggerBegin);
	Trigger->OnComponentEndOverlap.AddUniqueDynamic(this, &ANFFloorButton::HandleTriggerEnd);
}

void ANFFloorButton::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// La plaque s'enfonce tant qu'un joueur est dessus
	const float Target = IsPressed() ? 1.f : 0.f;
	if (!FMath::IsNearlyEqual(PressAlpha, Target))
	{
		PressAlpha = FMath::FInterpConstantTo(PressAlpha, Target, DeltaSeconds, PressSpeed);
		PlateMesh->SetRelativeLocation(PlateRestLocation - FVector(0.f, 0.f, PressDepth * PressAlpha));
	}

	// Lumière : forte pendant l'appui, pulsation douce pendant l'entraînement
	float Glow = PressAlpha;
	if (Builder && Builder->GetTrainingState() == ENFTrainingState::Training)
	{
		Glow = FMath::Max(Glow, 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f));
	}
	PlateMesh->SetScalarParameterValueOnMaterials(ANFInteractableActor::HighlightParameterName, Glow);
}

void ANFFloorButton::HandleTriggerBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	++OverlappingPawns;
	if (OverlappingPawns == 1)
	{
		Press(Pawn);
	}
}

void ANFFloorButton::HandleTriggerEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (Cast<APawn>(OtherActor))
	{
		OverlappingPawns = FMath::Max(0, OverlappingPawns - 1);
	}
}

void ANFFloorButton::Press(APawn* Pawn)
{
	if (PressSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PressSound, GetActorLocation());
	}
	OnPressed.Broadcast(Pawn);

	if (!Builder)
	{
		return;
	}

	if (!Builder->StartTraining())
	{
		if (ANFPlayerController* PC = Cast<ANFPlayerController>(Pawn->GetController()))
		{
			PC->ShowMessage(TEXT("Entraînement déjà en cours..."), FLinearColor(1.f, 0.8f, 0.3f), 2.f);
		}
	}
}
