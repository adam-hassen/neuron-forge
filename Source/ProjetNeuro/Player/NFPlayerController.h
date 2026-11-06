// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NFPlayerController.generated.h"

class ANFHUD;

/** Contrôleur du joueur : caméra du template et accès au HUD pour les messages. */
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

protected:
	virtual void BeginPlay() override;
};
