// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

/**
 * Charte graphique de l'interface Neuron Forge : couleurs, polices, formes arrondies
 * et petits widgets réutilisables (titres, paragraphes, pastilles numérotées...).
 */
namespace NFUI
{
	// --- Couleurs ---
	inline const FLinearColor Accent(0.10f, 0.78f, 1.00f);			// cyan
	inline const FLinearColor AccentSoft(0.10f, 0.78f, 1.00f, 0.18f);
	inline const FLinearColor Success(0.20f, 1.00f, 0.50f);
	inline const FLinearColor Warning(1.00f, 0.72f, 0.20f);
	inline const FLinearColor Danger(1.00f, 0.32f, 0.25f);
	inline const FLinearColor Text(0.88f, 0.94f, 1.00f);
	inline const FLinearColor Muted(0.55f, 0.65f, 0.76f);
	inline const FLinearColor Backdrop(0.00f, 0.01f, 0.03f, 0.55f);
	inline const FLinearColor Panel(0.02f, 0.04f, 0.07f, 0.94f);
	inline const FLinearColor Card(0.05f, 0.09f, 0.14f, 0.95f);
	inline const FLinearColor Outline(0.20f, 0.55f, 0.80f, 0.45f);

	// --- Polices (Roboto, fournie par le moteur) ---
	inline FSlateFontInfo Font(int32 Size, const FName Weight = TEXT("Regular"))
	{
		return FCoreStyle::GetDefaultFontStyle(Weight, Size);
	}

	// --- Pinceaux arrondis (durée de vie statique, requise par Slate) ---
	inline const FSlateBrush* PanelBrush()
	{
		static const FSlateRoundedBoxBrush Brush(Panel, 18.f, Outline, 1.5f);
		return &Brush;
	}
	inline const FSlateBrush* CardBrush()
	{
		static const FSlateRoundedBoxBrush Brush(Card, 12.f, FLinearColor(0.2f, 0.45f, 0.65f, 0.35f), 1.f);
		return &Brush;
	}
	inline const FSlateBrush* AccentCardBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor(0.04f, 0.14f, 0.22f, 0.95f), 12.f, Accent, 1.5f);
		return &Brush;
	}
	/** Cercle (ou pilule) : rayon égal à la moitié de la hauteur, quelle que soit la taille. */
	inline const FSlateBrush* CircleBrush()
	{
		static const FSlateRoundedBoxBrush Brush = []()
		{
			FSlateRoundedBoxBrush Result(FLinearColor::White, 0.f);
			Result.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
			return Result;
		}();
		return &Brush;
	}

	inline FButtonStyle MakeButtonStyle(const FLinearColor& Normal, const FLinearColor& Hovered, const FLinearColor& Pressed, const FLinearColor& Border)
	{
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("Button"));
		Style.SetNormal(FSlateRoundedBoxBrush(Normal, 8.f, Border, 1.f));
		Style.SetHovered(FSlateRoundedBoxBrush(Hovered, 8.f, Border, 1.f));
		Style.SetPressed(FSlateRoundedBoxBrush(Pressed, 8.f, Border, 1.f));
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f, 1.f, 0.f, 0.f));
		return Style;
	}

	/** Bouton principal : fond cyan, texte sombre. */
	inline const FButtonStyle* PrimaryButton()
	{
		static const FButtonStyle Style = MakeButtonStyle(Accent, FLinearColor(0.50f, 0.92f, 1.f), FLinearColor(0.05f, 0.55f, 0.75f), Accent);
		return &Style;
	}

	/** Bouton secondaire : fond bleu nuit, texte clair. */
	inline const FButtonStyle* SecondaryButton()
	{
		static const FButtonStyle Style = MakeButtonStyle(FLinearColor(0.10f, 0.17f, 0.25f), FLinearColor(0.16f, 0.27f, 0.38f),
			FLinearColor(0.07f, 0.12f, 0.18f), FLinearColor(0.3f, 0.5f, 0.7f, 0.6f));
		return &Style;
	}
	inline const FSlateBrush* PillBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White, 8.f);
		return &Brush;
	}
	inline const FSlateBrush* WhiteBrush()
	{
		return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	}

	/** Interpolation douce entre 0 et 1. */
	inline float EaseOut(float Alpha)
	{
		const float A = FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - FMath::Pow(1.f - A, 3.f);
	}
}
