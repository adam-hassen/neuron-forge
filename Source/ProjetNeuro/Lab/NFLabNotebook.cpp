// Neuron Forge - ENSIM

#include "Lab/NFLabNotebook.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Lab/NFNetworkBuilder.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Panneau : graphique carré à gauche (vu de face), légende à droite
	constexpr float BoardWidth = 860.f;
	constexpr float BoardHeight = 520.f;
	constexpr float ChartCenterY = 185.f;	// +Y = gauche vu depuis +X
	constexpr float LegendLeftY = -55.f;

	const FColor NeutralColor(18, 26, 36);
	const FColor GridColor(40, 55, 72);
	const FColor AliveZone(24, 92, 48);
	const FColor DeadZone(92, 30, 30);
	const FColor BoundaryColor(255, 215, 70);
	const FColor AlivePoint(80, 255, 120);
	const FColor DeadPoint(255, 80, 70);
	const FColor PointOutline(8, 10, 14);
	const FColor MistakeRing(255, 255, 255);

	FColor LerpColor(const FColor& A, const FColor& B, float Alpha)
	{
		return FLinearColor::LerpUsingHSV(FLinearColor(A), FLinearColor(B), FMath::Clamp(Alpha, 0.f, 1.f)).ToFColor(false);
	}

	UTextRenderComponent* MakeText(AActor* Owner, USceneComponent* Parent, const FName Name, float Size, EHorizTextAligment Align = EHTA_Center)
	{
		UTextRenderComponent* Text = Owner->CreateDefaultSubobject<UTextRenderComponent>(Name);
		Text->SetupAttachment(Parent);
		Text->SetHorizontalAlignment(Align);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetWorldSize(Size);
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Text;
	}
}

ANFLabNotebook::ANFLabNotebook()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane"));

	Board = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Board"));
	Board->SetupAttachment(SceneRoot);
	Board->SetStaticMesh(CubeMesh.Object);
	Board->SetRelativeLocation(FVector(-4.f, 0.f, 0.f));
	Board->SetRelativeScale3D(FVector(0.06f, BoardWidth / 100.f, BoardHeight / 100.f));

	// Le plan de base est horizontal (normale +Z) : on le redresse face à +X
	Chart = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chart"));
	Chart->SetupAttachment(SceneRoot);
	Chart->SetStaticMesh(PlaneMesh.Object);
	Chart->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Chart->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Chart->SetCastShadow(false);

	Title = MakeText(this, SceneRoot, TEXT("Title"), 30.f);
	Title->SetText(FText::FromString(TEXT("CARNET DU LABO")));
	Title->SetTextRenderColor(FColor(140, 220, 255));

	AxisWater = MakeText(this, SceneRoot, TEXT("AxisWater"), 16.f);
	AxisWater->SetText(FText::FromString(TEXT("EAU  (0 à 10)")));
	AxisWater->SetTextRenderColor(FColor(120, 190, 255));

	AxisLight = MakeText(this, SceneRoot, TEXT("AxisLight"), 16.f);
	AxisLight->SetText(FText::FromString(TEXT("LUMIÈRE  (0 à 10)")));
	AxisLight->SetTextRenderColor(FColor(255, 220, 110));

	const TCHAR* TickLabels[] = { TEXT("0"), TEXT("5"), TEXT("10"), TEXT("5"), TEXT("10") };
	for (int32 i = 0; i < 5; ++i)
	{
		UTextRenderComponent* Tick = MakeText(this, SceneRoot, *FString::Printf(TEXT("Tick%d"), i), 13.f);
		Tick->SetText(FText::FromString(TickLabels[i]));
		Tick->SetTextRenderColor(FColor(170, 185, 200));
		Ticks.Add(Tick);
	}

	const TCHAR* LegendLines[] = {
		TEXT("Chaque point = une plante de la serre"),
		TEXT("Point vert : la plante a survécu"),
		TEXT("Point rouge : la plante est morte"),
		TEXT("Zone verte : le modèle pense que la plante survit"),
		TEXT("Zone rouge : le modèle pense qu'elle meurt"),
		TEXT("Ligne jaune : la limite trouvée par le modèle"),
		TEXT("Point entouré de blanc : le modèle se trompe"),
	};
	const FColor LegendColors[] = { FColor(220, 235, 255), AlivePoint, DeadPoint, FColor(90, 220, 130), FColor(230, 110, 100),
		BoundaryColor, FColor(255, 255, 255) };
	for (int32 i = 0; i < UE_ARRAY_COUNT(LegendLines); ++i)
	{
		UTextRenderComponent* Line = MakeText(this, SceneRoot, *FString::Printf(TEXT("Legend%d"), i), 13.f, EHTA_Left);
		Line->SetText(FText::FromString(LegendLines[i]));
		Line->SetTextRenderColor(LegendColors[i]);
		Legend.Add(Line);
	}

	Status = MakeText(this, SceneRoot, TEXT("Status"), 15.f, EHTA_Left);
	Status->SetTextRenderColor(FColor(255, 230, 150));
}

void ANFLabNotebook::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutTexts();
	if (BoardMaterial)
	{
		Board->SetMaterial(0, BoardMaterial);
	}
	if (ChartMaterial)
	{
		Chart->SetMaterial(0, ChartMaterial);
	}
}

void ANFLabNotebook::LayoutTexts()
{
	const float Half = ChartSize * 0.5f;
	Chart->SetRelativeLocation(FVector(1.f, ChartCenterY, -20.f));
	Chart->SetRelativeScale3D(FVector(ChartSize / 100.f, ChartSize / 100.f, 1.f));

	const float ChartBottom = -20.f - Half;
	const float ChartLeftY = ChartCenterY + Half;	// gauche vu de face
	const float ChartRightY = ChartCenterY - Half;

	Title->SetRelativeLocation(FVector(2.f, 0.f, BoardHeight * 0.5f - 30.f));
	AxisWater->SetRelativeLocation(FVector(2.f, ChartCenterY, ChartBottom - 34.f));
	AxisLight->SetRelativeLocation(FVector(2.f, ChartLeftY + 42.f, -20.f));
	AxisLight->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));	// texte vertical, lu de bas en haut

	// Graduations : eau (sous le graphique) 0 / 5 / 10, lumière (à gauche) 5 / 10
	Ticks[0]->SetRelativeLocation(FVector(2.f, ChartLeftY + 10.f, ChartBottom - 12.f));
	Ticks[1]->SetRelativeLocation(FVector(2.f, ChartCenterY, ChartBottom - 12.f));
	Ticks[2]->SetRelativeLocation(FVector(2.f, ChartRightY, ChartBottom - 12.f));
	Ticks[3]->SetRelativeLocation(FVector(2.f, ChartLeftY + 12.f, -20.f));
	Ticks[4]->SetRelativeLocation(FVector(2.f, ChartLeftY + 12.f, -20.f + Half - 6.f));

	for (int32 i = 0; i < Legend.Num(); ++i)
	{
		Legend[i]->SetRelativeLocation(FVector(2.f, LegendLeftY, 150.f - i * 32.f));
	}
	Status->SetRelativeLocation(FVector(2.f, LegendLeftY, 150.f - Legend.Num() * 32.f - 30.f));
}

void ANFLabNotebook::BeginPlay()
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
	if (Builder)
	{
		Builder->OnNetworkChanged.AddUniqueDynamic(this, &ANFLabNotebook::HandleNetworkChanged);
	}
	LayoutTexts();
	EnsureTexture();
	Redraw();
}

void ANFLabNotebook::HandleNetworkChanged()
{
	bDirty = true;
}

void ANFLabNotebook::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateTimer += DeltaSeconds;
	const bool bTraining = Builder && Builder->GetTrainingState() == ENFTrainingState::Training;
	if (bDirty || (bTraining && UpdateTimer >= 1.f / UpdatesPerSecond))
	{
		UpdateTimer = 0.f;
		bDirty = false;
		Redraw();
	}
}

void ANFLabNotebook::EnsureTexture()
{
	if (ChartTexture && ChartTexture->GetSizeX() == TextureSize)
	{
		return;
	}

	ChartTexture = UTexture2D::CreateTransient(TextureSize, TextureSize, PF_B8G8R8A8, TEXT("NotebookChart"));
	ChartTexture->SRGB = true;
	ChartTexture->Filter = TF_Bilinear;
	ChartTexture->AddressX = TA_Clamp;
	ChartTexture->AddressY = TA_Clamp;
	ChartTexture->UpdateResource();

	Pixels.Init(NeutralColor, TextureSize * TextureSize);
	Background = Pixels;

	if (ChartMaterial)
	{
		ChartInstance = UMaterialInstanceDynamic::Create(ChartMaterial, this);
		ChartInstance->SetTextureParameterValue(TEXT("Chart"), ChartTexture);
		Chart->SetMaterial(0, ChartInstance);
	}
}

int32 ANFLabNotebook::PixelIndex(float U, float V) const
{
	// Coordonnées logiques : u vers la droite (eau), v vers le haut (lumière)
	float X = U;
	float Y = 1.f - V;	// ligne 0 = haut de l'image
	if (bTranspose)
	{
		Swap(X, Y);
	}
	if (bFlipHorizontal)
	{
		X = 1.f - X;
	}
	if (bFlipVertical)
	{
		Y = 1.f - Y;
	}
	const int32 PX = FMath::Clamp(FMath::FloorToInt32(X * TextureSize), 0, TextureSize - 1);
	const int32 PY = FMath::Clamp(FMath::FloorToInt32(Y * TextureSize), 0, TextureSize - 1);
	return PY * TextureSize + PX;
}

void ANFLabNotebook::PlotDisc(float U, float V, float RadiusPixels, const FColor& Color)
{
	const float Step = 1.f / TextureSize;
	const int32 R = FMath::CeilToInt32(RadiusPixels);
	for (int32 dy = -R; dy <= R; ++dy)
	{
		for (int32 dx = -R; dx <= R; ++dx)
		{
			if (dx * dx + dy * dy <= RadiusPixels * RadiusPixels)
			{
				const float PU = U + dx * Step;
				const float PV = V + dy * Step;
				if (PU >= 0.f && PU <= 1.f && PV >= 0.f && PV <= 1.f)
				{
					Pixels[PixelIndex(PU, PV)] = Color;
				}
			}
		}
	}
}

FColor ANFLabNotebook::GetBackgroundColorAt(float Water, float Light) const
{
	return Background.Num() > 0 ? Background[PixelIndex(Water / 10.f, Light / 10.f)] : NeutralColor;
}

void ANFLabNotebook::Redraw()
{
	EnsureTexture();
	UNFTrainerComponent* Trainer = Builder ? Builder->GetTrainer() : nullptr;
	if (!Trainer)
	{
		return;
	}

	const FNFDataset& Data = Trainer->GetDataset();
	const ENFTrainingState State = Builder->GetTrainingState();
	bShowPrediction = State != ENFTrainingState::Idle;

	// 1) Fond : grille neutre, ou zones prédites par le modèle (calculées sur une grille 64 x 64)
	constexpr int32 Cells = 64;
	TArray<float> Grid;
	if (bShowPrediction && Data.NumInputs == 2)
	{
		Grid.SetNumUninitialized(Cells * Cells);
		for (int32 j = 0; j < Cells; ++j)
		{
			for (int32 i = 0; i < Cells; ++i)
			{
				const float Water = (i + 0.5f) / Cells * 10.f;
				const float Light = (j + 0.5f) / Cells * 10.f;
				Grid[j * Cells + i] = Trainer->Predict(Data.Normalize(Water), Data.Normalize(Light));
			}
		}
	}

	for (int32 y = 0; y < TextureSize; ++y)
	{
		for (int32 x = 0; x < TextureSize; ++x)
		{
			const float U = (x + 0.5f) / TextureSize;
			const float V = (y + 0.5f) / TextureSize;
			FColor Color = NeutralColor;
			if (Grid.Num() > 0)
			{
				const float P = Grid[FMath::Min(FMath::FloorToInt32(V * Cells), Cells - 1) * Cells + FMath::Min(FMath::FloorToInt32(U * Cells), Cells - 1)];
				Color = P >= 0.5f ? LerpColor(NeutralColor, AliveZone, 0.4f + 1.2f * (P - 0.5f)) : LerpColor(NeutralColor, DeadZone, 0.4f + 1.2f * (0.5f - P));
				if (FMath::Abs(P - 0.5f) < 0.035f)
				{
					Color = BoundaryColor;
				}
			}
			// Quadrillage : une ligne par unité d'eau ou de lumière
			const float GU = U * 10.f, GV = V * 10.f;
			if (FMath::Abs(GU - FMath::RoundToFloat(GU)) < 0.025f || FMath::Abs(GV - FMath::RoundToFloat(GV)) < 0.025f)
			{
				Color = LerpColor(Color, GridColor, 0.6f);
			}
			Pixels[PixelIndex(U, V)] = Color;
		}
	}
	Background = Pixels;

	// 2) Points : les expériences, entourées de blanc si le modèle se trompe
	MistakeCount = 0;
	for (int32 s = 0; s < Data.Num(); ++s)
	{
		const float* Raw = Data.GetRawInput(s);
		const float U = (Raw[0] - Data.RawMin) / FMath::Max(KINDA_SMALL_NUMBER, Data.RawMax - Data.RawMin);
		const float V = (Raw[1] - Data.RawMin) / FMath::Max(KINDA_SMALL_NUMBER, Data.RawMax - Data.RawMin);
		const bool bAlive = Data.GetTarget(s)[0] >= 0.5f;

		bool bMistake = false;
		if (bShowPrediction)
		{
			const float* Input = Data.GetInput(s);
			bMistake = (Trainer->Predict(Input[0], Input[1]) >= 0.5f) != bAlive;
			MistakeCount += bMistake ? 1 : 0;
		}
		const float Radius = TextureSize / 64.f;
		PlotDisc(U, V, Radius + (bMistake ? 2.f : 1.f), bMistake ? MistakeRing : PointOutline);
		PlotDisc(U, V, Radius, bAlive ? AlivePoint : DeadPoint);
	}

	UploadTexture();

	if (Status)
	{
		FString Text;
		switch (State)
		{
		case ENFTrainingState::Idle:
			Text = TEXT("Lancez l'entraînement pour voir\nce que le modèle a compris.");
			break;
		case ENFTrainingState::Training:
			Text = FString::Printf(TEXT("Le modèle apprend...\nErreurs : %d plantes sur %d"), MistakeCount, Data.Num());
			break;
		case ENFTrainingState::Succeeded:
			Text = FString::Printf(TEXT("Réussi !\nSeulement %d erreurs sur %d plantes."), MistakeCount, Data.Num());
			break;
		case ENFTrainingState::Failed:
			Text = FString::Printf(TEXT("Raté : %d erreurs sur %d plantes.\nChangez le modèle et réessayez !"), MistakeCount, Data.Num());
			break;
		}
		Status->SetText(FText::FromString(Text));
	}
}

void ANFLabNotebook::UploadTexture()
{
	if (!ChartTexture || Pixels.Num() != TextureSize * TextureSize)
	{
		return;
	}

	// Copie possédée par le rendu, libérée une fois la mise à jour faite
	const int32 Bytes = Pixels.Num() * sizeof(FColor);
	uint8* Data = new uint8[Bytes];
	FMemory::Memcpy(Data, Pixels.GetData(), Bytes);
	FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, TextureSize, TextureSize);
	ChartTexture->UpdateTextureRegions(0, 1, Region, TextureSize * sizeof(FColor), sizeof(FColor), Data,
		[](uint8* SrcData, const FUpdateTextureRegion2D* Regions)
		{
			delete[] SrcData;
			delete Regions;
		});
}
