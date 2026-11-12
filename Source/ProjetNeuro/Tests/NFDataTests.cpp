// Neuron Forge - ENSIM
//
// Tests du cycle des données : expériences brutes et leurs défauts, découpage entraînement / validation,
// et mesures de calibrage (effet des défauts, du découpage, de la taille du réseau, du petit lot).

#include "Misc/AutomationTest.h"
#include "NeuralNetwork/NFExperimentSet.h"
#include "Tests/NFTestWorld.h"
#include "Training/NFTrainerComponent.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace NFDataTests
{
	struct FRun
	{
		bool bSucceeded = false;
		int32 Epochs = 0;
		float Seconds = 0.f;
		float TrainAccuracy = 0.f;
		float ValidationAccuracy = 0.f;
		float BestValidation = 0.f;
		int32 NumTrain = 0;
		int32 NumValidation = 0;
	};

	/** Entraîne exactement comme en jeu (vrai composant, profil de la serre), jusqu'à réussite ou échec. */
	FRun Train(const FNFExperimentSet& Set, const TArray<int32>& Kept, const TArray<FNFLayerConfig>& Hidden,
		float TrainFraction = 0.7f, int32 SmallBatch = 0, float LearningRate = 0.1f, int32 SplitSeed = 7)
	{
		TArray<int32> TrainIdx, ValIdx;
		FNFExperimentSet::Split(Kept, TrainFraction, SplitSeed, TrainIdx, ValIdx, SmallBatch);

		UNFTrainerComponent* Trainer = NewObject<UNFTrainerComponent>(GetTransientPackage());
		Trainer->ApplyProfile(FNFGreenhouseProfile());
		Trainer->SetDatasets(Set.ToDataset(TrainIdx), Set.ToDataset(ValIdx));
		Trainer->Configure(Hidden, LearningRate);
		Trainer->StartTraining();
		FRun Run;
		while (Trainer->IsTraining())
		{
			Trainer->Advance(1);
			Run.BestValidation = FMath::Max(Run.BestValidation, Trainer->GetProgress().ValidationAccuracy);
		}
		Run.bSucceeded = Trainer->GetState() == ENFTrainingState::Succeeded;
		Run.Epochs = Trainer->GetProgress().Epoch;
		Run.Seconds = Run.Epochs / Trainer->EpochsPerSecond;
		Run.TrainAccuracy = Trainer->GetProgress().Accuracy;
		Run.ValidationAccuracy = Trainer->GetSmoothedValidationAccuracy();
		Run.NumTrain = TrainIdx.Num();
		Run.NumValidation = ValIdx.Num();
		Trainer->MarkAsGarbage();
		return Run;
	}

	FString Describe(const TCHAR* Name, const FRun& R)
	{
		return FString::Printf(TEXT("%-34s %s en %3d epochs (%4.1f s) | entr. %3.0f %% / valid. %3.0f %% (max %3.0f %%) | %d / %d ex."),
			Name, R.bSucceeded ? TEXT("RÉUSSI") : TEXT("échec "), R.Epochs, R.Seconds, R.TrainAccuracy * 100.f,
			R.ValidationAccuracy * 100.f, R.BestValidation * 100.f, R.NumTrain, R.NumValidation);
	}

	/** Toutes les bonnes expériences + Keep défauts de chaque type (les premiers trouvés). */
	TArray<int32> KeepDefects(const FNFExperimentSet& Set, int32 KeepPerType, ENFDataDefect OnlyType = ENFDataDefect::None)
	{
		TArray<int32> Kept = Set.CleanIndices();
		TMap<ENFDataDefect, int32> Counts;
		for (int32 i = 0; i < Set.Num(); ++i)
		{
			const ENFDataDefect Type = Set.Experiments[i].Defect;
			if (Type == ENFDataDefect::None || (OnlyType != ENFDataDefect::None && Type != OnlyType))
			{
				continue;
			}
			int32& Count = Counts.FindOrAdd(Type);
			if (Count < KeepPerType)
			{
				Kept.Add(i);
				++Count;
			}
		}
		return Kept;
	}
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFDataExperimentsTest, "NeuronForge.Data.Experiments", NFTestUtils::Flags)

bool FNFDataExperimentsTest::RunTest(const FString& Parameters)
{
	const FNFExperimentSet Set = FNFExperimentSet::MakeGreenhouse();
	TestEqual(TEXT("200 expériences"), Set.Num(), 200);
	TestEqual(TEXT("40 défauts"), Set.CountDefects(Set.AllIndices()), 40);
	for (ENFDataDefect Type : { ENFDataDefect::Impossible, ENFDataDefect::Missing, ENFDataDefect::Duplicate, ENFDataDefect::WrongLabel })
	{
		TestEqual(FString::Printf(TEXT("10 défauts de type %d"), static_cast<int32>(Type)), Set.CountDefects(Set.AllIndices(), Type), 10);
	}
	TestEqual(TEXT("Reproductible"), FNFExperimentSet::MakeGreenhouse().Experiments[17].Water, Set.Experiments[17].Water);

	TSet<int32> Ids;
	for (int32 i = 0; i < Set.Num(); ++i)
	{
		const FNFExperiment& E = Set.Experiments[i];
		Ids.Add(E.Id);
		const bool bInRange = E.Water >= 0.f && E.Water <= 10.f && E.Light >= 0.f && E.Light <= 10.f;
		switch (E.Defect)
		{
		case ENFDataDefect::None:
			TestTrue(TEXT("Bonne expérience : valeurs dans 0..10"), bInRange && !E.HasMissingValue());
			break;
		case ENFDataDefect::Impossible:
			TestFalse(TEXT("Valeur impossible : hors de 0..10"), bInRange);
			break;
		case ENFDataDefect::Missing:
			TestTrue(TEXT("Valeur manquante signalée"), E.HasMissingValue());
			TestEqual(TEXT("Une valeur manquante est lue comme 0"), E.bWaterMissing ? E.GetTrainingWater() : E.GetTrainingLight(), 0.f);
			break;
		case ENFDataDefect::Duplicate:
		{
			const FNFExperiment& Original = Set.Experiments[E.DuplicateOf];
			TestTrue(TEXT("Doublon identique à l'original"), !Original.IsDefective() && Original.Water == E.Water
				&& Original.Light == E.Light && Original.bSurvived == E.bSurvived);
			break;
		}
		case ENFDataDefect::WrongLabel:
			TestNotEqual(TEXT("Étiquette fausse : contraire de la vraie serre"), E.bSurvived, FNFDataset::GreenhouseSurvives(E.Water, E.Light));
			break;
		}
	}
	TestEqual(TEXT("Numéros uniques"), Ids.Num(), 200);

	// Scanner : les étiquettes fausses contredisent leurs voisines, les bonnes expériences rarement
	int32 WrongFlagged = 0, CleanFlagged = 0;
	const TArray<int32> All = Set.AllIndices();
	for (int32 i = 0; i < Set.Num(); ++i)
	{
		if (Set.ContradictsNeighbors(i, All))
		{
			(Set.Experiments[i].Defect == ENFDataDefect::WrongLabel ? WrongFlagged : CleanFlagged)++;
		}
	}
	AddInfo(FString::Printf(TEXT("Scanner : %d étiquettes fausses sur 10 repérées, %d autres expériences signalées"), WrongFlagged, CleanFlagged));
	TestTrue(TEXT("Le scanner repère au moins 8 étiquettes fausses sur 10"), WrongFlagged >= 8);
	TestTrue(TEXT("Le scanner signale peu d'autres expériences"), CleanFlagged <= 8);
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFDataSplitTest, "NeuronForge.Data.Split", NFTestUtils::Flags)

bool FNFDataSplitTest::RunTest(const FString& Parameters)
{
	const FNFExperimentSet Set = FNFExperimentSet::MakeGreenhouse();
	const TArray<int32> Kept = Set.CleanIndices();
	for (float Fraction : { 0.6f, 0.7f, 0.8f, 0.9f })
	{
		TArray<int32> Train, Val;
		FNFExperimentSet::Split(Kept, Fraction, 7, Train, Val);
		TestEqual(FString::Printf(TEXT("Entraînement %.0f %%"), Fraction * 100.f), Train.Num(), FMath::RoundToInt32(Kept.Num() * Fraction));
		TestEqual(TEXT("Rien de perdu"), Train.Num() + Val.Num(), Kept.Num());
		TSet<int32> Seen(Train);
		bool bDisjoint = true;
		for (int32 Index : Val)
		{
			bDisjoint &= !Seen.Contains(Index);
		}
		TestTrue(TEXT("Une expérience est soit en entraînement, soit en validation"), bDisjoint);
	}
	TArray<int32> TrainA, ValA, TrainB, ValB;
	FNFExperimentSet::Split(Kept, 0.7f, 7, TrainA, ValA);
	FNFExperimentSet::Split(Kept, 0.7f, 7, TrainB, ValB);
	TestTrue(TEXT("Découpage reproductible"), TrainA == TrainB);
	FNFExperimentSet::Split(Kept, 0.7f, 7, TrainA, ValA, 20);
	TestEqual(TEXT("Petit lot : 20 expériences d'entraînement"), TrainA.Num(), 20);
	TestEqual(TEXT("Petit lot : tout le reste en validation"), ValA.Num(), Kept.Num() - 20);
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNFDataCalibrationTest, "NeuronForge.Data.Calibration", NFTestUtils::Flags)

bool FNFDataCalibrationTest::RunTest(const FString& Parameters)
{
	using namespace NFDataTests;
	const FNFExperimentSet Set = FNFExperimentSet::MakeGreenhouse();
	const TArray<FNFLayerConfig> Small = { { 5, ENFActivation::ReLU } };
	const TArray<FNFLayerConfig> Big = { { 6, ENFActivation::ReLU }, { 6, ENFActivation::ReLU }, { 6, ENFActivation::ReLU } };
	const TArray<FNFLayerConfig> Tanh5 = { { 5, ENFActivation::Tanh } };

	const FRun Clean = Train(Set, Set.CleanIndices(), Small);
	const FRun CleanTanh = Train(Set, Set.CleanIndices(), Tanh5);
	const FRun Dirty = Train(Set, Set.AllIndices(), Small);
	const FRun Door = Train(Set, KeepDefects(Set, 2), Small);
	AddInfo(Describe(TEXT("Propre, 5 ReLU"), Clean));
	AddInfo(Describe(TEXT("Propre, 5 Tanh"), CleanTanh));
	AddInfo(Describe(TEXT("Tous les défauts, 5 ReLU"), Dirty));
	AddInfo(Describe(TEXT("8 défauts restants, 5 ReLU"), Door));
	AddInfo(Describe(TEXT("4 défauts restants (1/type), 5 ReLU"), Train(Set, KeepDefects(Set, 1), Small)));
	AddInfo(Describe(TEXT("4 défauts restants (1/type), 5 Tanh"), Train(Set, KeepDefects(Set, 1), Tanh5)));
	AddInfo(Describe(TEXT("2 impossibles restantes"), Train(Set, KeepDefects(Set, 2, ENFDataDefect::Impossible), Small)));
	AddInfo(Describe(TEXT("1 impossible restante"), Train(Set, KeepDefects(Set, 1, ENFDataDefect::Impossible), Small)));
	AddInfo(Describe(TEXT("4 étiquettes fausses restantes"), Train(Set, KeepDefects(Set, 4, ENFDataDefect::WrongLabel), Small)));
	AddInfo(Describe(TEXT("2 étiquettes fausses restantes"), Train(Set, KeepDefects(Set, 2, ENFDataDefect::WrongLabel), Small)));
	AddInfo(Describe(TEXT("3 manquantes restantes"), Train(Set, KeepDefects(Set, 3, ENFDataDefect::Missing), Small)));
	for (const TPair<ENFDataDefect, const TCHAR*>& Type : TArray<TPair<ENFDataDefect, const TCHAR*>>{
		{ ENFDataDefect::Impossible, TEXT("Seulement les valeurs impossibles") }, { ENFDataDefect::Missing, TEXT("Seulement les valeurs manquantes") },
		{ ENFDataDefect::Duplicate, TEXT("Seulement les doublons") }, { ENFDataDefect::WrongLabel, TEXT("Seulement les étiquettes fausses") } })
	{
		AddInfo(Describe(Type.Value, Train(Set, KeepDefects(Set, 10, Type.Key), Small)));
	}
	for (float Fraction : { 0.6f, 0.8f, 0.9f })
	{
		AddInfo(Describe(*FString::Printf(TEXT("Propre, découpage %.0f/%.0f"), Fraction * 100.f, 100.f - Fraction * 100.f),
			Train(Set, Set.CleanIndices(), Small, Fraction)));
	}
	// Robustesse : 8 découpages différents
	struct FCase { const TCHAR* Name; TArray<int32> Kept; TArray<FNFLayerConfig> Hidden; int32 SmallBatch; };
	const TArray<FCase> Cases = {
		{ TEXT("propre 5 ReLU"), Set.CleanIndices(), Small, 0 },
		{ TEXT("propre 5 Tanh"), Set.CleanIndices(), Tanh5, 0 },
		{ TEXT("propre 6-6-6"), Set.CleanIndices(), Big, 0 },
		{ TEXT("1 impossible"), KeepDefects(Set, 1, ENFDataDefect::Impossible), Small, 0 },
		{ TEXT("3 manquantes"), KeepDefects(Set, 3, ENFDataDefect::Missing), Small, 0 },
		{ TEXT("4 étiq. fausses"), KeepDefects(Set, 4, ENFDataDefect::WrongLabel), Small, 0 },
		{ TEXT("1 de chaque"), KeepDefects(Set, 1), Small, 0 },
		{ TEXT("3 restants"), KeepDefects(Set, 1, ENFDataDefect::None).FilterByPredicate([&Set](int32 i) { return Set.Experiments[i].Defect != ENFDataDefect::Duplicate; }), Small, 0 },
		{ TEXT("2 restants"), KeepDefects(Set, 1, ENFDataDefect::None).FilterByPredicate([&Set](int32 i) { return Set.Experiments[i].Defect != ENFDataDefect::Duplicate && Set.Experiments[i].Defect != ENFDataDefect::Impossible; }), Small, 0 },
		{ TEXT("tous les défauts"), Set.AllIndices(), Small, 0 },
		{ TEXT("petit lot 5 ReLU"), Set.CleanIndices(), Small, 20 },
		{ TEXT("petit lot 6-6-6"), Set.CleanIndices(), Big, 20 },
	};
	TMap<FString, TArray<FRun>> Runs;
	for (const FCase& Case : Cases)
	{
		FString Line = FString::Printf(TEXT("ROBUSTESSE %-18s"), Case.Name);
		int32 Wins = 0;
		TArray<FRun>& CaseRuns = Runs.Add(Case.Name);
		for (int32 SplitSeed = 1; SplitSeed <= 8; ++SplitSeed)
		{
			const FRun R = Train(Set, Case.Kept, Case.Hidden, 0.7f, Case.SmallBatch, 0.1f, SplitSeed);
			CaseRuns.Add(R);
			Wins += R.bSucceeded ? 1 : 0;
			Line += FString::Printf(TEXT(" %s%.0f/%.0f@%.0fs"), R.bSucceeded ? TEXT("+") : TEXT("-"), R.TrainAccuracy * 100.f, R.BestValidation * 100.f, R.Seconds);
		}
		AddInfo(FString::Printf(TEXT("%s | %d/8"), *Line, Wins));
	}
	auto Wins = [&Runs](const TCHAR* Name) { return Runs[Name].FilterByPredicate([](const FRun& R) { return R.bSucceeded; }).Num(); };

	// Un bon modèle sur des données propres réussit, en 10 à 30 secondes de jeu environ
	TestTrue(TEXT("Données propres, 5 ReLU : au moins 7 réussites sur 8"), Wins(TEXT("propre 5 ReLU")) >= 7);
	for (const FRun& R : Runs[TEXT("propre 5 ReLU")])
	{
		TestTrue(TEXT("Réussite en moins de 30 s de jeu"), !R.bSucceeded || R.Seconds <= 30.f);
	}
	// Honnêteté : sur ce problème simple, un gros réseau ne fait ni mieux ni pire
	TestTrue(TEXT("Gros réseau 6-6-6 : autant de réussites que le petit, à 2 près"),
		FMath::Abs(Wins(TEXT("propre 6-6-6")) - Wins(TEXT("propre 5 ReLU"))) <= 2);
	// Les défauts dégradent réellement l'entraînement
	TestEqual(TEXT("Avec tous les défauts, le modèle échoue toujours"), Wins(TEXT("tous les défauts")), 0);
	for (const FRun& R : Runs[TEXT("tous les défauts")])
	{
		TestTrue(TEXT("Avec tous les défauts, la validation reste sous 80 %"), R.BestValidation < 0.8f);
	}
	TestTrue(TEXT("Un défaut de chaque type gêne déjà le modèle"), Wins(TEXT("1 de chaque")) < Wins(TEXT("propre 5 ReLU")));
	TestTrue(TEXT("Seuil de la porte (3 défauts restants) : le modèle réussit le plus souvent"), Wins(TEXT("3 restants")) >= 5);
	// Petit lot : le modèle apprend par cœur ses 20 exemples, la validation ne suit pas
	int32 Memorized = 0;
	for (const FRun& R : Runs[TEXT("petit lot 5 ReLU")])
	{
		Memorized += (R.TrainAccuracy >= 0.95f && R.TrainAccuracy - R.ValidationAccuracy >= 0.08f) ? 1 : 0;
	}
	TestTrue(TEXT("Petit lot : entraînement ≥ 95 % et validation au moins 8 points dessous, 6 fois sur 8"), Memorized >= 6);

	const FRun BigClean = Train(Set, Set.CleanIndices(), Big);
	const FRun SmallBatch = Train(Set, Set.CleanIndices(), Small, 0.7f, 20);
	const FRun BigSmallBatch = Train(Set, Set.CleanIndices(), Big, 0.7f, 20);
	AddInfo(Describe(TEXT("Propre, gros réseau 6-6-6"), BigClean));
	AddInfo(Describe(TEXT("Petit lot (20), 5 ReLU"), SmallBatch));
	AddInfo(Describe(TEXT("Petit lot (20), 6-6-6"), BigSmallBatch));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
