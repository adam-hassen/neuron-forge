// Neuron Forge - ENSIM

#include "Training/NFTrainerComponent.h"
#include "ProjetNeuro.h"

UNFTrainerComponent::UNFTrainerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	Dataset = FNFDataset::MakeXOR();
	RebuildNetwork();
}

void UNFTrainerComponent::RebuildNetwork()
{
	Network.Build(Dataset.NumInputs, HiddenConfig, Dataset.NumOutputs, ENFActivation::Sigmoid, Seed);
	Progress = FNFTrainingProgress();
	LossHistory.Reset();
	AccuracyHistory.Reset();
	ValidationHistory.Reset();
	EpochAccumulator = 0.f;
}

void UNFTrainerComponent::ApplyRate()
{
	// Rythme en exemples : même durée de jeu quelle que soit la taille du jeu d'entraînement
	if (SamplesPerSecond > 0.f && Dataset.Num() > 0)
	{
		EpochsPerSecond = SamplesPerSecond / Dataset.Num();
	}
	if (MaxTrainingSamples > 0 && Dataset.Num() > 0)
	{
		MaxEpochs = FMath::Max(1, FMath::CeilToInt32(static_cast<float>(MaxTrainingSamples) / Dataset.Num()));
	}
}

void UNFTrainerComponent::Configure(const TArray<FNFLayerConfig>& HiddenLayers, float InLearningRate)
{
	HiddenConfig = HiddenLayers;
	LearningRate = InLearningRate;
	RebuildNetwork();
	SetState(ENFTrainingState::Idle);
}

void UNFTrainerComponent::ApplyProfile(const FNFTrainingProfile& Profile)
{
	TargetAccuracy = Profile.TargetAccuracy;
	TargetLoss = Profile.TargetLoss;
	MaxEpochs = Profile.MaxEpochs;
	EpochsPerSecond = Profile.EpochsPerSecond;
	SamplesPerSecond = Profile.SamplesPerSecond;
	MaxTrainingSamples = Profile.MaxTrainingSamples;
	TargetValidationAccuracy = Profile.TargetValidationAccuracy;
	ApplyRate();
}

float UNFTrainerComponent::GetSmoothedValidationAccuracy() const
{
	const int32 Count = FMath::Min(ValidationSmoothing, ValidationHistory.Num());
	if (Count == 0)
	{
		return Progress.ValidationAccuracy;
	}
	float Sum = 0.f;
	for (int32 i = ValidationHistory.Num() - Count; i < ValidationHistory.Num(); ++i)
	{
		Sum += ValidationHistory[i];
	}
	return Sum / Count;
}

float UNFTrainerComponent::Predict(float InputA, float InputB)
{
	const float Input[2] = { InputA, InputB };
	return Network.GetNumInputs() == 2 ? Network.Forward(Input)[0] : 0.f;
}

void UNFTrainerComponent::SetDataset(const FNFDataset& InDataset)
{
	check(InDataset.IsValid());
	Dataset = InDataset;
	Validation = FNFDataset();
	ApplyRate();
	RebuildNetwork();
	SetState(ENFTrainingState::Idle);
}

void UNFTrainerComponent::SetDatasets(const FNFDataset& InTrain, const FNFDataset& InValidation)
{
	check(InTrain.IsValid());
	Dataset = InTrain;
	Validation = InValidation.IsValid() ? InValidation : FNFDataset();
	ApplyRate();
	RebuildNetwork();
	SetState(ENFTrainingState::Idle);
}

bool UNFTrainerComponent::StartTraining()
{
	if (State == ENFTrainingState::Training)
	{
		return false;
	}

	// Chaque essai repart de poids neufs : même architecture + même graine = même résultat
	Network.InitializeWeights(Seed);
	ShuffleRng.Initialize(Seed + 1);
	ShuffleOrder.SetNum(Dataset.Num());
	for (int32 s = 0; s < ShuffleOrder.Num(); ++s)
	{
		ShuffleOrder[s] = s;
	}
	SampleCursor = Dataset.Num();	// une nouvelle epoch commencera au premier exemple
	Progress = FNFTrainingProgress();
	LossHistory.Reset();
	AccuracyHistory.Reset();
	ValidationHistory.Reset();
	EpochAccumulator = 0.f;

	const FNFEvaluation Initial = Network.Evaluate(Dataset);
	Progress.Loss = Initial.Loss;
	Progress.Accuracy = Initial.Accuracy;
	Progress.ValidationAccuracy = HasValidation() ? Network.Evaluate(Validation).Accuracy : -1.f;

	UE_LOG(LogNeuronForge, Log, TEXT("Entraînement : %s, lr %.3f, %d epochs max"),
		*Network.Describe(), LearningRate, MaxEpochs);

	SetState(ENFTrainingState::Training);
	OnProgress.Broadcast(Progress);
	return true;
}

void UNFTrainerComponent::StopTraining()
{
	if (State == ENFTrainingState::Training)
	{
		SetState(ENFTrainingState::Idle);
	}
}

void UNFTrainerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (State != ENFTrainingState::Training)
	{
		return;
	}

	// Avance exemple par exemple : les poids (et donc l'hologramme) évoluent en continu
	EpochAccumulator += DeltaTime * EpochsPerSecond * Dataset.Num();
	const int32 NumSamples = FMath::Min(FMath::FloorToInt32(EpochAccumulator), 4000);
	if (NumSamples > 0)
	{
		EpochAccumulator -= NumSamples;
		AdvanceSamples(NumSamples);
	}
}

void UNFTrainerComponent::AdvanceSamples(int32 NumSamples)
{
	for (int32 i = 0; i < NumSamples && State == ENFTrainingState::Training; ++i)
	{
		TrainNextSample();
	}
}

void UNFTrainerComponent::TrainNextSample()
{
	const int32 NumSamples = Dataset.Num();
	if (NumSamples == 0)
	{
		return;
	}

	// Début d'epoch : nouvel ordre de passage (même mélange que FNFNeuralNetwork::TrainEpoch)
	if (SampleCursor >= NumSamples)
	{
		for (int32 s = NumSamples - 1; s > 0; --s)
		{
			ShuffleOrder.Swap(s, ShuffleRng.RandRange(0, s));
		}
		SampleCursor = 0;
	}

	const int32 Index = ShuffleOrder[SampleCursor++];
	Network.TrainSample(Dataset.GetInput(Index), Dataset.GetTarget(Index), LearningRate);

	if (SampleCursor >= NumSamples)
	{
		FinishEpoch();
	}
}

void UNFTrainerComponent::FinishEpoch()
{
	++Progress.Epoch;
	const FNFEvaluation Evaluation = Network.Evaluate(Dataset);
	Progress.Loss = Evaluation.Loss;
	Progress.Accuracy = Evaluation.Accuracy;
	LossHistory.Add(Evaluation.Loss);
	AccuracyHistory.Add(Evaluation.Accuracy);
	if (HasValidation())
	{
		Progress.ValidationAccuracy = Network.Evaluate(Validation).Accuracy;
		ValidationHistory.Add(Progress.ValidationAccuracy);
	}
	OnProgress.Broadcast(Progress);

	if (IsTargetReached(Evaluation))
	{
		UE_LOG(LogNeuronForge, Log, TEXT("Modèle validé en %d epochs (entraînement %.1f %%, validation %.1f %%, perte %.4f)"),
			Progress.Epoch, Progress.Accuracy * 100.f, Progress.ValidationAccuracy * 100.f, Progress.Loss);
		SetState(ENFTrainingState::Succeeded);
	}
	else if (Progress.Epoch >= MaxEpochs)
	{
		UE_LOG(LogNeuronForge, Log, TEXT("Échec après %d epochs (précision %.0f %%, perte %.4f)"),
			Progress.Epoch, Progress.Accuracy * 100.f, Progress.Loss);
		SetState(ENFTrainingState::Failed);
	}
}

void UNFTrainerComponent::Advance(int32 NumEpochs)
{
	for (int32 e = 0; e < NumEpochs && State == ENFTrainingState::Training; ++e)
	{
		const int32 StartEpoch = Progress.Epoch;
		while (State == ENFTrainingState::Training && Progress.Epoch == StartEpoch)
		{
			TrainNextSample();
		}
	}
}

bool UNFTrainerComponent::IsTargetReached(const FNFEvaluation& Evaluation) const
{
	if (HasValidation())
	{
		// Ce qui compte : bien répondre sur des exemples jamais vus (moyenne des dernières epochs)
		return ValidationHistory.Num() >= ValidationSmoothing
			&& GetSmoothedValidationAccuracy() >= TargetValidationAccuracy - KINDA_SMALL_NUMBER;
	}
	return Evaluation.Accuracy >= TargetAccuracy - KINDA_SMALL_NUMBER && Evaluation.Loss <= TargetLoss;
}

void UNFTrainerComponent::GetPredictions(TArray<float>& OutPredictions)
{
	OutPredictions.Reset(Dataset.Num());
	for (int32 s = 0; s < Dataset.Num(); ++s)
	{
		OutPredictions.Add(Network.Forward(Dataset.GetInput(s))[0]);
	}
}

void UNFTrainerComponent::SetState(ENFTrainingState NewState)
{
	if (State != NewState)
	{
		State = NewState;
		OnStateChanged.Broadcast(NewState);
	}
}
