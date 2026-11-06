// Neuron Forge - ENSIM

#include "UI/SNFIntroWidget.h"
#include "NeuralNetwork/NFActivation.h"
#include "UI/NFUIStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SOverlay.h"

#define LOCTEXT_NAMESPACE "NeuronForgeIntro"

namespace
{
	TSharedRef<SWidget> MakeText(const FText& Text, int32 Size, const FLinearColor& Color, FName Weight = TEXT("Regular"), bool bWrap = false)
	{
		return SNew(STextBlock)
			.Text(Text)
			.Font(NFUI::Font(Size, Weight))
			.ColorAndOpacity(Color)
			.AutoWrapText(bWrap);
	}

	TSharedRef<SWidget> MakeParagraph(const FText& Text, int32 Size = 15)
	{
		return SNew(STextBlock)
			.Text(Text)
			.Font(NFUI::Font(Size))
			.ColorAndOpacity(NFUI::Text)
			.AutoWrapText(true)
			.LineHeightPercentage(1.15f);
	}

	/** Petit intitulé de section en majuscules, couleur d'accent. */
	TSharedRef<SWidget> MakeEyebrow(const FText& Text)
	{
		return MakeText(Text, 12, NFUI::Accent, TEXT("Bold"));
	}

	/** Pastille ronde numérotée. */
	TSharedRef<SWidget> MakeBadge(const FText& Label, const FLinearColor& Color, float Size = 40.f)
	{
		return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
		[
			SNew(SBorder)
			.BorderImage(NFUI::CircleBrush())
			.BorderBackgroundColor(Color)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				MakeText(Label, FMath::RoundToInt(Size * 0.42f), FLinearColor(0.01f, 0.03f, 0.06f), TEXT("Bold"))
			]
		];
	}

	/** Touche de clavier stylisée. */
	TSharedRef<SWidget> MakeKey(const FText& Key)
	{
		return SNew(SBox).MinDesiredWidth(54.f).HeightOverride(34.f)
		[
			SNew(SBorder)
			.BorderImage(NFUI::PillBrush())
			.BorderBackgroundColor(FLinearColor(0.15f, 0.25f, 0.35f, 1.f))
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			.Padding(FMargin(10.f, 0.f))
			[
				MakeText(Key, 13, NFUI::Text, TEXT("Bold"))
			]
		];
	}

	/** Pastille de couleur + libellé (légende). */
	TSharedRef<SWidget> MakeSwatch(const FLinearColor& Color, const FText& Label)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SBox).WidthOverride(16.f).HeightOverride(16.f)
				[
					SNew(SBorder).BorderImage(NFUI::CircleBrush()).BorderBackgroundColor(Color)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				MakeText(Label, 14, NFUI::Text)
			];
	}

	/** Carte d'étape de la mission. */
	TSharedRef<SWidget> MakeStepCard(int32 Number, const FText& Title, const FText& Body, const FLinearColor& Color)
	{
		return SNew(SBorder)
			.BorderImage(NFUI::CardBrush())
			.Padding(FMargin(22.f, 22.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
				[
					MakeBadge(FText::AsNumber(Number), Color, 46.f)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 8.f)
				[
					MakeText(Title, 19, NFUI::Text, TEXT("Bold"))
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(STextBlock).Text(Body).Font(NFUI::Font(14)).ColorAndOpacity(NFUI::Muted).AutoWrapText(true).LineHeightPercentage(1.15f)
				]
			];
	}

	/** Case du tableau de vérité. */
	TSharedRef<SWidget> MakeCell(const FText& Text, const FLinearColor& Background, const FLinearColor& Foreground, FName Weight = TEXT("Bold"))
	{
		return SNew(SBox).WidthOverride(84.f).HeightOverride(48.f).Padding(3.f)
		[
			SNew(SBorder)
			.BorderImage(NFUI::PillBrush())
			.BorderBackgroundColor(Background)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				MakeText(Text, 18, Foreground, Weight)
			]
		];
	}
}

void SNFIntroWidget::Construct(const FArguments& InArgs)
{
	OnFinished = InArgs._OnFinished;
	CurrentPage = FMath::Clamp(InArgs._StartPage, 0, NumPages - 1);

	ChildSlot
	[
		SNew(SOverlay)

		// Fond flouté et assombri : le labo reste visible derrière
		+ SOverlay::Slot()
		[
			SNew(SBackgroundBlur)
			.BlurStrength(6.f)
			.Padding(0.f)
			[
				SNew(SBorder).BorderImage(NFUI::WhiteBrush()).BorderBackgroundColor(NFUI::Backdrop)
			]
		]

		// Panneau central
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(1120.f).HeightOverride(700.f)
			[
				SNew(SBorder)
				.BorderImage(NFUI::PanelBrush())
				.Padding(FMargin(48.f, 36.f))
				[
					SNew(SVerticalBox)

					// En-tête : logo + contexte
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							MakeBadge(FText::FromString(TEXT("NF")), NFUI::Accent, 44.f)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.f, 0.f, 0.f, 0.f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								MakeText(LOCTEXT("Logo", "NEURON FORGE"), 22, NFUI::Text, TEXT("Bold"))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								MakeText(LOCTEXT("Tagline", "Laboratoire de deep learning"), 12, NFUI::Muted)
							]
						]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SSpacer)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SBorder)
							.BorderImage(NFUI::PillBrush())
							.BorderBackgroundColor(NFUI::AccentSoft)
							.Padding(FMargin(14.f, 6.f))
							[
								MakeText(LOCTEXT("LevelTag", "NIVEAU 1  ·  XOR"), 12, NFUI::Accent, TEXT("Bold"))
							]
						]
					]

					// Liseré d'accent
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 26.f)
					[
						SNew(SBox).HeightOverride(2.f)
						[
							SNew(SBorder).BorderImage(NFUI::WhiteBrush()).BorderBackgroundColor(FLinearColor(0.1f, 0.78f, 1.f, 0.35f))
						]
					]

					// Contenu de la page (animé)
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SAssignNew(PageContainer, SBox)
						[
							BuildPage(CurrentPage)
						]
					]

					// Pied : points de pagination + navigation
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 24.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							BuildPageDots()
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(24.f, 0.f)
						[
							MakeText(LOCTEXT("KeysHint", "Entrée : suivant   ·   Échap : passer"), 12, NFUI::Muted)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
						[
							SNew(SButton)
							.Visibility_Lambda([this]() { return CurrentPage > 0 ? EVisibility::Visible : EVisibility::Hidden; })
							.ButtonStyle(NFUI::SecondaryButton())
							.ContentPadding(FMargin(22.f, 10.f))
							.OnClicked(this, &SNFIntroWidget::HandlePrevious)
							[
								MakeText(LOCTEXT("Previous", "PRÉCÉDENT"), 14, NFUI::Text, TEXT("Bold"))
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SButton)
							.ButtonStyle(NFUI::PrimaryButton())
							.ContentPadding(FMargin(28.f, 10.f))
							.OnClicked(this, &SNFIntroWidget::HandleNext)
							[
								SNew(STextBlock)
								.Font(NFUI::Font(14, TEXT("Bold")))
								.ColorAndOpacity(FLinearColor(0.01f, 0.03f, 0.06f))
								.Text_Lambda([this]() -> FText
								{
									return CurrentPage < NumPages - 1 ? LOCTEXT("Next", "SUIVANT") : LOCTEXT("Start", "COMMENCER LA MISSION");
								})
							]
						]
					]
				]
			]
		]
	];

	SetRenderOpacity(0.f);
}

TSharedRef<SWidget> SNFIntroWidget::BuildPageDots()
{
	TSharedRef<SHorizontalBox> Dots = SNew(SHorizontalBox);
	for (int32 i = 0; i < NumPages; ++i)
	{
		Dots->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
		[
			SNew(SBox)
			.WidthOverride_Lambda([this, i]() -> FOptionalSize { return i == CurrentPage ? 28.f : 10.f; })
			.HeightOverride(10.f)
			[
				SNew(SBorder)
				.BorderImage(NFUI::PillBrush())
				.BorderBackgroundColor_Lambda([this, i]() -> FSlateColor { return i == CurrentPage ? NFUI::Accent : FLinearColor(0.25f, 0.35f, 0.45f); })
			]
		];
	}
	return Dots;
}

TSharedRef<SWidget> SNFIntroWidget::BuildPage(int32 PageIndex)
{
	switch (PageIndex)
	{
	case 0:		return BuildWelcomePage();
	case 1:		return BuildXORPage();
	case 2:		return BuildMissionPage();
	default:	return BuildControlsPage();
	}
}

TSharedRef<SWidget> SNFIntroWidget::BuildWelcomePage()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.15f).Padding(0.f, 0.f, 40.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeEyebrow(LOCTEXT("WelcomeEyebrow", "BRIEFING DE MISSION"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 18.f)
			[
				MakeText(LOCTEXT("WelcomeTitle", "Bienvenue au laboratoire"), 34, NFUI::Text, TEXT("Bold"), true)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeParagraph(LOCTEXT("WelcomeBody1",
					"Vous êtes ingénieur·e en intelligence artificielle. La porte de sécurité du laboratoire est verrouillée : "
					"elle ne s'ouvrira que devant un modèle capable de résoudre le problème XOR sans la moindre erreur."), 17)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
			[
				MakeParagraph(LOCTEXT("WelcomeBody2",
					"Votre outil : l'holotable, au centre de la salle. Vous y assemblez un réseau de neurones, couche par couche, "
					"puis vous l'entraînez et le regardez apprendre en temps réel."), 17)
			]
		]
		+ SHorizontalBox::Slot().FillWidth(0.85f)
		[
			SNew(SBorder)
			.BorderImage(NFUI::AccentCardBrush())
			.Padding(FMargin(28.f, 26.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeEyebrow(LOCTEXT("ObjectiveEyebrow", "OBJECTIF"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 4.f)
				[
					MakeText(LOCTEXT("ObjectiveValue", "100 %"), 64, NFUI::Accent, TEXT("Bold"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeText(LOCTEXT("ObjectiveLabel", "de précision sur les 4 cas de XOR"), 16, NFUI::Text)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
				[
					SNew(SBox).HeightOverride(1.f)
					[
						SNew(SBorder).BorderImage(NFUI::WhiteBrush()).BorderBackgroundColor(NFUI::Outline)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
				[
					MakeSwatch(NFUI::Success, LOCTEXT("Reward1", "Récompense : la porte du labo s'ouvre"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					MakeSwatch(NFUI::Warning, LOCTEXT("Reward2", "Essais illimités : expérimentez !"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					MakeSwatch(NFUI::Accent, LOCTEXT("Reward3", "Durée : 5 à 10 minutes"))
				]
			]
		];
}

TSharedRef<SWidget> SNFIntroWidget::BuildXORPage()
{
	const FLinearColor Header(0.10f, 0.18f, 0.26f, 1.f);
	const FLinearColor Cell(0.06f, 0.10f, 0.15f, 1.f);
	const FLinearColor One(0.10f, 0.55f, 0.35f, 1.f);
	const FLinearColor Zero(0.20f, 0.12f, 0.14f, 1.f);

	TSharedRef<SGridPanel> Table = SNew(SGridPanel);
	Table->AddSlot(0, 0)[MakeCell(FText::FromString(TEXT("x1")), Header, NFUI::Accent)];
	Table->AddSlot(1, 0)[MakeCell(FText::FromString(TEXT("x2")), Header, NFUI::Accent)];
	Table->AddSlot(2, 0)[MakeCell(FText::FromString(TEXT("XOR")), Header, NFUI::Accent)];

	const int32 Rows[4][3] = { { 0, 0, 0 }, { 0, 1, 1 }, { 1, 0, 1 }, { 1, 1, 0 } };
	for (int32 r = 0; r < 4; ++r)
	{
		Table->AddSlot(0, r + 1)[MakeCell(FText::AsNumber(Rows[r][0]), Cell, NFUI::Text, TEXT("Regular"))];
		Table->AddSlot(1, r + 1)[MakeCell(FText::AsNumber(Rows[r][1]), Cell, NFUI::Text, TEXT("Regular"))];
		Table->AddSlot(2, r + 1)[MakeCell(FText::AsNumber(Rows[r][2]), Rows[r][2] ? One : Zero, NFUI::Text)];
	}

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 48.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(NFUI::CardBrush())
			.Padding(FMargin(20.f))
			[
				Table
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeEyebrow(LOCTEXT("XOREyebrow", "LE PROBLÈME"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 18.f)
			[
				MakeText(LOCTEXT("XORTitle", "Le OU exclusif (XOR)"), 36, NFUI::Text, TEXT("Bold"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeParagraph(LOCTEXT("XORBody1",
					"XOR vaut 1 quand ses deux entrées sont différentes, et 0 quand elles sont identiques. "
					"Le réseau reçoit x1 et x2 et doit prédire la bonne sortie pour les 4 cas."), 17)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
			[
				MakeParagraph(LOCTEXT("XORBody2",
					"Le piège : placez ces 4 points sur un plan, aucune droite ne sépare les 1 des 0. "
					"C'est ce problème qui a montré les limites du perceptron simple en 1969."), 17)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(NFUI::AccentCardBrush())
				.Padding(FMargin(18.f, 14.f))
				[
					MakeText(LOCTEXT("XORQuestion", "Question : de quoi votre réseau a-t-il besoin pour tracer une frontière courbe ?"),
						15, NFUI::Accent, TEXT("Bold"), true)
				]
			]
		];
}

TSharedRef<SWidget> SNFIntroWidget::BuildMissionPage()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeEyebrow(LOCTEXT("MissionEyebrow", "VOTRE MISSION EN 3 ÉTAPES"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 26.f)
		[
			MakeText(LOCTEXT("MissionTitle", "Concevoir, entraîner, valider"), 36, NFUI::Text, TEXT("Bold"))
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 14.f, 0.f)
			[
				MakeStepCard(1, LOCTEXT("Step1Title", "Concevoir"),
					LOCTEXT("Step1Body", "Sur la console de droite, visez un bouton et appuyez sur E : ajoutez des couches cachées, "
						"réglez le nombre de neurones, choisissez la fonction d'activation et le taux d'apprentissage."), NFUI::Accent)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(7.f, 0.f)
			[
				MakeStepCard(2, LOCTEXT("Step2Title", "Entraîner"),
					LOCTEXT("Step2Body", "Marchez sur le disque lumineux « ENTRAÎNER » devant l'holotable. Observez la perte diminuer, "
						"les poids changer de couleur et les signaux circuler dans le réseau."), NFUI::Warning)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(14.f, 0.f, 0.f, 0.f)
			[
				MakeStepCard(3, LOCTEXT("Step3Title", "Valider"),
					LOCTEXT("Step3Body", "À 100 % de précision, le système de sécurité valide votre modèle et la porte s'ouvre. "
						"Échec ? Lisez le conseil affiché sur l'écran d'état, modifiez le réseau et réessayez."), NFUI::Success)
			]
		];
}

TSharedRef<SWidget> SNFIntroWidget::BuildControlsPage()
{
	auto KeyRow = [](const FText& Key, const FText& Action)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(150.f).HAlign(HAlign_Left)
				[
					MakeKey(Key)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				MakeText(Action, 15, NFUI::Text)
			];
	};

	TSharedRef<SVerticalBox> Keys = SNew(SVerticalBox);
	const TPair<FText, FText> Bindings[] = {
		{ LOCTEXT("KeyMove", "Z Q S D"), LOCTEXT("ActMove", "Se déplacer") },
		{ LOCTEXT("KeyLook", "Souris"), LOCTEXT("ActLook", "Regarder") },
		{ LOCTEXT("KeyUse", "E  /  Clic"), LOCTEXT("ActUse", "Utiliser le bouton visé") },
		{ LOCTEXT("KeyJump", "Espace"), LOCTEXT("ActJump", "Sauter") },
		{ LOCTEXT("KeyHelp", "H"), LOCTEXT("ActHelp", "Revoir ce briefing") },
	};
	for (const TPair<FText, FText>& Binding : Bindings)
	{
		Keys->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)[KeyRow(Binding.Key, Binding.Value)];
	}

	TSharedRef<SVerticalBox> Legend = SNew(SVerticalBox);
	Legend->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)[MakeEyebrow(LOCTEXT("LegendNeurons", "NEURONES : COULEUR = ACTIVATION"))];
	const ENFActivation Activations[] = { ENFActivation::Linear, ENFActivation::ReLU, ENFActivation::Sigmoid, ENFActivation::Tanh };
	TSharedRef<SHorizontalBox> ActivationRow = SNew(SHorizontalBox);
	for (ENFActivation Activation : Activations)
	{
		ActivationRow->AddSlot().AutoWidth().Padding(0.f, 0.f, 22.f, 0.f)
		[
			MakeSwatch(NFActivation::GetColor(Activation), FText::FromString(NFActivation::ToString(Activation)))
		];
	}
	Legend->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[ActivationRow];
	Legend->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 22.f)
	[
		MakeText(LOCTEXT("LegendGlow", "Plus un neurone brille, plus sa valeur est forte."), 13, NFUI::Muted)
	];
	Legend->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)[MakeEyebrow(LOCTEXT("LegendWeights", "POIDS : COULEUR = SIGNE"))];
	Legend->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 22.f, 0.f)[MakeSwatch(FLinearColor(0.1f, 0.55f, 1.f), LOCTEXT("Positive", "Positif"))]
		+ SHorizontalBox::Slot().AutoWidth()[MakeSwatch(FLinearColor(1.f, 0.35f, 0.1f), LOCTEXT("Negative", "Négatif"))]
	];
	Legend->AddSlot().AutoHeight()
	[
		MakeText(LOCTEXT("LegendThickness", "Plus un lien est épais, plus le poids est important."), 13, NFUI::Muted)
	];

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeEyebrow(LOCTEXT("ControlsEyebrow", "AVANT DE COMMENCER"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 26.f)
		[
			MakeText(LOCTEXT("ControlsTitle", "Commandes et lecture de l'holotable"), 36, NFUI::Text, TEXT("Bold"))
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 14.f, 0.f)
			[
				SNew(SBorder).BorderImage(NFUI::CardBrush()).Padding(FMargin(24.f, 22.f))
				[
					Keys
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.2f).Padding(14.f, 0.f, 0.f, 0.f)
			[
				SNew(SBorder).BorderImage(NFUI::CardBrush()).Padding(FMargin(24.f, 22.f))
				[
					Legend
				]
			]
		];
}

void SNFIntroWidget::GoToPage(int32 PageIndex)
{
	const int32 NewPage = FMath::Clamp(PageIndex, 0, NumPages - 1);
	if (NewPage == CurrentPage || !PageContainer.IsValid())
	{
		return;
	}
	CurrentPage = NewPage;
	PageAge = 0.f;
	PageContainer->SetContent(BuildPage(CurrentPage));
}

FReply SNFIntroWidget::HandleNext()
{
	if (CurrentPage < NumPages - 1)
	{
		GoToPage(CurrentPage + 1);
	}
	else
	{
		Finish();
	}
	return FReply::Handled();
}

FReply SNFIntroWidget::HandlePrevious()
{
	GoToPage(CurrentPage - 1);
	return FReply::Handled();
}

void SNFIntroWidget::Finish()
{
	if (!bFinishing)
	{
		bFinishing = true;
		FinishAge = 0.f;
	}
}

FReply SNFIntroWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		return HandleNext();
	}
	if (Key == EKeys::Left || Key == EKeys::Q || Key == EKeys::A)
	{
		return HandlePrevious();
	}
	if (Key == EKeys::Escape || Key == EKeys::H || Key == EKeys::Gamepad_FaceButton_Right)
	{
		Finish();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SNFIntroWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	Age += InDeltaTime;
	PageAge += InDeltaTime;

	// Fondu d'entrée, puis fondu de sortie
	float Opacity = NFUI::EaseOut(Age / 0.45f);
	if (bFinishing)
	{
		FinishAge += InDeltaTime;
		Opacity = 1.f - NFUI::EaseOut(FinishAge / 0.3f);
		if (FinishAge >= 0.3f && !bNotified)
		{
			bNotified = true;	// une seule notification
			SetRenderOpacity(0.f);
			OnFinished.ExecuteIfBound();
			return;
		}
	}
	SetRenderOpacity(Opacity);

	// Chaque nouvelle page glisse légèrement depuis la droite
	if (PageContainer.IsValid())
	{
		const float PageAlpha = NFUI::EaseOut(PageAge / 0.35f);
		PageContainer->SetRenderOpacity(PageAlpha);
		PageContainer->SetRenderTransform(FSlateRenderTransform(FVector2f(40.f * (1.f - PageAlpha), 0.f)));
	}
}

#undef LOCTEXT_NAMESPACE
