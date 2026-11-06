// Neuron Forge - ENSIM

#include "Core/NFGameMode.h"
#include "Player/NFCharacter.h"
#include "Player/NFHUD.h"
#include "Player/NFPlayerController.h"

ANFGameMode::ANFGameMode()
{
	DefaultPawnClass = ANFCharacter::StaticClass();
	PlayerControllerClass = ANFPlayerController::StaticClass();
	HUDClass = ANFHUD::StaticClass();
}
