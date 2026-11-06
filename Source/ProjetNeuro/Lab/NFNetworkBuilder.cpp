// Neuron Forge - ENSIM

#include "Lab/NFNetworkBuilder.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Player/NFPlayerController.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "NeuronForge"

namespace
{
	/** Nombre maximal de colonnes affichées : entrée + 3 couches cachées + sortie. */
	constexpr int32 MaxColumns = 5;

	/** Données par instance lues par le material : R, G, B, intensité. */
	constexpr int32 NumCustomData = 4;

	/** Dimensions de la table de l'holotable, en cm. */
	constexpr float TableDepth = 120.f;
	constexpr float TableHeight = 80.f;

	FString FormatLearningRate(float Value)
	{
		return Value < 0.1f ? FString::Printf(TEXT("%.2f"), Value) : FString::Printf(TEXT("%.1f"), Value);
	}
}

ANFNetworkBuilder::ANFNetworkBuilder()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	TableMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TableMesh"));
	TableMesh->SetupAttachment(SceneRoot);
	// Table de 120 x 320 cm, 80 cm de haut (le cube de base fait 100 cm, pivot au centre)
	TableMesh->SetRelativeLocation(FVector(0.f, 0.f, TableHeight * 0.5f));
	TableMesh->SetRelativeScale3D(FVector(TableDepth, 320.f, TableHeight) / 100.f);

	NeuronInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("NeuronInstances"));
	NeuronInstances->SetupAttachment(SceneRoot);
	NeuronInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NeuronInstances->SetCastShadow(false);
	NeuronInstances->SetNumCustomDataFloats(NumCustomData);

	ConnectionInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ConnectionInstances"));
	ConnectionInstances->SetupAttachment(SceneRoot);
	ConnectionInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ConnectionInstances->SetCastShadow(false);
	ConnectionInstances->SetNumCustomDataFloats(NumCustomData);

	PulseInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PulseInstances"));
	PulseInstances->SetupAttachment(SceneRoot);
	PulseInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PulseInstances->SetCastShadow(false);
	PulseInstances->SetNumCustomDataFloats(NumCustomData);

	Trainer = CreateDefaultSubobject<UNFTrainerComponent>(TEXT("Trainer"));

	for (int32 c = 0; c < MaxColumns; ++c)
	{
		UTextRenderComponent* Label = CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("ColumnLabel%d"), c));
		Label->SetupAttachment(SceneRoot);
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetVerticalAlignment(EVRTA_TextTop);
		Label->SetWorldSize(6.f);
		Label->SetTextRenderColor(FColor(200, 230, 255));
		ColumnLabels.Add(Label);
	}

	// Exemple courant affiché sur la face avant de la table
	SampleLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SampleLabel"));
	SampleLabel->SetupAttachment(SceneRoot);
	SampleLabel->SetRelativeLocation(FVector(TableDepth * 0.5f + 1.f, 0.f, TableHeight * 0.6f));
	SampleLabel->SetHorizontalAlignment(EHTA_Center);
	SampleLabel->SetVerticalAlignment(EVRTA_TextCenter);
	SampleLabel->SetWorldSize(7.f);
	SampleLabel->SetTextRenderColor(FColor(255, 240, 200));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	TableMesh->SetStaticMesh(CubeMesh.Object);
	NeuronInstances->SetStaticMesh(SphereMesh.Object);
	PulseInstances->SetStaticMesh(SphereMesh.Object);
	ConnectionInstances->SetStaticMesh(CylinderMesh.Object);

	LearningRateSteps = { 0.01f, 0.05f, 0.1f, 0.3f, 0.5f, 1.f, 2.f };
}

void ANFNetworkBuilder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyConfigToTrainer();
}

void ANFNetworkBuilder::BeginPlay()
{
	Super::BeginPlay();

	Trainer->OnStateChanged.AddUniqueDynamic(this, &ANFNetworkBuilder::HandleTrainerStateChanged);
	ApplyConfigToTrainer();
}

void ANFNetworkBuilder::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// L'holotable fait défiler les exemples du dataset pour montrer la réponse du réseau
	SampleTimer += DeltaSeconds;
	bool bNeedsUpdate = false;
	if (SampleTimer >= SampleDisplayDuration)
	{
		SampleTimer = 0.f;
		const int32 NumSamples = Trainer->GetDataset().Num();
		DisplayedSample = NumSamples > 0 ? (DisplayedSample + 1) % NumSamples : 0;
		bNeedsUpdate = true;
	}

	// Pendant l'entraînement, les poids changent en continu
	VisualTimer += DeltaSeconds;
	if (Trainer->IsTraining() && VisualTimer >= 1.f / VisualUpdateRate)
	{
		bNeedsUpdate = true;
	}

	if (bNeedsUpdate)
	{
		VisualTimer = 0.f;
		UpdateVisualization();
	}

	UpdatePulses(DeltaSeconds);
}

void ANFNetworkBuilder::UpdatePulses(float DeltaSeconds)
{
	if (!PulseInstances || PulseStarts.Num() != PulseInstances->GetInstanceCount())
	{
		return;
	}

	// Les impulsions accélèrent pendant l'entraînement
	const float TargetSpeed = Trainer && Trainer->IsTraining() ? PulseSpeedTraining : PulseSpeedIdle;
	PulseSpeed = FMath::FInterpTo(PulseSpeed, TargetSpeed, DeltaSeconds, 3.f);
	PulsePhase = FMath::Fmod(PulsePhase + DeltaSeconds * PulseSpeed, 1.f);

	const FVector Scale(PulseDiameter / 100.f);
	for (int32 i = 0; i < PulseStarts.Num(); ++i)
	{
		const float Alpha = FMath::Frac(PulsePhase + PulseOffsets[i]);
		const FVector Location = FMath::Lerp(PulseStarts[i], PulseEnds[i], Alpha);
		// Grossit au milieu du trajet, disparaît aux extrémités
		const float Size = FMath::Sin(Alpha * PI);
		PulseInstances->UpdateInstanceTransform(i, FTransform(FRotator::ZeroRotator, Location, Scale * Size), false, false, true);
	}
	PulseInstances->MarkRenderStateDirty();
}

// ---------------------------------------------------------------------------
// Configuration

bool ANFNetworkBuilder::CanEdit() const
{
	return Trainer && !Trainer->IsTraining();
}

bool ANFNetworkBuilder::CanApplyAction(ENFConfigAction Action, int32 LayerIndex) const
{
	if (!CanEdit())
	{
		return false;
	}

	const bool bLayerExists = HiddenLayers.IsValidIndex(LayerIndex);
	switch (Action)
	{
	case ENFConfigAction::AddLayer:
		return HiddenLayers.Num() < MaxHiddenLayers;
	case ENFConfigAction::RemoveLayer:
		return HiddenLayers.Num() > 0;
	case ENFConfigAction::AddNeuron:
		return bLayerExists && HiddenLayers[LayerIndex].NumNeurons < MaxNeuronsPerLayer;
	case ENFConfigAction::RemoveNeuron:
		return bLayerExists && HiddenLayers[LayerIndex].NumNeurons > MinNeuronsPerLayer;
	case ENFConfigAction::NextActivation:
		return bLayerExists;
	case ENFConfigAction::IncreaseLearningRate:
		return LearningRateIndex < LearningRateSteps.Num() - 1;
	case ENFConfigAction::DecreaseLearningRate:
		return LearningRateIndex > 0;
	default:
		return false;
	}
}

bool ANFNetworkBuilder::ApplyAction(ENFConfigAction Action, int32 LayerIndex)
{
	if (!CanApplyAction(Action, LayerIndex))
	{
		return false;
	}

	switch (Action)
	{
	case ENFConfigAction::AddLayer:
		HiddenLayers.Add(NewLayerTemplate);
		break;
	case ENFConfigAction::RemoveLayer:
		HiddenLayers.Pop();
		break;
	case ENFConfigAction::AddNeuron:
		++HiddenLayers[LayerIndex].NumNeurons;
		break;
	case ENFConfigAction::RemoveNeuron:
		--HiddenLayers[LayerIndex].NumNeurons;
		break;
	case ENFConfigAction::NextActivation:
		HiddenLayers[LayerIndex].Activation = NFActivation::Next(HiddenLayers[LayerIndex].Activation);
		break;
	case ENFConfigAction::IncreaseLearningRate:
		++LearningRateIndex;
		break;
	case ENFConfigAction::DecreaseLearningRate:
		--LearningRateIndex;
		break;
	}

	ApplyConfigToTrainer();
	return true;
}

FText ANFNetworkBuilder::DescribeAction(ENFConfigAction Action, int32 LayerIndex) const
{
	const bool bLayerExists = HiddenLayers.IsValidIndex(LayerIndex);
	const FText LayerName = FText::Format(LOCTEXT("LayerName", "Couche {0}"), FText::AsNumber(LayerIndex + 1));

	switch (Action)
	{
	case ENFConfigAction::AddLayer:
		return HiddenLayers.Num() < MaxHiddenLayers
			? FText::Format(LOCTEXT("AddLayer", "Ajouter une couche cachée ({0} neurones, {1})"),
				FText::AsNumber(NewLayerTemplate.NumNeurons), FText::FromString(NFActivation::ToString(NewLayerTemplate.Activation)))
			: FText::Format(LOCTEXT("AddLayerMax", "Ajouter une couche (maximum {0} atteint)"), FText::AsNumber(MaxHiddenLayers));
	case ENFConfigAction::RemoveLayer:
		return HiddenLayers.Num() > 0
			? FText::Format(LOCTEXT("RemoveLayer", "Retirer la couche {0}"), FText::AsNumber(HiddenLayers.Num()))
			: LOCTEXT("RemoveLayerNone", "Retirer une couche (aucune couche cachée)");
	case ENFConfigAction::AddNeuron:
		return bLayerExists
			? FText::Format(LOCTEXT("AddNeuron", "{0} : ajouter un neurone ({1} → {2})"), LayerName,
				FText::AsNumber(HiddenLayers[LayerIndex].NumNeurons), FText::AsNumber(HiddenLayers[LayerIndex].NumNeurons + 1))
			: FText::Format(LOCTEXT("NoLayer", "{0} : couche absente"), LayerName);
	case ENFConfigAction::RemoveNeuron:
		return bLayerExists
			? FText::Format(LOCTEXT("RemoveNeuron", "{0} : retirer un neurone ({1} → {2})"), LayerName,
				FText::AsNumber(HiddenLayers[LayerIndex].NumNeurons), FText::AsNumber(HiddenLayers[LayerIndex].NumNeurons - 1))
			: FText::Format(LOCTEXT("NoLayer", "{0} : couche absente"), LayerName);
	case ENFConfigAction::NextActivation:
		return bLayerExists
			? FText::Format(LOCTEXT("NextActivation", "{0} : activation {1} → {2}"), LayerName,
				FText::FromString(NFActivation::ToString(HiddenLayers[LayerIndex].Activation)),
				FText::FromString(NFActivation::ToString(NFActivation::Next(HiddenLayers[LayerIndex].Activation))))
			: FText::Format(LOCTEXT("NoLayer", "{0} : couche absente"), LayerName);
	case ENFConfigAction::IncreaseLearningRate:
	case ENFConfigAction::DecreaseLearningRate:
	{
		const int32 Delta = (Action == ENFConfigAction::IncreaseLearningRate) ? 1 : -1;
		const int32 NewIndex = FMath::Clamp(LearningRateIndex + Delta, 0, LearningRateSteps.Num() - 1);
		return FText::Format(LOCTEXT("LearningRate", "Taux d'apprentissage : {0} → {1}"),
			FText::FromString(FormatLearningRate(GetLearningRate())),
			FText::FromString(FormatLearningRate(LearningRateSteps.IsValidIndex(NewIndex) ? LearningRateSteps[NewIndex] : 0.f)));
	}
	default:
		return FText::GetEmpty();
	}
}

FString ANFNetworkBuilder::GetActionLabel(ENFConfigAction Action, int32 LayerIndex) const
{
	switch (Action)
	{
	case ENFConfigAction::AddLayer:				return TEXT("+ COUCHE");
	case ENFConfigAction::RemoveLayer:			return TEXT("- COUCHE");
	case ENFConfigAction::AddNeuron:			return TEXT("+");
	case ENFConfigAction::RemoveNeuron:			return TEXT("-");
	case ENFConfigAction::NextActivation:
		return HiddenLayers.IsValidIndex(LayerIndex) ? NFActivation::ToString(HiddenLayers[LayerIndex].Activation) : TEXT("f(x)");
	case ENFConfigAction::IncreaseLearningRate:	return TEXT("LR +");
	case ENFConfigAction::DecreaseLearningRate:	return TEXT("LR -");
	default:									return FString();
	}
}

void ANFNetworkBuilder::SetConfiguration(const TArray<FNFLayerConfig>& InHiddenLayers, int32 InLearningRateIndex)
{
	HiddenLayers = InHiddenLayers;
	LearningRateIndex = InLearningRateIndex;
	ApplyConfigToTrainer();
}

float ANFNetworkBuilder::GetLearningRate() const
{
	return LearningRateSteps.IsValidIndex(LearningRateIndex) ? LearningRateSteps[LearningRateIndex] : 0.5f;
}

FString ANFNetworkBuilder::DescribeNetwork() const
{
	return Trainer ? Trainer->GetNetwork().Describe() : FString();
}

FString ANFNetworkBuilder::GetHintText() const
{
	const ENFTrainingState State = GetTrainingState();
	if (State == ENFTrainingState::Succeeded)
	{
		return TEXT("Bravo ! Le modèle sépare correctement les 4 cas de XOR.");
	}
	if (State == ENFTrainingState::Training)
	{
		return TEXT("Observez la perte diminuer et les poids évoluer...");
	}

	const bool bFailed = (State == ENFTrainingState::Failed);
	if (HiddenLayers.Num() == 0)
	{
		return bFailed
			? TEXT("Sans couche cachée, le modèle trace une seule droite :\nXOR n'est pas linéairement séparable. Insérez un module de couche dans la Forge.")
			: TEXT("Assemblez le réseau sur la Forge, puis marchez sur le bouton au sol.");
	}

	const bool bAllLinear = HiddenLayers.ContainsByPredicate(
		[](const FNFLayerConfig& Layer) { return Layer.Activation != ENFActivation::Linear; }) == false;
	if (bAllLinear)
	{
		return bFailed
			? TEXT("Des couches Linear empilées restent linéaires.\nPlantez un cristal ReLU, Tanh ou Sigmoid sur un module.")
			: TEXT("Astuce : sans cristal, un module est Linear et n'ajoute aucune non-linéarité.");
	}

	if (bFailed)
	{
		if (GetLearningRate() <= 0.05f)
		{
			return TEXT("Le taux d'apprentissage est très faible :\nle modèle apprend trop lentement. Tirez le levier.");
		}
		if (GetLearningRate() >= 2.f)
		{
			return TEXT("Un taux d'apprentissage trop fort rend l'entraînement instable.\nEssayez une valeur plus faible.");
		}
		const bool bSmallLayer = HiddenLayers.ContainsByPredicate(
			[](const FNFLayerConfig& Layer) { return Layer.NumNeurons <= 2; });
		if (bSmallLayer)
		{
			return TEXT("Une couche de 1 ou 2 neurones manque de capacité.\nAjoutez des orbes-neurones autour du module.");
		}
		return TEXT("Presque ! Essayez une autre activation,\nplus de neurones ou un autre taux d'apprentissage.");
	}
	return TEXT("Marchez sur le bouton au sol pour lancer l'entraînement.");
}

// ---------------------------------------------------------------------------
// Entraînement

bool ANFNetworkBuilder::CanStartTraining() const
{
	return Trainer && !Trainer->IsTraining();
}

bool ANFNetworkBuilder::StartTraining()
{
	if (!CanStartTraining() || !Trainer->StartTraining())
	{
		return false;
	}
	++TrainingAttempts;

	if (TrainingStartSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, TrainingStartSound, GetActorLocation());
	}
	ShowPlayerMessage(TEXT("Entraînement lancé !"), FLinearColor(0.3f, 0.8f, 1.f));
	return true;
}

ENFTrainingState ANFNetworkBuilder::GetTrainingState() const
{
	return Trainer ? Trainer->GetState() : ENFTrainingState::Idle;
}

void ANFNetworkBuilder::HandleTrainerStateChanged(ENFTrainingState NewState)
{
	if (NewState == ENFTrainingState::Succeeded)
	{
		bHasSucceeded = true;
		SuccessArchitecture = DescribeNetwork();
		SuccessLearningRate = GetLearningRate();
		SuccessProgress = Trainer->GetProgress();
		ShowPlayerMessage(FString::Printf(TEXT("Modèle validé en %d epochs !"), Trainer->GetProgress().Epoch),
			FLinearColor(0.2f, 1.f, 0.4f));
	}
	else if (NewState == ENFTrainingState::Failed)
	{
		ShowPlayerMessage(TEXT("Échec : précision cible non atteinte."), FLinearColor(1.f, 0.35f, 0.2f));
		if (TrainingFailedSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, TrainingFailedSound, GetActorLocation());
		}
	}

	UpdateVisualization();
	OnNetworkChanged.Broadcast();
}

void ANFNetworkBuilder::ShowPlayerMessage(const FString& Text, const FLinearColor& Color) const
{
	if (ANFPlayerController* PC = Cast<ANFPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->ShowMessage(Text, Color, 4.f);
	}
}

void ANFNetworkBuilder::ApplyConfigToTrainer()
{
	if (!Trainer)
	{
		return;
	}

	// La configuration de l'éditeur peut sortir des bornes : on la normalise
	if (HiddenLayers.Num() > MaxHiddenLayers)
	{
		HiddenLayers.SetNum(MaxHiddenLayers);
	}
	for (FNFLayerConfig& Layer : HiddenLayers)
	{
		Layer.NumNeurons = FMath::Clamp(Layer.NumNeurons, MinNeuronsPerLayer, MaxNeuronsPerLayer);
	}
	LearningRateIndex = FMath::Clamp(LearningRateIndex, 0, FMath::Max(0, LearningRateSteps.Num() - 1));

	Trainer->Configure(HiddenLayers, GetLearningRate());
	DisplayedSample = 0;
	SampleTimer = 0.f;

	RebuildVisualization();
	OnNetworkChanged.Broadcast();
}

// ---------------------------------------------------------------------------
// Visualisation

void ANFNetworkBuilder::GetColumnSizes(TArray<int32>& OutSizes) const
{
	OutSizes.Reset();
	const FNFNeuralNetwork& Network = Trainer->GetNetwork();
	OutSizes.Add(Network.GetNumInputs());
	for (const FNFDenseLayer& Layer : Network.GetLayers())
	{
		OutSizes.Add(Layer.NumOutputs);
	}
}

FVector ANFNetworkBuilder::GetNeuronLocation(int32 Column, int32 Index, int32 NumColumns, int32 ColumnSize) const
{
	// Entrée côté +Y (à gauche vu depuis +X), sortie côté -Y
	const float Y = (NumColumns - 1) * 0.5f * LayerSpacing - Column * LayerSpacing;
	const float Z = ((ColumnSize - 1) * 0.5f - Index) * NeuronSpacing;
	return VisualOrigin + FVector(0.f, Y, Z);
}

void ANFNetworkBuilder::RebuildVisualization()
{
	if (!Trainer || !NeuronInstances || !ConnectionInstances)
	{
		return;
	}

	NeuronInstances->ClearInstances();
	ConnectionInstances->ClearInstances();

	TArray<int32> Sizes;
	GetColumnSizes(Sizes);
	const int32 NumColumns = Sizes.Num();

	const float NeuronScale = NeuronDiameter / 100.f;	// la sphère de base fait 100 cm
	for (int32 c = 0; c < NumColumns; ++c)
	{
		for (int32 n = 0; n < Sizes[c]; ++n)
		{
			NeuronInstances->AddInstance(FTransform(FRotator::ZeroRotator, GetNeuronLocation(c, n, NumColumns, Sizes[c]), FVector(NeuronScale)));
		}
	}

	// Un cylindre par poids, orienté de la colonne c vers la colonne c + 1, et une impulsion qui le parcourt
	if (PulseInstances)
	{
		PulseInstances->ClearInstances();
	}
	PulseStarts.Reset();
	PulseEnds.Reset();
	PulseOffsets.Reset();
	FRandomStream PulseRng(1234);
	for (int32 c = 0; c + 1 < NumColumns; ++c)
	{
		for (int32 o = 0; o < Sizes[c + 1]; ++o)
		{
			for (int32 i = 0; i < Sizes[c]; ++i)
			{
				ConnectionInstances->AddInstance(FTransform::Identity);
				PulseStarts.Add(GetNeuronLocation(c, i, NumColumns, Sizes[c]));
				PulseEnds.Add(GetNeuronLocation(c + 1, o, NumColumns, Sizes[c + 1]));
				PulseOffsets.Add(PulseRng.FRand());
				if (PulseInstances)
				{
					PulseInstances->AddInstance(FTransform::Identity);
				}
			}
		}
	}

	UpdateVisualization();
}

void ANFNetworkBuilder::UpdateVisualization()
{
	if (!Trainer || !NeuronInstances || !ConnectionInstances)
	{
		return;
	}

	FNFNeuralNetwork& Network = Trainer->GetMutableNetwork();
	const FNFDataset& Dataset = Trainer->GetDataset();
	if (!Network.IsValid() || Dataset.Num() == 0)
	{
		return;
	}

	// Propage l'exemple affiché pour obtenir les activations de chaque neurone
	DisplayedSample = FMath::Clamp(DisplayedSample, 0, Dataset.Num() - 1);
	Network.Forward(Dataset.GetInput(DisplayedSample));

	TArray<int32> Sizes;
	GetColumnSizes(Sizes);
	const int32 NumColumns = Sizes.Num();
	const TArray<FNFDenseLayer>& NetLayers = Network.GetLayers();

	// --- Neurones ---
	int32 Instance = 0;
	for (int32 c = 0; c < NumColumns; ++c)
	{
		for (int32 n = 0; n < Sizes[c]; ++n, ++Instance)
		{
			FLinearColor Color;
			float Value;
			if (c == 0)
			{
				Color = InputNeuronColor;
				Value = Network.GetLastInput()[n];
			}
			else
			{
				const FNFDenseLayer& Layer = NetLayers[c - 1];
				Color = NFActivation::GetColor(Layer.Activation);
				Value = Layer.A[n];
			}
			const float Intensity = 0.3f + 4.f * FMath::Clamp(FMath::Abs(Value), 0.f, 1.f);
			const float Data[NumCustomData] = { Color.R, Color.G, Color.B, Intensity };
			NeuronInstances->SetCustomData(Instance, MakeArrayView(Data, NumCustomData), false);
		}
	}
	NeuronInstances->MarkRenderStateDirty();

	// --- Poids ---
	Instance = 0;
	for (int32 c = 0; c + 1 < NumColumns; ++c)
	{
		const FNFDenseLayer& Layer = NetLayers[c];
		for (int32 o = 0; o < Sizes[c + 1]; ++o)
		{
			const FVector End = GetNeuronLocation(c + 1, o, NumColumns, Sizes[c + 1]);
			for (int32 i = 0; i < Sizes[c]; ++i, ++Instance)
			{
				const FVector Start = GetNeuronLocation(c, i, NumColumns, Sizes[c]);
				const FVector Delta = End - Start;
				const float Weight = Layer.GetWeight(o, i);
				const float Magnitude = FMath::Clamp(FMath::Abs(Weight) / 3.f, 0.f, 1.f);

				// Le cylindre de base fait 100 cm de haut et 100 cm de diamètre, centré, axe Z
				const float Thickness = FMath::Lerp(MinConnectionThickness, MaxConnectionThickness, Magnitude) / 100.f;
				const FTransform Transform(FRotationMatrix::MakeFromZ(Delta.GetSafeNormal()).Rotator(),
					Start + Delta * 0.5f, FVector(Thickness, Thickness, Delta.Size() / 100.f));
				ConnectionInstances->UpdateInstanceTransform(Instance, Transform, false, false, true);

				const FLinearColor Color = Weight >= 0.f ? PositiveWeightColor : NegativeWeightColor;
				const float Data[NumCustomData] = { Color.R, Color.G, Color.B, 0.3f + 2.f * Magnitude };
				ConnectionInstances->SetCustomData(Instance, MakeArrayView(Data, NumCustomData), false);

				// L'impulsion est d'autant plus vive que le signal transmis (activation x poids) est fort
				if (PulseInstances && Instance < PulseInstances->GetInstanceCount())
				{
					const float Source = (c == 0) ? Network.GetLastInput()[i] : NetLayers[c - 1].A[i];
					const float Signal = FMath::Clamp(FMath::Abs(Source * Weight), 0.f, 1.5f);
					const float PulseData[NumCustomData] = { Color.R, Color.G, Color.B, 0.6f + 5.f * Signal };
					PulseInstances->SetCustomData(Instance, MakeArrayView(PulseData, NumCustomData), false);
				}
			}
		}
	}
	ConnectionInstances->MarkRenderStateDirty();
	if (PulseInstances)
	{
		PulseInstances->MarkRenderStateDirty();
	}

	UpdateLabels();
}

void ANFNetworkBuilder::UpdateLabels()
{
	const FNFNeuralNetwork& Network = Trainer->GetNetwork();
	const TArray<FNFDenseLayer>& NetLayers = Network.GetLayers();

	TArray<int32> Sizes;
	GetColumnSizes(Sizes);
	const int32 NumColumns = Sizes.Num();
	const float BottomZ = VisualOrigin.Z - (MaxNeuronsPerLayer * 0.5f) * NeuronSpacing - 4.f;

	for (int32 c = 0; c < ColumnLabels.Num(); ++c)
	{
		UTextRenderComponent* Label = ColumnLabels[c];
		if (!Label)
		{
			continue;
		}

		const bool bVisible = c < NumColumns;
		Label->SetVisibility(bVisible);
		if (!bVisible)
		{
			continue;
		}

		FString Text;
		if (c == 0)
		{
			Text = FString::Printf(TEXT("Entrée\n%d"), Sizes[0]);
		}
		else if (c == NumColumns - 1)
		{
			Text = FString::Printf(TEXT("Sortie\n%s"), *NFActivation::ToString(NetLayers[c - 1].Activation));
		}
		else
		{
			Text = FString::Printf(TEXT("Couche %d\n%d %s"), c, Sizes[c], *NFActivation::ToString(NetLayers[c - 1].Activation));
		}
		Label->SetText(FText::FromString(Text));

		const float Y = (NumColumns - 1) * 0.5f * LayerSpacing - c * LayerSpacing;
		Label->SetRelativeLocation(FVector(VisualOrigin.X + 8.f, Y, BottomZ));
	}

	if (SampleLabel)
	{
		const FNFDataset& Dataset = Trainer->GetDataset();
		const float* Input = Dataset.GetInput(DisplayedSample);
		const float Target = Dataset.GetTarget(DisplayedSample)[0];
		const float Output = NetLayers.Num() > 0 ? NetLayers.Last().A[0] : 0.f;
		SampleLabel->SetText(FText::FromString(FString::Printf(TEXT("Exemple : %.0f XOR %.0f = %.0f    sortie du réseau : %.2f"),
			Input[0], Input[1], Target, Output)));
	}
}

#undef LOCTEXT_NAMESPACE
