// Neuron Forge - ENSIM

#include "Lab/NFGreenhousePlant.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "NeuralNetwork/NFDataset.h"
#include "UObject/ConstructorHelpers.h"

namespace NFGreenhouseLook
{
	const FLinearColor SproutLeaf(0.30f, 0.62f, 0.25f);
	const FLinearColor HealthyLeaf(0.10f, 0.78f, 0.20f);
	const FLinearColor WiltedLeaf(0.40f, 0.26f, 0.08f);
	const FLinearColor HealthyFlower(1.f, 0.42f, 0.70f);
	const FLinearColor WiltedFlower(0.30f, 0.22f, 0.16f);

	UStaticMeshComponent* MakePart(AActor* Owner, USceneComponent* Parent, const FName Name, UStaticMesh* Mesh,
		const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(Mesh);
		Part->SetRelativeLocationAndRotation(Location, Rotation);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		return Part;
	}

	float SmoothStep(float X)
	{
		const float T = FMath::Clamp(X, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}
}

ANFGreenhousePlant::ANFGreenhousePlant()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Pot de 36 cm, terre à 34 cm du sol
	Pot = NFGreenhouseLook::MakePart(this, SceneRoot, TEXT("Pot"), CylinderMesh.Object, FVector(0.f, 0.f, 16.f), FVector(0.36f, 0.36f, 0.32f));
	Pot->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Soil = NFGreenhouseLook::MakePart(this, SceneRoot, TEXT("Soil"), CylinderMesh.Object, FVector(0.f, 0.f, 32.5f), FVector(0.33f, 0.33f, 0.02f));

	PlantPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PlantPivot"));
	PlantPivot->SetupAttachment(SceneRoot);
	PlantPivot->SetRelativeLocation(FVector(0.f, 0.f, 33.f));

	Stem = NFGreenhouseLook::MakePart(this, PlantPivot, TEXT("Stem"), CylinderMesh.Object, FVector(0.f, 0.f, 35.f), FVector(0.04f, 0.04f, 0.7f));
	const FVector LeafPositions[] = { FVector(12.f, 0.f, 22.f), FVector(-12.f, 0.f, 34.f), FVector(0.f, 12.f, 46.f), FVector(0.f, -12.f, 56.f) };
	for (int32 i = 0; i < UE_ARRAY_COUNT(LeafPositions); ++i)
	{
		const FVector& P = LeafPositions[i];
		const FRotator Tilt(P.X != 0.f ? 25.f * FMath::Sign(P.X) : 0.f, 0.f, P.Y != 0.f ? -25.f * FMath::Sign(P.Y) : 0.f);
		Leaves.Add(NFGreenhouseLook::MakePart(this, PlantPivot, *FString::Printf(TEXT("Leaf%d"), i), SphereMesh.Object, P,
			FVector(P.X != 0.f ? 0.26f : 0.1f, P.Y != 0.f ? 0.26f : 0.1f, 0.035f), Tilt));
	}
	Flower = NFGreenhouseLook::MakePart(this, PlantPivot, TEXT("Flower"), SphereMesh.Object, FVector(0.f, 0.f, 72.f), FVector(0.1f));
	for (int32 i = 0; i < 6; ++i)
	{
		const float Angle = 2.f * PI * i / 6.f;
		Petals.Add(NFGreenhouseLook::MakePart(this, Flower, *FString::Printf(TEXT("Petal%d"), i), SphereMesh.Object,
			FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 60.f, FVector(0.7f, 0.45f, 0.18f), FRotator(0.f, FMath::RadiansToDegrees(Angle), 0.f)));
	}
}

void ANFGreenhousePlant::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (PotMaterial)
	{
		Pot->SetMaterial(0, PotMaterial);
	}
	if (PlantMaterial)
	{
		for (UStaticMeshComponent* Part : { Stem.Get(), Flower.Get(), Soil.Get() })
		{
			Part->SetMaterial(0, PlantMaterial);
		}
		for (UStaticMeshComponent* Part : Leaves)
		{
			Part->SetMaterial(0, PlantMaterial);
		}
		for (UStaticMeshComponent* Part : Petals)
		{
			Part->SetMaterial(0, PlantMaterial);
		}
	}
	SwayPhase = FMath::Frac((Water * 7.31f + Light * 3.17f) * 0.13f) * 2.f * PI;
	ResetPlant();
}

void ANFGreenhousePlant::ResetPlant()
{
	bRevealed = false;
	Progress = 0.f;
	ApplyLook(0.f, 0.6f, 0.f);
}

bool ANFGreenhousePlant::ReallySurvives() const
{
	return FNFDataset::GreenhouseSurvives(Water, Light);
}

void ANFGreenhousePlant::Reveal(bool bInPredictedAlive, float InPrediction, float Delay)
{
	bRevealed = true;
	bPredictedAlive = bInPredictedAlive;
	Prediction = InPrediction;
	DelayRemaining = FMath::Max(0.f, Delay);
	Progress = 0.f;
}

void ANFGreenhousePlant::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	if (bRevealed)
	{
		if (DelayRemaining > 0.f)
		{
			DelayRemaining -= DeltaSeconds;
		}
		else
		{
			Progress = FMath::Min(1.f, Progress + DeltaSeconds / GrowDuration);
		}

		const float T = NFGreenhouseLook::SmoothStep(Progress);
		if (bPredictedAlive)
		{
			// Pousse, puis la fleur s'ouvre dans la seconde moitié de l'animation
			ApplyLook(T, FMath::Lerp(0.6f, 1.f, T), NFGreenhouseLook::SmoothStep(Progress * 2.f - 1.f));
		}
		else
		{
			// Fane : se courbe, brunit et rapetisse
			ApplyLook(FMath::Lerp(0.f, -0.35f, T), FMath::Lerp(0.6f, 0.f, T), 0.f);
		}
	}

	// Léger balancement des plantes vivantes (brise de la serre)
	if (!bRevealed || bPredictedAlive)
	{
		const float Sway = 2.5f * FMath::Sin(Time * 1.3f + SwayPhase);
		FRotator Rotation = PlantPivot->GetRelativeRotation();
		Rotation.Pitch = Sway;
		PlantPivot->SetRelativeRotation(Rotation);
	}
}

void ANFGreenhousePlant::ApplyLook(float Growth, float Health, float Bloom)
{
	const float H = FMath::Clamp(Health, 0.f, 1.f);

	// Taille : jeune pousse 45 %, adulte 100 % ; une plante fanée se tasse
	const float Size = Growth >= 0.f ? FMath::Lerp(0.45f, 1.f, Growth) : FMath::Lerp(0.45f, 0.35f, -Growth / 0.35f);
	PlantPivot->SetRelativeScale3D(FVector(Size));

	// Courbure : une plante fanée penche jusqu'à 65°
	FRotator Rotation = PlantPivot->GetRelativeRotation();
	Rotation.Roll = (1.f - FMath::Clamp(H / 0.6f, 0.f, 1.f)) * 65.f;
	PlantPivot->SetRelativeRotation(Rotation);

	const FLinearColor Leaf = H >= 0.6f ? FMath::Lerp(NFGreenhouseLook::SproutLeaf, NFGreenhouseLook::HealthyLeaf, (H - 0.6f) / 0.4f) : FMath::Lerp(NFGreenhouseLook::WiltedLeaf, NFGreenhouseLook::SproutLeaf, H / 0.6f);
	for (UStaticMeshComponent* Part : Leaves)
	{
		Part->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), FVector(Leaf));
		Part->SetVectorParameterValueOnMaterials(TEXT("EmissiveColor"), FVector(Leaf * 0.35f));
	}
	Stem->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), FVector(Leaf * 0.8f));
	Stem->SetVectorParameterValueOnMaterials(TEXT("EmissiveColor"), FVector(Leaf * 0.25f));

	// Fleur : bouton au départ, s'ouvre avec Bloom, devient terne si la plante fane
	const float B = FMath::Clamp(Bloom, 0.f, 1.f);
	const FLinearColor FlowerColor = FMath::Lerp(NFGreenhouseLook::WiltedFlower, NFGreenhouseLook::HealthyFlower, FMath::Max(B, H >= 0.6f ? 0.3f : 0.f));
	Flower->SetRelativeScale3D(FVector(FMath::Lerp(0.07f, 0.12f, B)));
	Flower->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), FVector(FMath::Lerp(FlowerColor, FLinearColor(1.f, 0.85f, 0.2f), B)));
	Flower->SetVectorParameterValueOnMaterials(TEXT("EmissiveColor"), FVector(FlowerColor * (0.1f + 0.6f * B)));
	for (UStaticMeshComponent* Petal : Petals)
	{
		Petal->SetVisibility(B > 0.02f);
		Petal->SetRelativeScale3D(FVector(0.7f, 0.45f, 0.18f) * FMath::Max(B, 0.01f));
		Petal->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), FVector(NFGreenhouseLook::HealthyFlower));
		Petal->SetVectorParameterValueOnMaterials(TEXT("EmissiveColor"), FVector(NFGreenhouseLook::HealthyFlower * 0.5f * B));
	}

	// Terre : plus sombre quand la plante est bien arrosée
	const FLinearColor SoilColor = FMath::Lerp(FLinearColor(0.35f, 0.25f, 0.15f), FLinearColor(0.08f, 0.05f, 0.03f), Water / 10.f);
	Soil->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), FVector(SoilColor));
	Soil->SetVectorParameterValueOnMaterials(TEXT("EmissiveColor"), FVector(0.f));
}
