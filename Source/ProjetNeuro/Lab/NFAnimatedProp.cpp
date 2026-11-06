// Neuron Forge - ENSIM

#include "Lab/NFAnimatedProp.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Lab/NFNetworkBuilder.h"

ANFAnimatedProp::ANFAnimatedProp()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);

	Orbit = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Orbit"));
	Orbit->SetupAttachment(SceneRoot);
	Orbit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Orbit->SetCastShadow(false);
}

void ANFAnimatedProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildOrbit();
}

void ANFAnimatedProp::BeginPlay()
{
	Super::BeginPlay();
	MeshRestLocation = Mesh->GetRelativeLocation();
	RebuildOrbit();
}

void ANFAnimatedProp::RebuildOrbit()
{
	Orbit->ClearInstances();
	for (int32 i = 0; i < OrbitCount; ++i)
	{
		Orbit->AddInstance(FTransform::Identity);
	}
	UpdateOrbit(OrbitAngle, 0.f);
}

void ANFAnimatedProp::UpdateOrbit(float Angle, float Time)
{
	const float Scale = OrbitItemSize / 100.f;	// les formes de base font 100 cm
	for (int32 i = 0; i < OrbitCount; ++i)
	{
		const float Phase = 2.f * PI * i / FMath::Max(1, OrbitCount);
		const float Theta = FMath::DegreesToRadians(Angle) + Phase;
		const FVector Location(OrbitRadius * FMath::Cos(Theta), OrbitRadius * FMath::Sin(Theta),
			OrbitWave * FMath::Sin(Time * 1.3f + Phase * 3.f));
		const FRotator Spin(Time * 40.f + i * 17.f, Time * 60.f + i * 31.f, 0.f);
		Orbit->UpdateInstanceTransform(i, FTransform(Spin, Location, FVector(Scale)), false, false, true);
	}
	Orbit->MarkRenderStateDirty();
}

void ANFAnimatedProp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Accélère en douceur pendant l'entraînement
	const bool bTraining = Builder && Builder->GetTrainingState() == ENFTrainingState::Training;
	SpeedFactor = FMath::FInterpTo(SpeedFactor, bTraining ? TrainingSpeedMultiplier : 1.f, DeltaSeconds, 2.f);

	const float Time = GetWorld()->GetTimeSeconds();
	Mesh->AddRelativeRotation(RotationRate * DeltaSeconds * SpeedFactor);
	if (BobAmplitude > 0.f)
	{
		Mesh->SetRelativeLocation(MeshRestLocation + FVector(0.f, 0.f, BobAmplitude * FMath::Sin(Time * BobFrequency * 2.f * PI)));
	}

	if (OrbitCount > 0)
	{
		OrbitAngle = FMath::Fmod(OrbitAngle + OrbitSpeed * DeltaSeconds * SpeedFactor, 360.f);
		UpdateOrbit(OrbitAngle, Time);
	}
}
