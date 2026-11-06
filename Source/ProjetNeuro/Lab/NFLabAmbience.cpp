// Neuron Forge - ENSIM

#include "Lab/NFLabAmbience.h"
#include "Components/LightComponent.h"
#include "Engine/Light.h"
#include "EngineUtils.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Lab/NFNetworkBuilder.h"
#include "Materials/MaterialParameterCollection.h"

namespace
{
	const FName MoodColorParam(TEXT("MoodColor"));
	const FName MoodIntensityParam(TEXT("MoodIntensity"));
}

ANFLabAmbience::ANFLabAmbience()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	CurrentColor = IdleColor;
}

void ANFLabAmbience::BeginPlay()
{
	Super::BeginPlay();

	if (!Builder)
	{
		for (TActorIterator<ANFNetworkBuilder> It(GetWorld()); It; ++It)
		{
			Builder = *It;
			break;
		}
	}
	if (Builder && Builder->GetTrainer())
	{
		Builder->GetTrainer()->OnStateChanged.AddUniqueDynamic(this, &ANFLabAmbience::HandleTrainingStateChanged);
		State = Builder->GetTrainingState();
	}

	// Lumières pilotées : on retient leur réglage d'origine
	for (TActorIterator<ALight> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(LightTag) && It->GetLightComponent())
		{
			ULightComponent* Light = It->GetLightComponent();
			Light->SetMobility(EComponentMobility::Movable);
			Lights.Add({ Light, Light->GetLightColor(), Light->Intensity });
		}
	}

	CurrentColor = IdleColor;
	ApplyMood(CurrentColor, 1.f);
}

void ANFLabAmbience::HandleTrainingStateChanged(ENFTrainingState NewState)
{
	State = NewState;
	if (NewState == ENFTrainingState::Succeeded || NewState == ENFTrainingState::Failed)
	{
		Flash = 1.f;
	}
	if (NewState == ENFTrainingState::Failed)
	{
		FailureTimer = 2.5f;
	}
}

void ANFLabAmbience::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Time = GetWorld()->GetTimeSeconds();
	FailureTimer = FMath::Max(0.f, FailureTimer - DeltaSeconds);
	Flash = FMath::Max(0.f, Flash - DeltaSeconds * 1.2f);

	FLinearColor Target = IdleColor;
	float Intensity = 1.f;
	switch (State)
	{
	case ENFTrainingState::Training:
		Target = TrainingColor;
		Intensity = 0.8f + 0.35f * FMath::Sin(Time * 6.f);		// pulsation pendant l'apprentissage
		break;
	case ENFTrainingState::Succeeded:
		Target = SuccessColor;
		Intensity = 1.f + 0.1f * FMath::Sin(Time * 1.5f);
		break;
	case ENFTrainingState::Failed:
		// Alerte rouge clignotante, puis retour au calme
		Target = FailureTimer > 0.f ? FailureColor : IdleColor;
		Intensity = FailureTimer > 0.f ? 0.7f + 0.5f * FMath::Abs(FMath::Sin(Time * 8.f)) : 1.f;
		break;
	default:
		Intensity = 1.f + 0.06f * FMath::Sin(Time * 1.2f);	// respiration lente au repos
		break;
	}

	CurrentColor = FMath::Lerp(CurrentColor, Target, FMath::Clamp(DeltaSeconds * BlendSpeed, 0.f, 1.f));
	ApplyMood(CurrentColor, Intensity + 2.f * Flash);
}

void ANFLabAmbience::ApplyMood(const FLinearColor& Color, float Intensity)
{
	if (MoodCollection)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(this, MoodCollection, MoodColorParam, Color);
		UKismetMaterialLibrary::SetScalarParameterValue(this, MoodCollection, MoodIntensityParam, Intensity);
	}

	const float LightIntensity = FMath::Lerp(1.f, Intensity, 0.4f);
	for (const FLightState& LightState : Lights)
	{
		if (ULightComponent* Light = LightState.Light.Get())
		{
			Light->SetLightColor(FMath::Lerp(LightState.BaseColor, Color, LightTint));
			Light->SetIntensity(LightState.BaseIntensity * LightIntensity);
		}
	}
}
