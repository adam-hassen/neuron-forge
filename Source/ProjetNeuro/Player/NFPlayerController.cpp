// Neuron Forge - ENSIM

#include "Player/NFPlayerController.h"
#include "Player/NFHUD.h"
#include "Player/NFPlayerCameraManager.h"

ANFPlayerController::ANFPlayerController()
{
	PlayerCameraManagerClass = ANFPlayerCameraManager::StaticClass();
}

void ANFPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Jeu à la souris : pas de curseur, toutes les entrées vont au jeu
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

ANFHUD* ANFPlayerController::GetNFHUD() const
{
	return Cast<ANFHUD>(GetHUD());
}

void ANFPlayerController::ShowMessage(const FString& Text, FLinearColor Color, float Duration)
{
	if (ANFHUD* NFHUD = GetNFHUD())
	{
		NFHUD->ShowMessage(Text, Color, Duration);
	}
}
