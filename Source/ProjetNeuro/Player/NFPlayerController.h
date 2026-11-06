// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NFPlayerController.generated.h"

class ANFHUD;
class SNFEndScreenWidget;
class SNFIntroWidget;
class SNFObjectivesWidget;
class SWidget;

/**
 * Contrôleur du joueur : caméra du template, messages du HUD et interface plein écran.
 *
 *  - Briefing d'introduction au lancement (jeu en pause), rappelé avec H / F1.
 *  - Panneau d'objectifs permanent pendant la partie.
 *  - Écran « Mission accomplie » à la sortie du labo (Rejouer / Quitter).
 *
 * Lancer le jeu avec -NFNoIntro pour sauter le briefing (tests automatisés).
 */
UCLASS()
class PROJETNEURO_API ANFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANFPlayerController();

	/** Affiche un message temporaire en haut de l'écran. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|UI")
	void ShowMessage(const FString& Text, FLinearColor Color = FLinearColor::White, float Duration = 3.f);

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|UI")
	ANFHUD* GetNFHUD() const;

	/** Ouvre le briefing (jeu en pause) à la page donnée. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|UI")
	void ShowIntro(int32 StartPage = 0);

	/** Ferme immédiatement le briefing et l'écran de fin, et reprend le jeu. */
	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|UI")
	void CloseOverlays();

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|UI")
	bool IsIntroOpen() const { return IntroWidget.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Neuron Forge|UI")
	bool IsEndScreenOpen() const { return EndScreenWidget.IsValid(); }

	/** Appelé par la zone de sortie : affiche le bilan de mission après un court délai. */
	void HandleLevelCompleted();

	/** Affiche le briefing au lancement du niveau. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|UI")
	bool bShowIntroOnStart = true;

	/** Délai entre l'entrée dans la zone de sortie et l'écran de fin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|UI")
	float EndScreenDelay = 1.5f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;

	void ToggleIntro();
	void HandleIntroFinished();
	void ShowEndScreen();
	void Replay();
	void Quit();

	/** Bascule l'entrée vers l'interface (curseur, pause) ou vers le jeu. */
	void SetUIMode(TSharedPtr<SWidget> FocusWidget);
	void SetGameMode();

	void AddOverlay(const TSharedRef<SWidget>& Widget, int32 ZOrder);
	void RemoveOverlay(TSharedPtr<SWidget>& Widget);

private:
	TSharedPtr<SNFIntroWidget> IntroWidget;
	TSharedPtr<SNFEndScreenWidget> EndScreenWidget;
	TSharedPtr<SNFObjectivesWidget> ObjectivesWidget;

	/** Temps de jeu (hors pause) au début de la mission, pour la durée affichée en fin de niveau. */
	float MissionStartTime = 0.f;
	FTimerHandle EndScreenTimer;
};
