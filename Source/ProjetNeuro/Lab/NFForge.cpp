// Neuron Forge - ENSIM

#include "Lab/NFForge.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Lab/NFLayerModule.h"
#include "Lab/NFNetworkBuilder.h"
#include "Lab/NFSocketComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "NeuronForgeForge"

namespace
{
	constexpr float PlatformHeight = 70.f;
	constexpr float PlatformDiameter = 130.f;
	constexpr float PylonHeight = 240.f;

	UTextRenderComponent* MakeLabel(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, float Size)
	{
		UTextRenderComponent* Label = Owner->CreateDefaultSubobject<UTextRenderComponent>(Name);
		Label->SetupAttachment(Parent);
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetVerticalAlignment(EVRTA_TextCenter);
		Label->SetWorldSize(Size);
		Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Label;
	}

	bool SameConfiguration(const TArray<FNFLayerConfig>& A, const TArray<FNFLayerConfig>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 i = 0; i < A.Num(); ++i)
		{
			if (A[i].NumNeurons != B[i].NumNeurons || A[i].Activation != B[i].Activation)
			{
				return false;
			}
		}
		return true;
	}
}

ANFForge::ANFForge()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));

	Rail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rail"));
	Rail->SetupAttachment(SceneRoot);
	Rail->SetStaticMesh(CubeMesh.Object);

	InputPylon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InputPylon"));
	InputPylon->SetupAttachment(SceneRoot);
	InputPylon->SetStaticMesh(CylinderMesh.Object);

	OutputPylon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OutputPylon"));
	OutputPylon->SetupAttachment(SceneRoot);
	OutputPylon->SetStaticMesh(CylinderMesh.Object);

	for (int32 i = 0; i < NumBays; ++i)
	{
		UStaticMeshComponent* Platform = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Platform%d"), i));
		Platform->SetupAttachment(SceneRoot);
		Platform->SetStaticMesh(CylinderMesh.Object);
		Platforms.Add(Platform);

		UNFSocketComponent* Bay = CreateDefaultSubobject<UNFSocketComponent>(*FString::Printf(TEXT("Bay%d"), i));
		Bay->SetupAttachment(SceneRoot);
		Bay->AcceptedType = ENFPieceType::LayerModule;
		Bay->SnapRadius = 160.f;
		Bay->SocketName = FText::Format(LOCTEXT("BayName", "la fente {0}"), FText::AsNumber(i + 1));
		Bays.Add(Bay);

		BayLabels.Add(MakeLabel(this, SceneRoot, *FString::Printf(TEXT("BayLabel%d"), i), 13.f));
	}

	InputLabel = MakeLabel(this, SceneRoot, TEXT("InputLabel"), 15.f);
	OutputLabel = MakeLabel(this, SceneRoot, TEXT("OutputLabel"), 15.f);
	InputLabel->SetText(LOCTEXT("Input", "ENTRÉES\nx1  x2"));
	OutputLabel->SetText(LOCTEXT("Output", "SORTIE\nXOR"));
	InputLabel->SetTextRenderColor(FColor(230, 230, 230));
	OutputLabel->SetTextRenderColor(FColor(120, 255, 160));
}

void ANFForge::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutForge();
	TArray<FNFLayerConfig> HiddenLayers;
	UpdateLabels(HiddenLayers);
}

void ANFForge::LayoutForge()
{
	// Vue de face (le joueur regarde depuis +X) : entrée à gauche (+Y), sortie à droite (-Y)
	const float HalfLength = (NumBays + 1) * 0.5f * BaySpacing;

	Rail->SetRelativeLocation(FVector(0.f, 0.f, PlatformHeight * 0.5f));
	Rail->SetRelativeScale3D(FVector(0.25f, 2.f * HalfLength / 100.f, 0.18f));

	InputPylon->SetRelativeLocation(FVector(0.f, HalfLength, PylonHeight * 0.5f));
	InputPylon->SetRelativeScale3D(FVector(0.7f, 0.7f, PylonHeight / 100.f));
	OutputPylon->SetRelativeLocation(FVector(0.f, -HalfLength, PylonHeight * 0.5f));
	OutputPylon->SetRelativeScale3D(FVector(0.7f, 0.7f, PylonHeight / 100.f));

	InputLabel->SetRelativeLocation(FVector(45.f, HalfLength, PylonHeight + 30.f));
	OutputLabel->SetRelativeLocation(FVector(45.f, -HalfLength, PylonHeight + 30.f));

	for (int32 i = 0; i < NumBays; ++i)
	{
		const float Y = HalfLength - (i + 1) * BaySpacing;
		Platforms[i]->SetRelativeLocation(FVector(0.f, Y, PlatformHeight * 0.5f));
		Platforms[i]->SetRelativeScale3D(FVector(PlatformDiameter / 100.f, PlatformDiameter / 100.f, PlatformHeight / 100.f));
		Bays[i]->SetRelativeLocation(FVector(0.f, Y, PlatformHeight + 26.f));
		BayLabels[i]->SetRelativeLocation(FVector(PlatformDiameter * 0.5f + 4.f, Y, PlatformHeight * 0.45f));
	}

	if (PlatformMaterial)
	{
		for (UStaticMeshComponent* Platform : Platforms)
		{
			Platform->SetMaterial(0, PlatformMaterial);
		}
	}
	if (RailMaterial)
	{
		Rail->SetMaterial(0, RailMaterial);
	}
	if (PylonMaterial)
	{
		InputPylon->SetMaterial(0, PylonMaterial);
		OutputPylon->SetMaterial(0, PylonMaterial);
	}
}

void ANFForge::BeginPlay()
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
	if (Builder && Builder->IsGreenhouse())
	{
		InputLabel->SetText(LOCTEXT("GreenhouseInput", "ENTRÉES\nEau · Lumière"));
		OutputLabel->SetText(LOCTEXT("GreenhouseOutput", "SORTIE\nSurvie"));
	}
	RefreshAssembly();
}

void ANFForge::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RefreshTimer += DeltaSeconds;
	if (RefreshTimer >= 0.1f)
	{
		RefreshTimer = 0.f;
		RefreshAssembly();
	}
}

ANFLayerModule* ANFForge::GetModuleInBay(int32 Index) const
{
	const UNFSocketComponent* Bay = GetBay(Index);
	return Bay ? Cast<ANFLayerModule>(Bay->GetPiece()) : nullptr;
}

void ANFForge::BuildConfiguration(TArray<FNFLayerConfig>& OutLayers) const
{
	OutLayers.Reset();
	for (int32 i = 0; i < NumBays; ++i)
	{
		const ANFLayerModule* Module = GetModuleInBay(i);
		if (Module && Module->GetNeuronCount() > 0)
		{
			OutLayers.Emplace(Module->GetNeuronCount(), Module->GetLayerActivation());
		}
	}
}

void ANFForge::RefreshAssembly()
{
	TArray<FNFLayerConfig> HiddenLayers;
	BuildConfiguration(HiddenLayers);
	UpdateLabels(HiddenLayers);

	if (Builder && Builder->CanEdit() && !SameConfiguration(HiddenLayers, Builder->GetHiddenLayers()))
	{
		Builder->SetConfiguration(HiddenLayers, Builder->LearningRateIndex);
	}
}

void ANFForge::UpdateLabels(const TArray<FNFLayerConfig>& HiddenLayers)
{
	for (int32 i = 0; i < NumBays; ++i)
	{
		UTextRenderComponent* Label = BayLabels.IsValidIndex(i) ? BayLabels[i].Get() : nullptr;
		if (!Label)
		{
			continue;
		}

		const ANFLayerModule* Module = GetModuleInBay(i);
		FString Text;
		FColor Color(150, 170, 190);
		if (!Module)
		{
			Text = FString::Printf(TEXT("FENTE %d\nvide"), i + 1);
		}
		else if (Module->GetNeuronCount() == 0)
		{
			Text = FString::Printf(TEXT("FENTE %d\najoutez des orbes"), i + 1);
			Color = FColor(255, 130, 90);
		}
		else
		{
			Text = FString::Printf(TEXT("COUCHE %d\n%d %s"), i + 1, Module->GetNeuronCount(), *NFActivation::ToString(Module->GetLayerActivation()));
			Color = NFActivation::GetColor(Module->GetLayerActivation()).ToFColor(true);
		}
		Label->SetText(FText::FromString(Text));
		Label->SetTextRenderColor(Color);
	}
}

#undef LOCTEXT_NAMESPACE
