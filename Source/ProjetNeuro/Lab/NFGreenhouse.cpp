// Neuron Forge - ENSIM

#include "Lab/NFGreenhouse.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Lab/NFGreenhousePlant.h"
#include "Lab/NFNetworkBuilder.h"
#include "Materials/MaterialInterface.h"
#include "Player/NFPlayerController.h"
#include "ProjetNeuro.h"
#include "UObject/ConstructorHelpers.h"

// ===========================================================================
// Serre

ANFGreenhouse::ANFGreenhouse()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

	EntranceTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EntranceTrigger"));
	EntranceTrigger->SetupAttachment(RootComponent);
	EntranceTrigger->SetBoxExtent(FVector(120.f, 300.f, 150.f));
	EntranceTrigger->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	EntranceTrigger->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	EntranceTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EntranceTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	EntranceTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	EntranceTrigger->SetGenerateOverlapEvents(true);
	EntranceTrigger->ShapeColor = FColor(120, 255, 140);

	BoardTitle = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BoardTitle"));
	BoardTitle->SetupAttachment(RootComponent);
	BoardTitle->SetRelativeLocation(FVector(0.f, 0.f, 420.f));
	BoardTitle->SetHorizontalAlignment(EHTA_Center);
	BoardTitle->SetVerticalAlignment(EVRTA_TextCenter);
	BoardTitle->SetWorldSize(30.f);
	BoardTitle->SetText(FText::FromString(TEXT("LA SERRE")));
	BoardTitle->SetTextRenderColor(FColor(150, 255, 170));

	BoardText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BoardText"));
	BoardText->SetupAttachment(RootComponent);
	BoardText->SetRelativeLocation(FVector(0.f, 0.f, 350.f));
	BoardText->SetHorizontalAlignment(EHTA_Center);
	BoardText->SetVerticalAlignment(EVRTA_TextCenter);
	BoardText->SetWorldSize(15.f);
	BoardText->SetTextRenderColor(FColor(255, 240, 200));

	Ambience = CreateDefaultSubobject<UAudioComponent>(TEXT("Ambience"));
	Ambience->SetupAttachment(RootComponent);
	Ambience->bAutoActivate = false;
}

void ANFGreenhouse::BeginPlay()
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
	if (Plants.Num() == 0)
	{
		for (TActorIterator<ANFGreenhousePlant> It(GetWorld()); It; ++It)
		{
			Plants.Add(*It);
		}
	}
	EntranceTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ANFGreenhouse::HandleTriggerBegin);
	RefreshBoard();
}

void ANFGreenhouse::HandleTriggerBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn && Pawn->IsPlayerControlled() && Activate())
	{
		if (ANFPlayerController* PC = Cast<ANFPlayerController>(Pawn->GetController()))
		{
			PC->ShowMessage(GetSummaryText().Replace(TEXT("\n"), TEXT(" ")), FLinearColor(0.5f, 1.f, 0.55f), 7.f);
		}
	}
}

bool ANFGreenhouse::Activate()
{
	if (bActivated)
	{
		return true;
	}
	UNFTrainerComponent* Trainer = Builder ? Builder->GetTrainer() : nullptr;
	if (!Trainer || Builder->GetTrainingState() != ENFTrainingState::Succeeded)
	{
		RefreshBoard();
		return false;
	}

	// Le modèle entraîné décide du sort de chaque plante
	const FNFDataset& Data = Trainer->GetDataset();
	const FVector Entrance = GetActorLocation();
	GrownCount = MistakeCount = ReallyAliveCount = 0;
	for (ANFGreenhousePlant* Plant : Plants)
	{
		if (!Plant)
		{
			continue;
		}
		const float Prediction = Trainer->Predict(Data.Normalize(Plant->Water), Data.Normalize(Plant->Light));
		const bool bAlive = Prediction >= 0.5f;
		const float Delay = 0.4f + WaveDelayPerMeter * FVector::Dist2D(Entrance, Plant->GetActorLocation()) / 100.f;
		Plant->Reveal(bAlive, Prediction, Delay);

		GrownCount += bAlive ? 1 : 0;
		ReallyAliveCount += Plant->ReallySurvives() ? 1 : 0;
		MistakeCount += bAlive != Plant->ReallySurvives() ? 1 : 0;
	}
	bActivated = true;

	UE_LOG(LogNeuronForge, Log, TEXT("Serre : %d plantes poussent sur %d, %d erreurs"), GrownCount, Plants.Num(), MistakeCount);
	if (AmbientSound)
	{
		Ambience->SetSound(AmbientSound);
		Ambience->FadeIn(2.f, 0.6f);
	}
	if (GrowSound)
	{
		UGameplayStatics::PlaySound2D(this, GrowSound, 0.8f);
	}
	RefreshBoard();
	OnActivated.Broadcast();
	return true;
}

FString ANFGreenhouse::GetSummaryText() const
{
	if (!bActivated)
	{
		return TEXT("Entrez : votre modèle va décider du sort de chaque plante.");
	}
	return FString::Printf(TEXT("Ton modèle a fait pousser %d plantes sur %d.\nIl s'est trompé sur %d plante%s.\n(Dans la vraie serre, %d plantes survivent.)"),
		GrownCount, Plants.Num(), MistakeCount, MistakeCount > 1 ? TEXT("s") : TEXT(""), ReallyAliveCount);
}

void ANFGreenhouse::RefreshBoard()
{
	if (!BoardText)
	{
		return;
	}
	const bool bReady = Builder && Builder->GetTrainingState() == ENFTrainingState::Succeeded;
	BoardText->SetText(FText::FromString(bActivated || bReady ? GetSummaryText()
		: TEXT("Validez d'abord le modèle pour faire pousser les plantes.")));
}

// ===========================================================================
// Lucioles

ANFFireflies::ANFFireflies()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	Swarm = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Swarm"));
	Swarm->SetupAttachment(RootComponent);
	Swarm->SetStaticMesh(SphereMesh.Object);
	Swarm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Swarm->SetCastShadow(false);
}

void ANFFireflies::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (GlowMaterial)
	{
		Swarm->SetMaterial(0, GlowMaterial);
	}

	FRandomStream Rng(777);
	Flies.Reset();
	Swarm->ClearInstances();
	for (int32 i = 0; i < Count; ++i)
	{
		FFirefly Fly;
		Fly.Home = FVector(Rng.FRandRange(-Extent.X, Extent.X), Rng.FRandRange(-Extent.Y, Extent.Y), Rng.FRandRange(-Extent.Z, Extent.Z));
		Fly.Frequency = FVector(Rng.FRandRange(0.1f, 0.35f), Rng.FRandRange(0.1f, 0.35f), Rng.FRandRange(0.2f, 0.5f));
		Fly.Phase = FVector(Rng.FRandRange(0.f, 6.28f), Rng.FRandRange(0.f, 6.28f), Rng.FRandRange(0.f, 6.28f));
		Fly.Blink = Rng.FRandRange(0.f, 6.28f);
		Flies.Add(Fly);
		Swarm->AddInstance(FTransform(Fly.Home));
	}
}

void ANFFireflies::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Time = GetWorld()->GetTimeSeconds();
	const float Scale = Size / 100.f;
	for (int32 i = 0; i < Flies.Num(); ++i)
	{
		const FFirefly& Fly = Flies[i];
		const FVector Offset(60.f * FMath::Sin(Time * Fly.Frequency.X + Fly.Phase.X),
			60.f * FMath::Sin(Time * Fly.Frequency.Y + Fly.Phase.Y),
			25.f * FMath::Sin(Time * Fly.Frequency.Z + Fly.Phase.Z));
		// Scintillement : la luciole grossit et rapetisse doucement
		const float Glow = 0.4f + 0.6f * FMath::Max(0.f, FMath::Sin(Time * 1.7f + Fly.Blink));
		Swarm->UpdateInstanceTransform(i, FTransform(FRotator::ZeroRotator, Fly.Home + Offset, FVector(Scale * Glow)), false, false, true);
	}
	Swarm->MarkRenderStateDirty();
}

// ===========================================================================
// Rampe d'arrosage

ANFSprinklerRow::ANFSprinklerRow()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));

	// Tuyau horizontal le long de Y
	Pipe = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pipe"));
	Pipe->SetupAttachment(RootComponent);
	Pipe->SetStaticMesh(CylinderMesh.Object);
	Pipe->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
	Pipe->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Drops = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Drops"));
	Drops->SetupAttachment(RootComponent);
	Drops->SetStaticMesh(SphereMesh.Object);
	Drops->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Drops->SetCastShadow(false);
}

void ANFSprinklerRow::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Pipe->SetRelativeScale3D(FVector(0.06f, 0.06f, Length / 100.f));
	if (PipeMaterial)
	{
		Pipe->SetMaterial(0, PipeMaterial);
	}
	if (DropMaterial)
	{
		Drops->SetMaterial(0, DropMaterial);
	}
	RebuildDrops();
}

void ANFSprinklerRow::RebuildDrops()
{
	FRandomStream Rng(FMath::RoundToInt32(Water * 101.f) + 7);
	DropSeeds.Reset();
	Drops->ClearInstances();
	const int32 Count = FMath::RoundToInt32(Water * 6.f);
	for (int32 i = 0; i < Count; ++i)
	{
		DropSeeds.Add(FVector2D(Rng.FRandRange(-0.5f, 0.5f) * Length, Rng.FRand()));
		Drops->AddInstance(FTransform(FVector(0.f, DropSeeds.Last().X, 0.f)));
	}
}

void ANFSprinklerRow::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Time = GetWorld()->GetTimeSeconds();
	for (int32 i = 0; i < DropSeeds.Num(); ++i)
	{
		// Chute accélérée, en boucle, chaque goutte avec son propre décalage
		const float T = FMath::Frac(Time * 0.9f + DropSeeds[i].Y);
		const FVector Location(0.f, DropSeeds[i].X, -DropHeight * T * T);
		Drops->UpdateInstanceTransform(i, FTransform(FRotator::ZeroRotator, Location, FVector(0.02f, 0.02f, 0.05f)), false, false, true);
	}
	Drops->MarkRenderStateDirty();
}
