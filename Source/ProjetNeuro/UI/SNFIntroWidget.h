// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SBox;

/**
 * Écran d'introduction affiché au lancement (et rappelé avec la touche H).
 *
 * Quatre pages animées : bienvenue / le problème XOR / la mission / commandes et légende.
 * Navigation : boutons, flèches, Entrée ou Espace ; Échap pour passer.
 */
class PROJETNEURO_API SNFIntroWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNFIntroWidget) {}
		/** Appelé quand le joueur ferme l'introduction (après le fondu de sortie). */
		SLATE_EVENT(FSimpleDelegate, OnFinished)
		/** Page affichée à l'ouverture. */
		SLATE_ARGUMENT(int32, StartPage)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	void GoToPage(int32 PageIndex);
	int32 GetCurrentPage() const { return CurrentPage; }

	/** Lance le fondu de sortie puis appelle OnFinished. */
	void Finish();

	bool IsFinishing() const { return bFinishing; }

	static constexpr int32 NumPages = 4;

private:
	TSharedRef<SWidget> BuildPage(int32 PageIndex);
	TSharedRef<SWidget> BuildWelcomePage();
	TSharedRef<SWidget> BuildXORPage();
	TSharedRef<SWidget> BuildMissionPage();
	TSharedRef<SWidget> BuildControlsPage();
	TSharedRef<SWidget> BuildPageDots();

	FReply HandleNext();
	FReply HandlePrevious();

	FSimpleDelegate OnFinished;
	TSharedPtr<SBox> PageContainer;
	int32 CurrentPage = 0;
	float Age = 0.f;
	float PageAge = 0.f;
	float FinishAge = 0.f;
	bool bFinishing = false;
	bool bNotified = false;
};
