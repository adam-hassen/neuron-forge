// Neuron Forge - ENSIM

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "NFPlayerCameraManager.generated.h"

/** Limites de tangage de la caméra, identiques au template First Person (-70° / +80°). */
UCLASS()
class PROJETNEURO_API ANFPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	ANFPlayerCameraManager()
	{
		ViewPitchMin = -70.f;
		ViewPitchMax = 80.f;
	}
};
