// Neuron Forge - ENSIM

#include "Lab/NFStatusDisplay.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Lab/NFNetworkBuilder.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Écran de 300 x 170 cm, texte sur la face avant (+X)
	constexpr float ScreenWidth = 300.f;
	constexpr float ScreenHeight = 170.f;

	UTextRenderComponent* MakeText(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, float Z, float Size)
	{
		UTextRenderComponent* Text = Owner->CreateDefaultSubobject<UTextRenderComponent>(Name);
		Text->SetupAttachment(Parent);
		Text->SetRelativeLocation(FVector(3.f, ScreenWidth * 0.5f - 12.f, Z));
		Text->SetHorizontalAlignment(EHTA_Left);
		Text->SetVerticalAlignment(EVRTA_TextTop);
		Text->SetWorldSize(Size);
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Text;
	}
}

ANFStatusDisplay::ANFStatusDisplay()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	ScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenMesh"));
	ScreenMesh->SetupAttachment(SceneRoot);
	ScreenMesh->SetStaticMesh(CubeMesh.Object);
	ScreenMesh->SetRelativeLocation(FVector(-2.f, 0.f, 0.f));
	ScreenMesh->SetRelativeScale3D(FVector(0.04f, ScreenWidth / 100.f, ScreenHeight / 100.f));

	const float Top = ScreenHeight * 0.5f - 8.f;
	TitleText = MakeText(this, SceneRoot, TEXT("TitleText"), Top, 14.f);
	BodyText = MakeText(this, SceneRoot, TEXT("BodyText"), Top - 24.f, 8.5f);
	HintText = MakeText(this, SceneRoot, TEXT("HintText"), -ScreenHeight * 0.5f + 26.f, 7.f);
	HintText->SetTextRenderColor(FColor(230, 230, 230));
}

void ANFStatusDisplay::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Refresh();
}

void ANFStatusDisplay::BeginPlay()
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
		Builder->OnNetworkChanged.AddUniqueDynamic(this, &ANFStatusDisplay::HandleNetworkChanged);
		if (UNFTrainerComponent* Trainer = Builder->GetTrainer())
		{
			Trainer->OnProgress.AddUniqueDynamic(this, &ANFStatusDisplay::HandleProgress);
		}
	}
	Refresh();
}

void ANFStatusDisplay::HandleNetworkChanged()
{
	Refresh();
}

void ANFStatusDisplay::HandleProgress(const FNFTrainingProgress& Progress)
{
	Refresh();
}

void ANFStatusDisplay::Refresh()
{
	if (!TitleText || !BodyText || !HintText)
	{
		return;
	}

	if (!Builder || !Builder->GetTrainer())
	{
		TitleText->SetText(FText::FromString(MissionTitle));
		TitleText->SetTextRenderColor(IdleColor);
		BodyText->SetText(FText::FromString(TEXT("Aucune holotable associée.")));
		HintText->SetText(FText::GetEmpty());
		return;
	}

	FString Title;
	FColor Color;
	switch (Builder->GetTrainingState())
	{
	case ENFTrainingState::Training:
		Title = TEXT("ENTRAÎNEMENT EN COURS...");
		Color = TrainingColor;
		break;
	case ENFTrainingState::Succeeded:
		Title = TEXT("MODÈLE VALIDÉ - ACCÈS AUTORISÉ");
		Color = SuccessColor;
		break;
	case ENFTrainingState::Failed:
		Title = TEXT("ÉCHEC - PRÉCISION INSUFFISANTE");
		Color = FailureColor;
		break;
	case ENFTrainingState::Idle:
	default:
		Title = MissionTitle;
		Color = IdleColor;
		break;
	}

	TitleText->SetText(FText::FromString(Title));
	TitleText->SetTextRenderColor(Color);
	BodyText->SetText(FText::FromString(BuildBodyText()));
	BodyText->SetTextRenderColor(FColor(225, 240, 255));
	HintText->SetText(FText::FromString(Builder->GetHintText()));
}

FString ANFStatusDisplay::BuildBodyText() const
{
	UNFTrainerComponent* Trainer = Builder->GetTrainer();
	const FNFTrainingProgress& Progress = Trainer->GetProgress();
	const bool bTrained = Builder->GetTrainingState() != ENFTrainingState::Idle;

	FString Text;
	Text += FString::Printf(TEXT("Architecture : %s\n"), *Builder->DescribeNetwork());
	Text += FString::Printf(TEXT("Taux d'apprentissage : %.2f\n"), Builder->GetLearningRate());

	if (bTrained && Builder->IsGreenhouse())
	{
		// Bilan simple sur toutes les expériences de la serre
		TArray<float> Predictions;
		Trainer->GetPredictions(Predictions);
		const FNFDataset& Dataset = Trainer->GetDataset();
		int32 Alive = 0, AliveFound = 0, Dead = 0, DeadFound = 0;
		for (int32 s = 0; s < Predictions.Num(); ++s)
		{
			const bool bAlive = Dataset.GetTarget(s)[0] >= 0.5f;
			const bool bPredictedAlive = Predictions[s] >= 0.5f;
			Alive += bAlive ? 1 : 0;
			Dead += bAlive ? 0 : 1;
			AliveFound += (bAlive && bPredictedAlive) ? 1 : 0;
			DeadFound += (!bAlive && !bPredictedAlive) ? 1 : 0;
		}
		Text += FString::Printf(TEXT("Tour d'entraînement : %d / %d\n"), Progress.Epoch, Trainer->MaxEpochs);
		Text += FString::Printf(TEXT("Bonnes prédictions : %d / %d  (%.0f %%, objectif %.0f %%)\n"),
			AliveFound + DeadFound, Predictions.Num(), Progress.Accuracy * 100.f, Trainer->TargetAccuracy * 100.f);
		Text += FString::Printf(TEXT("   Plantes vivantes reconnues : %d / %d\n"), AliveFound, Alive);
		Text += FString::Printf(TEXT("   Plantes mortes reconnues : %d / %d\n"), DeadFound, Dead);
		Text += FString::Printf(TEXT("Erreur moyenne (perte) : %.3f\n"), Progress.Loss);
	}
	else if (bTrained)
	{
		Text += FString::Printf(TEXT("Epoch : %d / %d\n"), Progress.Epoch, Trainer->MaxEpochs);
		Text += FString::Printf(TEXT("Perte : %.4f  (objectif < %.2f)\n"), Progress.Loss, Trainer->TargetLoss);
		Text += FString::Printf(TEXT("Précision : %.0f %%  (objectif %.0f %%)\n"), Progress.Accuracy * 100.f, Trainer->TargetAccuracy * 100.f);

		// Prédictions sur chaque exemple du dataset
		TArray<float> Predictions;
		Trainer->GetPredictions(Predictions);
		const FNFDataset& Dataset = Trainer->GetDataset();
		for (int32 s = 0; s < Predictions.Num(); ++s)
		{
			const float* Input = Dataset.GetInput(s);
			const float Target = Dataset.GetTarget(s)[0];
			const bool bCorrect = (Predictions[s] >= 0.5f) == (Target >= 0.5f);
			Text += FString::Printf(TEXT("   %.0f XOR %.0f = %.0f   ->   %.2f   %s\n"),
				Input[0], Input[1], Target, Predictions[s], bCorrect ? TEXT("OK") : TEXT("FAUX"));
		}
	}
	else
	{
		if (Builder->IsGreenhouse())
		{
			Text += FString::Printf(TEXT("Expériences de la serre : %d plantes\n"), Trainer->GetDataset().Num());
		}
		Text += FString::Printf(TEXT("Paramètres entraînables : %d\n"), Trainer->GetNetwork().GetNumParameters());
		Text += Builder->IsGreenhouse()
			? FString::Printf(TEXT("Objectif : %.0f %% de bonnes prédictions\n"), Trainer->TargetAccuracy * 100.f)
			: FString::Printf(TEXT("Objectif : précision %.0f %% et perte < %.2f\n"), Trainer->TargetAccuracy * 100.f, Trainer->TargetLoss);
		Text += TEXT("\nAssemblez le réseau sur la Forge,\npuis marchez sur le bouton au sol.");
	}
	return Text;
}
