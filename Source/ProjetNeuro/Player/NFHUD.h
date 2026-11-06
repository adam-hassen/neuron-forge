// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NFHUD.generated.h"

/**
 * HUD minimal dessiné au Canvas :
 *  - réticule au centre, qui grossit et change de couleur sur un objet interactif ;
 *  - texte d'aide sous le réticule (« [E] Ajouter un neurone ») ;
 *  - messages temporaires en haut de l'écran.
 *
 * Les informations de jeu (état de l'entraînement...) sont affichées dans le monde
 * avec des Text Render, ce qui fonctionne aussi en VR.
 */
UCLASS()
class PROJETNEURO_API ANFHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UFUNCTION(BlueprintCallable, Category = "Neuron Forge|UI")
	void ShowMessage(const FString& Text, FLinearColor Color = FLinearColor::White, float Duration = 3.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|UI")
	FLinearColor IdleCrosshairColor = FLinearColor(1.f, 1.f, 1.f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|UI")
	FLinearColor FocusCrosshairColor = FLinearColor(0.1f, 0.9f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neuron Forge|UI")
	FLinearColor DisabledColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.9f);

private:
	struct FMessage
	{
		FString Text;
		FLinearColor Color;
		float TimeRemaining = 0.f;
	};

	void DrawCrosshairAndPrompt();
	void DrawMessages(float DeltaSeconds);
	void DrawCenteredText(const FString& Text, const FLinearColor& Color, float CenterX, float Y, float Scale);

	TArray<FMessage> Messages;
};
