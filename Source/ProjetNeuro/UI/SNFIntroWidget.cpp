// Neuron Forge - ENSIM

#include "UI/SNFIntroWidget.h"
#include "NeuralNetwork/NFActivation.h"
#include "NeuralNetwork/NFDataset.h"
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
								MakeText(LOCTEXT("LevelTag", "NIVEAU 1  ·  LA SERRE"), 12, NFUI::Accent, TEXT("Bold"))
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
	case 1:		return BuildProblemPage();
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
					"Vous êtes ingénieur·e en intelligence artificielle. La serre du labo a besoin d'aide : il faut un modèle "
					"capable de prédire si une plante va survivre selon l'eau et la lumière qu'elle reçoit. La porte de "
					"sécurité ne s'ouvrira que devant un modèle assez fiable."), 17)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
			[
				MakeParagraph(LOCTEXT("WelcomeBody2",
					"Votre outil : la Forge. Vous y assemblez de vos mains un réseau de neurones, pièce par pièce, et l'hologramme "
					"géant le reconstruit en direct. Puis vous l'entraînez et le regardez apprendre en temps réel."), 17)
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
					MakeText(LOCTEXT("ObjectiveValue", "95 %"), 64, NFUI::Accent, TEXT("Bold"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeText(LOCTEXT("ObjectiveLabel", "de bonnes prédictions sur les 160 plantes"), 16, NFUI::Text)
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
					MakeSwatch(NFUI::Success, LOCTEXT("Reward1", "Récompense : la porte de la serre s'ouvre"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					MakeSwatch(NFUI::Warning, LOCTEXT("Reward2", "Ensuite : votre modèle fait pousser les 36 plantes de la serre"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					MakeSwatch(NFUI::Accent, LOCTEXT("Reward3", "Durée : 5 à 10 minutes"))
				]
			]
		];
}

TSharedRef<SWidget> SNFIntroWidget::BuildProblemPage()
{
	// Mini-carte de la serre : chaque case = un réglage eau / lumière, verte si la plante survit
	const FLinearColor Alive(0.12f, 0.62f, 0.30f, 1.f);
	const FLinearColor Dead(0.42f, 0.13f, 0.12f, 1.f);
	TSharedRef<SGridPanel> Map = SNew(SGridPanel);
	for (int32 Light = 10; Light >= 0; --Light)
	{
		const int32 Row = 10 - Light;
		Map->AddSlot(0, Row)
		[
			SNew(SBox).WidthOverride(26.f).HeightOverride(20.f).HAlign(HAlign_Right).VAlign(VAlign_Center)
			[
				MakeText(FText::AsNumber(Light), 9, NFUI::Muted)
			]
		];
		for (int32 Water = 0; Water <= 10; ++Water)
		{
			Map->AddSlot(Water + 1, Row)
			[
				SNew(SBox).WidthOverride(20.f).HeightOverride(20.f).Padding(1.f)
				[
					SNew(SBorder)
					.BorderImage(NFUI::PillBrush())
					.BorderBackgroundColor(FNFDataset::GreenhouseSurvives(Water, Light) ? Alive : Dead)
				]
			];
		}
	}
	for (int32 Water = 0; Water <= 10; Water += 5)
	{
		Map->AddSlot(Water + 1, 11)[SNew(SBox).HAlign(HAlign_Center)[MakeText(FText::AsNumber(Water), 9, NFUI::Muted)]];
	}

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 40.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(NFUI::CardBrush())
			.Padding(FMargin(16.f, 14.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
				[
					MakeText(LOCTEXT("MapTitle", "Lumière (haut) / Eau (droite)"), 12, NFUI::Muted, TEXT("Bold"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					Map
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 16.f, 0.f)[MakeSwatch(Alive, LOCTEXT("MapAlive", "survit"))]
					+ SHorizontalBox::Slot().AutoWidth()[MakeSwatch(Dead, LOCTEXT("MapDead", "meurt"))]
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeEyebrow(LOCTEXT("ProblemEyebrow", "LE PROBLÈME"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 18.f)
			[
				MakeText(LOCTEXT("ProblemTitle", "La serre du labo"), 36, NFUI::Text, TEXT("Bold"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeParagraph(LOCTEXT("ProblemBody1",
					"Le labo a fait pousser 160 plantes en changeant deux réglages : la quantité d'eau et la quantité "
					"de lumière (de 0 à 10). Pour chacune, on a noté si elle a survécu ou si elle est morte."), 17)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
			[
				MakeParagraph(LOCTEXT("ProblemBody2",
					"Une plante ne survit que si elle n'a ni trop ni trop peu d'eau et de lumière : la zone de survie est "
					"un îlot au milieu. Votre modèle doit apprendre, à partir de ces exemples, à prédire si une plante survivra."), 17)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(NFUI::AccentCardBrush())
				.Padding(FMargin(18.f, 14.f))
				[
					MakeText(LOCTEXT("ProblemQuestion", "Question : une simple ligne droite peut-elle entourer un îlot ?"),
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
			MakeText(LOCTEXT("MissionTitle", "Concevoir, entraîner, tester"), 36, NFUI::Text, TEXT("Bold"))
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 14.f, 0.f)
			[
				MakeStepCard(1, LOCTEXT("Step1Title", "Concevoir"),
					LOCTEXT("Step1Body", "Dans l'atelier, prenez les pièces avec E et assemblez-les sur la Forge : un module = une couche "
						"cachée, une orbe posée autour = un neurone, un cristal planté dessus = l'activation. Le levier règle "
						"le taux d'apprentissage."), NFUI::Accent)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(7.f, 0.f)
			[
				MakeStepCard(2, LOCTEXT("Step2Title", "Entraîner"),
					LOCTEXT("Step2Body", "Marchez sur le disque lumineux « ENTRAÎNER ». Sur le carnet du labo, regardez la zone "
						"verte se dessiner : c'est là où le modèle pense que la plante survit. Un bon modèle réussit en 10 à 30 secondes."), NFUI::Warning)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(14.f, 0.f, 0.f, 0.f)
			[
				MakeStepCard(3, LOCTEXT("Step3Title", "Valider et tester"),
					LOCTEXT("Step3Body", "À 95 % de bonnes prédictions, la porte s'ouvre. Testez votre modèle sur le pot de plante, puis "
						"entrez dans la serre : il décide quelles plantes poussent. Échec ? Lisez le conseil et réessayez."), NFUI::Success)
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
		{ LOCTEXT("KeyUse", "E  /  Clic"), LOCTEXT("ActUse", "Prendre, poser, insérer, tirer le levier") },
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
