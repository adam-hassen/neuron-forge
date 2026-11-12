// Neuron Forge - ENSIM

#include "NeuralNetwork/NFExperimentSet.h"

namespace NFExperiments
{
	/** Distance « elliptique » au centre de la zone de survie : < 1 dedans, > 1 dehors. */
	float ZoneDistance(float Water, float Light)
	{
		const float W = (Water - FNFDataset::GreenhouseWaterCenter) / FNFDataset::GreenhouseWaterRadius;
		const float L = (Light - FNFDataset::GreenhouseLightCenter) / FNFDataset::GreenhouseLightRadius;
		return FMath::Sqrt(W * W + L * L);
	}

	/** Un point tiré au hasard dont la distance à la zone est dans [MinDist, MaxDist]. */
	FVector2f RandomPointInBand(FRandomStream& Rng, float MinDist, float MaxDist)
	{
		for (int32 Attempt = 0; Attempt < 1000; ++Attempt)
		{
			const FVector2f P(Rng.FRandRange(0.3f, 9.7f), Rng.FRandRange(0.3f, 9.7f));
			const float D = ZoneDistance(P.X, P.Y);
			if (D >= MinDist && D <= MaxDist)
			{
				return P;
			}
		}
		return FVector2f(FNFDataset::GreenhouseWaterCenter, FNFDataset::GreenhouseLightCenter);
	}

	float RoundTenth(float Value)
	{
		return FMath::RoundToFloat(Value * 10.f) / 10.f;
	}
}

FNFExperimentSet FNFExperimentSet::MakeGreenhouse(int32 NumClean, int32 DefectsPerType, int32 Seed)
{
	using namespace NFExperiments;
	FNFExperimentSet Set;

	// Les bonnes expériences : exactement celles du dataset historique de la serre
	const FNFDataset Clean = FNFDataset::MakeGreenhouse(NumClean, Seed);
	for (int32 s = 0; s < Clean.Num(); ++s)
	{
		FNFExperiment E;
		E.Water = RoundTenth(Clean.GetRawInput(s)[0]);
		E.Light = RoundTenth(Clean.GetRawInput(s)[1]);
		E.bSurvived = Clean.GetTarget(s)[0] >= 0.5f;
		Set.Experiments.Add(E);
	}

	FRandomStream Rng(Seed + 1);
	auto AddDefect = [&Set](FNFExperiment E, ENFDataDefect Type)
	{
		E.Defect = Type;
		Set.Experiments.Add(E);
	};

	for (int32 i = 0; i < DefectsPerType; ++i)
	{
		// Valeur impossible : un capteur déréglé affiche une valeur hors de l'échelle 0 à 10
		{
			const FVector2f P = RandomPointInBand(Rng, 0.f, 3.f);
			FNFExperiment E;
			E.Water = RoundTenth(P.X);
			E.Light = RoundTenth(P.Y);
			E.bSurvived = FNFDataset::GreenhouseSurvives(P.X, P.Y);
			const float Wrong = Rng.FRand() < 0.6f ? Rng.FRandRange(12.f, 19.f) : -Rng.FRandRange(2.f, 6.f);
			(i % 2 == 0 ? E.Water : E.Light) = RoundTenth(Wrong);
			AddDefect(E, ENFDataDefect::Impossible);
		}
		// Valeur manquante : plante qui a survécu, mais un capteur en panne n'a rien noté
		{
			const FVector2f P = RandomPointInBand(Rng, 0.f, 0.8f);
			FNFExperiment E;
			E.Water = RoundTenth(P.X);
			E.Light = RoundTenth(P.Y);
			E.bSurvived = true;
			(i % 2 == 0 ? E.bWaterMissing : E.bLightMissing) = true;
			AddDefect(E, ENFDataDefect::Missing);
		}
		// Doublon : une bonne expérience enregistrée deux fois
		{
			const int32 Original = Rng.RandRange(0, NumClean - 1);
			FNFExperiment E = Set.Experiments[Original];
			E.DuplicateOf = Original;
			AddDefect(E, ENFDataDefect::Duplicate);
		}
		// Étiquette fausse : bien au milieu d'une zone, mais le résultat noté est le contraire
		{
			const bool bInside = (i % 2 == 0);
			const FVector2f P = bInside ? RandomPointInBand(Rng, 0.f, 0.7f) : RandomPointInBand(Rng, 1.35f, 10.f);
			FNFExperiment E;
			E.Water = RoundTenth(P.X);
			E.Light = RoundTenth(P.Y);
			E.bSurvived = !bInside;
			AddDefect(E, ENFDataDefect::WrongLabel);
		}
	}

	// Mélange : les défauts sont noyés parmi les bonnes expériences (les doublons gardent leur original)
	TArray<int32> Order;
	for (int32 i = 0; i < Set.Experiments.Num(); ++i)
	{
		Order.Add(i);
	}
	for (int32 i = Order.Num() - 1; i > 0; --i)
	{
		Order.Swap(i, Rng.RandRange(0, i));
	}
	TArray<int32> NewIndexOf;
	NewIndexOf.SetNum(Order.Num());
	for (int32 NewIndex = 0; NewIndex < Order.Num(); ++NewIndex)
	{
		NewIndexOf[Order[NewIndex]] = NewIndex;
	}
	TArray<FNFExperiment> Shuffled;
	for (int32 NewIndex = 0; NewIndex < Order.Num(); ++NewIndex)
	{
		FNFExperiment E = Set.Experiments[Order[NewIndex]];
		E.Id = NewIndex + 1;
		if (E.DuplicateOf != INDEX_NONE)
		{
			E.DuplicateOf = NewIndexOf[E.DuplicateOf];
		}
		Shuffled.Add(E);
	}
	Set.Experiments = MoveTemp(Shuffled);
	return Set;
}

int32 FNFExperimentSet::CountDefects(const TArray<int32>& Indices, ENFDataDefect Type) const
{
	int32 Count = 0;
	for (int32 Index : Indices)
	{
		const FNFExperiment& E = Experiments[Index];
		Count += (Type == ENFDataDefect::None ? E.IsDefective() : E.Defect == Type) ? 1 : 0;
	}
	return Count;
}

TArray<int32> FNFExperimentSet::AllIndices() const
{
	TArray<int32> Indices;
	for (int32 i = 0; i < Experiments.Num(); ++i)
	{
		Indices.Add(i);
	}
	return Indices;
}

TArray<int32> FNFExperimentSet::CleanIndices() const
{
	TArray<int32> Indices;
	for (int32 i = 0; i < Experiments.Num(); ++i)
	{
		if (!Experiments[i].IsDefective())
		{
			Indices.Add(i);
		}
	}
	return Indices;
}

FNFDataset FNFExperimentSet::ToDataset(const TArray<int32>& Indices, const FString& Name) const
{
	FNFDataset Dataset;
	Dataset.Name = Name;
	Dataset.NumInputs = 2;
	Dataset.NumOutputs = 1;
	Dataset.InputNames = { TEXT("Eau"), TEXT("Lumière") };
	Dataset.OutputName = TEXT("Survie");
	Dataset.RawMin = 0.f;
	Dataset.RawMax = 10.f;
	for (int32 Index : Indices)
	{
		const FNFExperiment& E = Experiments[Index];
		// Pas de garde-fou : ce qui n'a pas été nettoyé est lu tel quel
		Dataset.AddRawSample({ E.GetTrainingWater(), E.GetTrainingLight() }, { E.bSurvived ? 1.f : 0.f });
	}
	return Dataset;
}

bool FNFExperimentSet::ContradictsNeighbors(int32 Index, const TArray<int32>& Among, int32 K, int32 MinDisagree) const
{
	const FNFExperiment& Self = Experiments[Index];
	if (Self.HasMissingValue())
	{
		return false;
	}
	TArray<TPair<float, int32>> Nearest;
	for (int32 Other : Among)
	{
		const FNFExperiment& E = Experiments[Other];
		if (Other == Index || E.HasMissingValue() || (E.Water == Self.Water && E.Light == Self.Light))
		{
			continue;
		}
		Nearest.Add({ FMath::Square(E.Water - Self.Water) + FMath::Square(E.Light - Self.Light), Other });
	}
	Nearest.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) { return A.Key < B.Key; });
	int32 Disagree = 0;
	for (int32 n = 0; n < FMath::Min(K, Nearest.Num()); ++n)
	{
		Disagree += Experiments[Nearest[n].Value].bSurvived != Self.bSurvived ? 1 : 0;
	}
	return Nearest.Num() >= K && Disagree >= MinDisagree;
}

void FNFExperimentSet::Split(const TArray<int32>& Kept, float TrainFraction, int32 Seed, TArray<int32>& OutTrain, TArray<int32>& OutValidation,
	int32 SmallBatch)
{
	TArray<int32> Shuffled = Kept;
	FRandomStream Rng(Seed);
	for (int32 i = Shuffled.Num() - 1; i > 0; --i)
	{
		Shuffled.Swap(i, Rng.RandRange(0, i));
	}
	const int32 NumTrain = SmallBatch > 0
		? FMath::Min(SmallBatch, Shuffled.Num())
		: FMath::Clamp(FMath::RoundToInt32(Shuffled.Num() * TrainFraction), 0, Shuffled.Num());
	OutTrain.Reset();
	OutValidation.Reset();
	for (int32 i = 0; i < Shuffled.Num(); ++i)
	{
		(i < NumTrain ? OutTrain : OutValidation).Add(Shuffled[i]);
	}
}
