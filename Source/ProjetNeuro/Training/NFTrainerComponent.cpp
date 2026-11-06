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

	EpochAccumulator += DeltaTime * EpochsPerSecond;
	const int32 NumEpochs = FMath::FloorToInt32(EpochAccumulator);
	if (NumEpochs > 0)
	{
		EpochAccumulator -= NumEpochs;
		Advance(NumEpochs);
	}
}

void UNFTrainerComponent::Advance(int32 NumEpochs)
{
	if (State != ENFTrainingState::Training || NumEpochs <= 0)
	{
		return;
	}

	for (int32 i = 0; i < NumEpochs && Progress.Epoch < MaxEpochs; ++i)
	{
		Network.TrainEpoch(Dataset, LearningRate, ShuffleRng);
		++Progress.Epoch;

		const FNFEvaluation Evaluation = Network.Evaluate(Dataset);
		Progress.Loss = Evaluation.Loss;
		Progress.Accuracy = Evaluation.Accuracy;
		LossHistory.Add(Evaluation.Loss);

		if (IsTargetReached(Evaluation))
		{
			UE_LOG(LogNeuronForge, Log, TEXT("Modèle validé en %d epochs (perte %.4f)"), Progress.Epoch, Progress.Loss);
			OnProgress.Broadcast(Progress);
			SetState(ENFTrainingState::Succeeded);
			return;
		}
	}

	OnProgress.Broadcast(Progress);

	if (Progress.Epoch >= MaxEpochs)
	{
		UE_LOG(LogNeuronForge, Log, TEXT("Échec après %d epochs (précision %.0f %%, perte %.4f)"),
			Progress.Epoch, Progress.Accuracy * 100.f, Progress.Loss);
		SetState(ENFTrainingState::Failed);
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
