// Neuron Forge - ENSIM

#include "Lab/NFPlantPot.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Interaction/NFInteractableActor.h"
#include "Kismet/GameplayStatics.h"
#include "Lab/NFNetworkBuilder.h"
#include "Materials/MaterialInterface.h"
#include "NeuralNetwork/NFDataset.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "NeuronForgePlant"

namespace
{
	const FLinearColor HealthyLeaf(0.12f, 0.75f, 0.22f);
	const FLinearColor WiltedLeaf(0.42f, 0.27f, 0.08f);
	const FLinearColor HealthyFlower(1.f, 0.45f, 0.75f);
	const FLinearColor WiltedFlower(0.35f, 0.25f, 0.2f);

	UStaticMeshComponent* MakePart(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh,
		const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(Mesh);
		Part->SetRelativeLocationAndRotation(Location, Rotation);
		Part->SetRelativeScale3D(Scale);
		return Part;
	}

	UTextRenderComponent* MakeText(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, float Size)
	{
		UTextRenderComponent* Text = Owner->CreateDefaultSubobject<UTextRenderComponent>(Name);
		Text->SetupAttachment(Parent);
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetWorldSize(Size);
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Text;
	}
}

// ===========================================================================
// Commande (arrosage / lampe)

ANFPlantControl::ANFPlantControl()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));

	// Colonne de 110 cm surmontée d'une molette, face avant vers +X
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	RootComponent = Base;
	Base->SetStaticMesh(CubeMesh.Object);
	Base->SetRelativeScale3D(FVector(0.35f, 0.35f, 1.1f));

	Knob = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Knob"));
	Knob->SetupAttachment(Base);
	Knob->SetAbsolute(false, false, true);
	Knob->SetStaticMesh(CylinderMesh.Object);
	Knob->SetRelativeLocation(FVector(0.f, 0.f, 50.f + 4.f / 1.1f));
	Knob->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.08f));

	Label = MakeText(this, Base, TEXT("Label"), 9.f);
	Label->SetAbsolute(false, false, true);
	Label->SetRelativeLocation(FVector(52.f, 0.f, 20.f));
}

void ANFPlantControl::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (BodyMaterial)
	{
		Base->SetMaterial(0, BodyMaterial);
	}
	if (KnobMaterial)
	{
		Knob->SetMaterial(0, KnobMaterial);
	}
	const FLinearColor Color = Setting == ENFPlantSetting::Water ? FLinearColor(0.2f, 0.6f, 1.f) : FLinearColor(1.f, 0.8f, 0.25f);
	Knob->SetVectorParameterValueOnMaterials(TEXT("EmissiveColor"), FVector(Color));
	RefreshLabel();
}

int32 ANFPlantControl::GetValue() const
{
	return Pot ? Pot->GetValue(Setting) : 0;
}

void ANFPlantControl::RefreshLabel()
{
	const bool bWater = Setting == ENFPlantSetting::Water;
	Label->SetText(FText::FromString(FString::Printf(TEXT("%s\n%d / 10"), bWater ? TEXT("ARROSAGE") : TEXT("LAMPE"), GetValue())));
	Label->SetTextRenderColor(bWater ? FColor(120, 190, 255) : FColor(255, 215, 110));
}

void ANFPlantControl::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// La molette tourne selon la valeur réglée
	KnobAngle = FMath::FInterpTo(KnobAngle, GetValue() * 30.f, DeltaSeconds, 8.f);
	Knob->SetRelativeRotation(FRotator(0.f, KnobAngle, 0.f));
	RefreshLabel();
}

bool ANFPlantControl::CanInteract_Implementation(AActor* InstigatorActor) const
{
	return Pot && Pot->IsUnlocked();
}

void ANFPlantControl::Interact_Implementation(AActor* InstigatorActor)
{
	if (Pot && Pot->StepSetting(Setting) && ClickSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ClickSound, GetActorLocation());
	}
}

void ANFPlantControl::OnFocusBegin_Implementation(AActor* InstigatorActor)
{
	Knob->SetScalarParameterValueOnMaterials(ANFInteractableActor::HighlightParameterName, 1.f);
}

void ANFPlantControl::OnFocusEnd_Implementation(AActor* InstigatorActor)
{
	Knob->SetScalarParameterValueOnMaterials(ANFInteractableActor::HighlightParameterName, 0.f);
}

FText ANFPlantControl::GetInteractionPrompt_Implementation() const
{
	if (!Pot || !Pot->IsUnlocked())
	{
		return LOCTEXT("Locked", "Validez d'abord le modèle pour tester une plante");
	}
	const int32 Value = GetValue();
	return FText::Format(Setting == ENFPlantSetting::Water ? LOCTEXT("Water", "Arrosage : {0} (appuyez pour passer à {1})")
		: LOCTEXT("Light", "Lampe : {0} (appuyez pour passer à {1})"),
		FText::AsNumber(Value), FText::AsNumber((Value + 1) % 11));
}

// ===========================================================================
// Pot de plante

ANFPlantPot::ANFPlantPot()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Socle 120 x 120 x 70 cm, pot de 50 cm de diamètre, terre affleurante
	Pedestal = MakePart(this, SceneRoot, TEXT("Pedestal"), CubeMesh.Object, FVector(0.f, 0.f, 35.f), FVector(1.2f, 1.2f, 0.7f));
	PotMesh = MakePart(this, SceneRoot, TEXT("Pot"), CylinderMesh.Object, FVector(0.f, 0.f, 92.f), FVector(0.5f, 0.5f, 0.44f));
	Soil = MakePart(this, SceneRoot, TEXT("Soil"), CylinderMesh.Object, FVector(0.f, 0.f, 113.f), FVector(0.46f, 0.46f, 0.03f));

	PlantPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PlantPivot"));
	PlantPivot->SetupAttachment(SceneRoot);
	PlantPivot->SetRelativeLocation(FVector(0.f, 0.f, 115.f));

	Stem = MakePart(this, PlantPivot, TEXT("Stem"), CylinderMesh.Object, FVector(0.f, 0.f, 40.f), FVector(0.05f, 0.05f, 0.8f));
	const FVector LeafPositions[] = { FVector(14.f, 0.f, 30.f), FVector(-14.f, 0.f, 42.f), FVector(0.f, 14.f, 54.f), FVector(0.f, -14.f, 64.f) };
	for (int32 i = 0; i < UE_ARRAY_COUNT(LeafPositions); ++i)
	{
		const FRotator Tilt(LeafPositions[i].X != 0.f ? 25.f * FMath::Sign(LeafPositions[i].X) : 0.f, 0.f,
			LeafPositions[i].Y != 0.f ? -25.f * FMath::Sign(LeafPositions[i].Y) : 0.f);
		Leaves.Add(MakePart(this, PlantPivot, *FString::Printf(TEXT("Leaf%d"), i), SphereMesh.Object, LeafPositions[i],
			FVector(LeafPositions[i].X != 0.f ? 0.3f : 0.12f, LeafPositions[i].Y != 0.f ? 0.3f : 0.12f, 0.04f), Tilt));
	}
	Flower = MakePart(this, PlantPivot, TEXT("Flower"), SphereMesh.Object, FVector(0.f, 0.f, 84.f), FVector(0.16f));

	for (UStaticMeshComponent* Part : { Stem.Get(), Flower.Get() })
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	for (UStaticMeshComponent* Leaf : Leaves)
	{
		Leaf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Lampe de culture au-dessus de la plante
	Lamp = MakePart(this, SceneRoot, TEXT("Lamp"), ConeMesh.Object, FVector(0.f, 0.f, 290.f), FVector(0.45f, 0.45f, 0.25f), FRotator(180.f, 0.f, 0.f));
	LampLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LampLight"));
	LampLight->SetupAttachment(SceneRoot);
	LampLight->SetRelativeLocation(FVector(0.f, 0.f, 270.f));
	LampLight->SetIntensityUnits(ELightUnits::Lumens);
	LampLight->SetAttenuationRadius(400.f);
	LampLight->SetLightColor(FLinearColor(1.f, 0.85f, 0.55f));
	LampLight->SetCastShadows(false);

	Title = MakeText(this, SceneRoot, TEXT("Title"), 20.f);
	Title->SetRelativeLocation(FVector(62.f, 0.f, 360.f));
	Title->SetText(LOCTEXT("Title", "TESTEZ VOTRE MODÈLE"));
	Title->SetTextRenderColor(FColor(140, 220, 255));

	Readout = MakeText(this, SceneRoot, TEXT("Readout"), 10.f);
	Readout->SetRelativeLocation(FVector(62.f, 0.f, 45.f));
}

void ANFPlantPot::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (PotMaterial)
	{
		PotMesh->SetMaterial(0, PotMaterial);
	}
	if (PedestalMaterial)
	{
		Pedestal->SetMaterial(0, PedestalMaterial);
		Lamp->SetMaterial(0, PedestalMaterial);
	}
	if (PlantMaterial)
	{
		for (UStaticMeshComponent* Part : { Stem.Get(), Flower.Get(), Soil.Get() })
		{
			Part->SetMaterial(0, PlantMaterial);
		}
		for (UStaticMeshComponent* Leaf : Leaves)
		{
			Leaf->SetMaterial(0, PlantMaterial);
		}
	}
	ApplyPlantLook(DisplayedHealth);
}

void ANFPlantPot::BeginPlay()
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
	UpdatePrediction();
}

bool ANFPlantPot::IsUnlocked() const
{
	return Builder && Builder->GetTrainingState() == ENFTrainingState::Succeeded;
}

bool ANFPlantPot::StepSetting(ENFPlantSetting Setting)
{
	if (!IsUnlocked())
	{
		return false;
	}
	int32& Value = (Setting == ENFPlantSetting::Water) ? Water : Light;
	Value = (Value + 1) % 11;
	bTested = true;
	UpdatePrediction();
	return true;
}

void ANFPlantPot::SetConditions(int32 InWater, int32 InLight)
{
	Water = FMath::Clamp(InWater, 0, 10);
	Light = FMath::Clamp(InLight, 0, 10);
	if (IsUnlocked())
	{
		bTested = true;
	}
	UpdatePrediction();
}

bool ANFPlantPot::WouldReallySurvive() const
{
	return FNFDataset::GreenhouseSurvives(static_cast<float>(Water), static_cast<float>(Light));
}

void ANFPlantPot::UpdatePrediction()
{
	UNFTrainerComponent* Trainer = Builder ? Builder->GetTrainer() : nullptr;
	if (!IsUnlocked() || !Trainer)
	{
		Prediction = -1.f;
	}
	else
	{
		const FNFDataset& Data = Trainer->GetDataset();
		Prediction = Trainer->Predict(Data.Normalize(static_cast<float>(Water)), Data.Normalize(static_cast<float>(Light)));
	}

	if (!Readout)
	{
		return;
	}

	if (Prediction < 0.f)
	{
		Readout->SetText(LOCTEXT("LockedReadout", "Le pot s'active quand le modèle est validé.\nRevenez après avoir réussi l'entraînement !"));
		Readout->SetTextRenderColor(FColor(170, 185, 200));
		return;
	}

	const bool bPredictedAlive = Prediction >= 0.5f;
	const bool bReallyAlive = WouldReallySurvive();
	const FString Text = FString::Printf(
		TEXT("Eau : %d    Lumière : %d\nLe modèle pense que la plante a %.0f %% de chances de survivre\nVerdict : %s\nDans la vraie serre : elle %s  (%s)"),
		Water, Light, Prediction * 100.f,
		bPredictedAlive ? TEXT("la plante est en bonne santé") : TEXT("la plante fane"),
		bReallyAlive ? TEXT("survivrait") : TEXT("mourrait"),
		bPredictedAlive == bReallyAlive ? TEXT("le modèle a raison") : TEXT("le modèle se trompe !"));
	Readout->SetText(FText::FromString(Text));
	Readout->SetTextRenderColor(bPredictedAlive ? FColor(120, 255, 150) : FColor(255, 150, 110));
}

void ANFPlantPot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Le modèle peut changer (nouvel entraînement) : on relit régulièrement sa prédiction
	RefreshTimer += DeltaSeconds;
	if (RefreshTimer >= 0.25f)
	{
		RefreshTimer = 0.f;
		UpdatePrediction();
	}

	const float TargetHealth = Prediction < 0.f ? 0.5f : Prediction;
	DisplayedHealth = FMath::FInterpTo(DisplayedHealth, TargetHealth, DeltaSeconds, 2.5f);
	ApplyPlantLook(DisplayedHealth);

	// La lampe suit le réglage de lumière
	LampLight->SetIntensity(Light * 180.f);
}

void ANFPlantPot::ApplyPlantLook(float Health)
{
	const float H = FMath::Clamp(Health, 0.f, 1.f);

	// Plante penchée et rabougrie quand elle fane, droite quand elle est en bonne santé
	PlantPivot->SetRelativeRotation(FRotator(0.f, 0.f, (1.f - H) * 55.f));
	PlantPivot->SetRelativeScale3D(FVector(FMath::Lerp(0.75f, 1.f, H)));

	const FLinearColor Leaf = FMath::Lerp(WiltedLeaf, HealthyLeaf, H);
	const FLinearColor Bloom = FMath::Lerp(WiltedFlower, HealthyFlower, H);
	for (UStaticMeshComponent* Part : Leaves)
	{
		Part->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), FVector(Leaf));
		Part->SetVectorParameterValueOnMaterials(TEXT("EmissiveColor"), FVector(Leaf * 0.4f));
	}
	Stem->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), FVector(Leaf * 0.8f));
	Stem->SetVectorParameterValueOnMaterials(TEXT("EmissiveColor"), FVector(Leaf * 0.3f));
	Flower->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), FVector(Bloom));
	Flower->SetVectorParameterValueOnMaterials(TEXT("EmissiveColor"), FVector(Bloom * FMath::Lerp(0.1f, 0.8f, H)));
	Flower->SetRelativeScale3D(FVector(FMath::Lerp(0.08f, 0.16f, H)));

	// Terre sèche (claire) ou humide (sombre) selon l'arrosage
	const float Wet = Water / 10.f;
	const FLinearColor SoilColor = FMath::Lerp(FLinearColor(0.35f, 0.25f, 0.15f), FLinearColor(0.08f, 0.05f, 0.03f), Wet);
	Soil->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), FVector(SoilColor));
	Soil->SetVectorParameterValueOnMaterials(TEXT("EmissiveColor"), FVector(0.f));
}

#undef LOCTEXT_NAMESPACE
