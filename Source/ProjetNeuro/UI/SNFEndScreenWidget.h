// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/** Bilan de la mission affiché sur l'écran de fin. */
struct FNFMissionSummary
{
	FString Architecture;
	float LearningRate = 0.f;
	int32 Epochs = 0;
	float Accuracy = 0.f;
	int32 Attempts = 0;
	float FinalLoss = 0.f;
	float Seconds = 0.f;
};

/** Écran « Mission accomplie » : bilan, leçon retenue, boutons Rejouer / Quitter. */
class PROJETNEURO_API SNFEndScreenWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNFEndScreenWidget) {}
		SLATE_ARGUMENT(FNFMissionSummary, Summary)
		SLATE_EVENT(FSimpleDelegate, OnReplay)
		SLATE_EVENT(FSimpleDelegate, OnQuit)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TSharedRef<SWidget> MakeStat(const FText& Label, const FText& Value, const FLinearColor& Color) const;

	FSimpleDelegate OnReplay;
	FSimpleDelegate OnQuit;
	float Age = 0.f;
};

class ANFNetworkBuilder;
class ANFExitTrigger;
class ANFPlantPot;
class ANFGreenhouse;

/**
 * Panneau d'objectifs en haut à gauche pendant la partie :
 * liste des étapes de la mission (cochées au fur et à mesure), nombre d'essais, rappel de l'aide.
 */
class PROJETNEURO_API SNFObjectivesWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNFObjectivesWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ANFNetworkBuilder>, Builder)
		SLATE_ARGUMENT(TWeakObjectPtr<ANFExitTrigger>, Exit)
		SLATE_ARGUMENT(TWeakObjectPtr<ANFPlantPot>, Pot)
		SLATE_ARGUMENT(TWeakObjectPtr<ANFGreenhouse>, Greenhouse)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	enum class EObjective : uint8 { Design, Train, Validate, TestPlant, Greenhouse, Exit };

	bool IsDone(EObjective Objective) const;
	bool IsCurrent(EObjective Objective) const;
	TSharedRef<SWidget> MakeObjectiveRow(EObjective Objective, const TAttribute<FText>& Label);

	TWeakObjectPtr<ANFNetworkBuilder> Builder;
	TWeakObjectPtr<ANFExitTrigger> Exit;
	TWeakObjectPtr<ANFPlantPot> Pot;
	TWeakObjectPtr<ANFGreenhouse> Greenhouse;
};
