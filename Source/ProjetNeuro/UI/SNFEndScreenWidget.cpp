// Neuron Forge - ENSIM

#include "UI/SNFEndScreenWidget.h"
#include "Lab/NFExitTrigger.h"
#include "Lab/NFNetworkBuilder.h"
#include "UI/NFUIStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"

#define LOCTEXT_NAMESPACE "NeuronForgeEnd"

// ---------------------------------------------------------------------------
// Écran de fin

void SNFEndScreenWidget::Construct(const FArguments& InArgs)
{
	OnReplay = InArgs._OnReplay;
	OnQuit = InArgs._OnQuit;
	const FNFMissionSummary& Summary = InArgs._Summary;

	const int32 Minutes = FMath::FloorToInt32(Summary.Seconds / 60.f);
	const int32 Seconds = FMath::FloorToInt32(Summary.Seconds) % 60;
	const FText Duration = FText::FromString(FString::Printf(TEXT("%d min %02d s"), Minutes, Seconds));

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBackgroundBlur)
			.BlurStrength(8.f)
			[
				SNew(SBorder).BorderImage(NFUI::WhiteBrush()).BorderBackgroundColor(FLinearColor(0.f, 0.03f, 0.02f, 0.6f))
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(980.f)
			[
				SNew(SBorder)
				.BorderImage(NFUI::PanelBrush())
				.Padding(FMargin(48.f, 40.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SBorder)
						.BorderImage(NFUI::PillBrush())
						.BorderBackgroundColor(FLinearColor(0.2f, 1.f, 0.5f, 0.15f))
						.Padding(FMargin(16.f, 6.f))
						[
							SNew(STextBlock).Text(LOCTEXT("Badge", "NIVEAU 1 TERMINÉ")).Font(NFUI::Font(12, TEXT("Bold"))).ColorAndOpacity(NFUI::Success)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 16.f, 0.f, 6.f)
					[
						SNew(STextBlock).Text(LOCTEXT("Title", "MISSION ACCOMPLIE")).Font(NFUI::Font(48, TEXT("Bold"))).ColorAndOpacity(NFUI::Text)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Subtitle", "Votre réseau de neurones a résolu XOR : la porte du laboratoire s'est ouverte."))
						.Font(NFUI::Font(16)).ColorAndOpacity(NFUI::Muted)
					]

					// Statistiques
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 32.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.6f).Padding(0.f, 0.f, 8.f, 0.f)
						[
							MakeStat(LOCTEXT("StatArchi", "ARCHITECTURE"), FText::FromString(Summary.Architecture), NFUI::Accent)
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f)
						[
							MakeStat(LOCTEXT("StatEpochs", "EPOCHS"), FText::AsNumber(Summary.Epochs), NFUI::Text)
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f)
						[
							MakeStat(LOCTEXT("StatAttempts", "ESSAIS"), FText::AsNumber(Summary.Attempts), NFUI::Warning)
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 0.f, 0.f)
						[
							MakeStat(LOCTEXT("StatTime", "DURÉE"), Duration, NFUI::Success)
						]
					]

					// Leçon retenue
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 24.f, 0.f, 0.f)
					[
						SNew(SBorder)
						.BorderImage(NFUI::AccentCardBrush())
						.Padding(FMargin(24.f, 18.f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(LOCTEXT("LessonTitle", "CE QU'IL FAUT RETENIR")).Font(NFUI::Font(12, TEXT("Bold"))).ColorAndOpacity(NFUI::Accent)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("Lesson",
									"XOR n'est pas linéairement séparable : un modèle sans couche cachée, ou avec seulement des activations "
									"linéaires, ne peut tracer qu'une droite. Une couche cachée avec une activation non linéaire "
									"(ReLU, Tanh, Sigmoid) permet au réseau de plier l'espace et de séparer les 4 cas."))
								.Font(NFUI::Font(15)).ColorAndOpacity(NFUI::Text).AutoWrapText(true).LineHeightPercentage(1.15f)
							]
						]
					]

					// Boutons
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 30.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 14.f, 0.f)
						[
							SNew(SButton)
							.ButtonStyle(NFUI::PrimaryButton())
							.ContentPadding(FMargin(32.f, 12.f))
							.OnClicked_Lambda([this]() { OnReplay.ExecuteIfBound(); return FReply::Handled(); })
							[
								SNew(STextBlock).Text(LOCTEXT("Replay", "REJOUER")).Font(NFUI::Font(15, TEXT("Bold"))).ColorAndOpacity(FLinearColor(0.01f, 0.03f, 0.06f))
							]
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton)
							.ButtonStyle(NFUI::SecondaryButton())
							.ContentPadding(FMargin(32.f, 12.f))
							.OnClicked_Lambda([this]() { OnQuit.ExecuteIfBound(); return FReply::Handled(); })
							[
								SNew(STextBlock).Text(LOCTEXT("Quit", "QUITTER")).Font(NFUI::Font(15, TEXT("Bold"))).ColorAndOpacity(NFUI::Text)
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 14.f, 0.f, 0.f)
					[
						SNew(STextBlock).Text(LOCTEXT("Keys", "Entrée : rejouer    Échap : quitter")).Font(NFUI::Font(12)).ColorAndOpacity(NFUI::Muted)
					]
				]
			]
		]
	];

	SetRenderOpacity(0.f);
}

TSharedRef<SWidget> SNFEndScreenWidget::MakeStat(const FText& Label, const FText& Value, const FLinearColor& Color) const
{
	return SNew(SBorder)
		.BorderImage(NFUI::CardBrush())
		.Padding(FMargin(18.f, 16.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Label).Font(NFUI::Font(11, TEXT("Bold"))).ColorAndOpacity(NFUI::Muted)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(STextBlock).Text(Value).Font(NFUI::Font(22, TEXT("Bold"))).ColorAndOpacity(Color)
			]
		];
}

FReply SNFEndScreenWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		OnReplay.ExecuteIfBound();
		return FReply::Handled();
	}
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		OnQuit.ExecuteIfBound();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SNFEndScreenWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	Age += InDeltaTime;
	const float Alpha = NFUI::EaseOut(Age / 0.6f);
	SetRenderOpacity(Alpha);
	SetRenderTransform(FSlateRenderTransform(FVector2f(0.f, 30.f * (1.f - Alpha))));
}

// ---------------------------------------------------------------------------
// Panneau d'objectifs

void SNFObjectivesWidget::Construct(const FArguments& InArgs)
{
	Builder = InArgs._Builder;
	Exit = InArgs._Exit;

	ChildSlot
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Top)
	.Padding(FMargin(28.f, 28.f))
	[
		SNew(SBox).WidthOverride(350.f)
		[
			SNew(SBorder)
			.BorderImage(NFUI::CardBrush())
			.Padding(FMargin(18.f, 14.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(STextBlock).Text(LOCTEXT("ObjTitle", "MISSION · XOR")).Font(NFUI::Font(12, TEXT("Bold"))).ColorAndOpacity(NFUI::Accent)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock)
						.Font(NFUI::Font(12, TEXT("Bold")))
						.ColorAndOpacity(NFUI::Muted)
						.Text_Lambda([this]() -> FText
						{
							const int32 Attempts = Builder.IsValid() ? Builder->GetTrainingAttempts() : 0;
							return FText::Format(LOCTEXT("Attempts", "Essais : {0}"), FText::AsNumber(Attempts));
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)[MakeObjectiveRow(EObjective::Design, LOCTEXT("ObjDesign", "Assembler le réseau (Forge)"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[MakeObjectiveRow(EObjective::Train, LOCTEXT("ObjTrain", "Lancer l'entraînement"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[MakeObjectiveRow(EObjective::Validate, LOCTEXT("ObjValidate", "Atteindre 100 % de précision"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[MakeObjectiveRow(EObjective::Exit, LOCTEXT("ObjExit", "Quitter le laboratoire"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(STextBlock).Text(LOCTEXT("ObjHelp", "H : revoir le briefing")).Font(NFUI::Font(11)).ColorAndOpacity(NFUI::Muted)
				]
			]
		]
	];
}

bool SNFObjectivesWidget::IsDone(EObjective Objective) const
{
	const ANFNetworkBuilder* B = Builder.Get();
	switch (Objective)
	{
	case EObjective::Design:	return B && B->GetNumHiddenLayers() > 0;
	case EObjective::Train:		return B && B->GetTrainingAttempts() > 0;
	case EObjective::Validate:	return B && B->HasSucceeded();
	case EObjective::Exit:		return Exit.IsValid() && Exit->IsCompleted();
	default:					return false;
	}
}

bool SNFObjectivesWidget::IsCurrent(EObjective Objective) const
{
	// L'objectif courant est le premier qui n'est pas atteint
	for (EObjective Candidate : { EObjective::Design, EObjective::Train, EObjective::Validate, EObjective::Exit })
	{
		if (!IsDone(Candidate))
		{
			return Candidate == Objective;
		}
	}
	return false;
}

TSharedRef<SWidget> SNFObjectivesWidget::MakeObjectiveRow(EObjective Objective, const FText& Label)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
		[
			SNew(SBox).WidthOverride(18.f).HeightOverride(18.f)
			[
				SNew(SBorder)
				.BorderImage(NFUI::CircleBrush())
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				.BorderBackgroundColor_Lambda([this, Objective]() -> FSlateColor
				{
					if (IsDone(Objective))
					{
						return NFUI::Success;
					}
					return IsCurrent(Objective) ? NFUI::Accent : FLinearColor(0.2f, 0.28f, 0.36f);
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("✓")))
					.Font(NFUI::Font(10, TEXT("Bold")))
					.ColorAndOpacity(FLinearColor(0.01f, 0.05f, 0.03f))
					.Visibility_Lambda([this, Objective]() { return IsDone(Objective) ? EVisibility::Visible : EVisibility::Hidden; })
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(Label)
			.Font_Lambda([this, Objective]() { return NFUI::Font(13, IsCurrent(Objective) ? TEXT("Bold") : TEXT("Regular")); })
			.ColorAndOpacity_Lambda([this, Objective]() -> FSlateColor
			{
				if (IsDone(Objective))
				{
					return NFUI::Muted;
				}
				return IsCurrent(Objective) ? NFUI::Text : FLinearColor(0.6f, 0.68f, 0.76f);
			})
		];
}

#undef LOCTEXT_NAMESPACE
