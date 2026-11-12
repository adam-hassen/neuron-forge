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
	EpochAccumulator = 0.f;
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
	EpochAccumulator = 0.f;

	const FNFEvaluation Initial = Network.Evaluate(Dataset);
	Progress.Loss = Initial.Loss;
	Progress.Accuracy = Initial.Accuracy;

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
	OnProgress.Broadcast(Progress);

	if (IsTargetReached(Evaluation))
	{
		UE_LOG(LogNeuronForge, Log, TEXT("Modèle validé en %d epochs (précision %.1f %%, perte %.4f)"),
			Progress.Epoch, Progress.Accuracy * 100.f, Progress.Loss);
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
