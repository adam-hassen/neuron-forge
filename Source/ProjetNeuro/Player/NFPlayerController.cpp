// Neuron Forge - ENSIM

#include "Player/NFPlayerController.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Lab/NFExitTrigger.h"
#include "Lab/NFNetworkBuilder.h"
#include "Misc/CommandLine.h"
#include "Player/NFHUD.h"
#include "Player/NFPlayerCameraManager.h"
#include "TimerManager.h"
#include "UI/SNFEndScreenWidget.h"
#include "UI/SNFIntroWidget.h"

namespace
{
	template <typename ActorType>
	ActorType* FindFirstActor(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		TActorIterator<ActorType> It(World);
		return It ? *It : nullptr;
	}

	// Ordre d'affichage des interfaces dans le viewport
	constexpr int32 ObjectivesZOrder = 10;
	constexpr int32 IntroZOrder = 100;
	constexpr int32 EndScreenZOrder = 110;
}

ANFPlayerController::ANFPlayerController()
{
	PlayerCameraManagerClass = ANFPlayerCameraManager::StaticClass();
}

void ANFPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetGameMode();

	if (!IsLocalController())
	{
		return;
	}

	// Panneau d'objectifs permanent
	ObjectivesWidget = SNew(SNFObjectivesWidget)
		.Builder(FindFirstActor<ANFNetworkBuilder>(GetWorld()))
		.Exit(FindFirstActor<ANFExitTrigger>(GetWorld()));
	AddOverlay(ObjectivesWidget.ToSharedRef(), ObjectivesZOrder);

	MissionStartTime = GetWorld()->GetTimeSeconds();

	if (bShowIntroOnStart && !FParse::Param(FCommandLine::Get(), TEXT("NFNoIntro")))
	{
		ShowIntro(0);
	}
}

void ANFPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TSharedPtr<SWidget> Intro = IntroWidget;
	TSharedPtr<SWidget> End = EndScreenWidget;
	TSharedPtr<SWidget> Objectives = ObjectivesWidget;
	RemoveOverlay(Intro);
	RemoveOverlay(End);
	RemoveOverlay(Objectives);
	IntroWidget.Reset();
	EndScreenWidget.Reset();
	ObjectivesWidget.Reset();

	Super::EndPlay(EndPlayReason);
}

void ANFPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Touches d'interface : fonctionnent aussi pendant la pause
	FInputKeyBinding& Help = InputComponent->BindKey(EKeys::H, IE_Pressed, this, &ANFPlayerController::ToggleIntro);
	Help.bExecuteWhenPaused = true;
	FInputKeyBinding& HelpF1 = InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &ANFPlayerController::ToggleIntro);
	HelpF1.bExecuteWhenPaused = true;
}

// ---------------------------------------------------------------------------
// Overlays

void ANFPlayerController::AddOverlay(const TSharedRef<SWidget>& Widget, int32 ZOrder)
{
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->AddViewportWidgetContent(Widget, ZOrder);
	}
}

void ANFPlayerController::RemoveOverlay(TSharedPtr<SWidget>& Widget)
{
	if (Widget.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(Widget.ToSharedRef());
		}
		Widget.Reset();
	}
}

void ANFPlayerController::SetUIMode(TSharedPtr<SWidget> FocusWidget)
{
	FInputModeUIOnly Mode;
	if (FocusWidget.IsValid())
	{
		Mode.SetWidgetToFocus(FocusWidget);
	}
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	bShowMouseCursor = true;
	SetPause(true);

	if (FocusWidget.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetKeyboardFocus(FocusWidget);
	}
}

void ANFPlayerController::SetGameMode()
{
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	SetPause(false);
}

void ANFPlayerController::ShowIntro(int32 StartPage)
{
	if (IntroWidget.IsValid() || EndScreenWidget.IsValid() || !IsLocalController())
	{
		return;
	}

	IntroWidget = SNew(SNFIntroWidget)
		.StartPage(StartPage)
		.OnFinished(FSimpleDelegate::CreateUObject(this, &ANFPlayerController::HandleIntroFinished));
	AddOverlay(IntroWidget.ToSharedRef(), IntroZOrder);
	SetUIMode(IntroWidget);
}

void ANFPlayerController::ToggleIntro()
{
	if (IntroWidget.IsValid())
	{
		IntroWidget->Finish();
	}
	else
	{
		ShowIntro(0);
	}
}

void ANFPlayerController::HandleIntroFinished()
{
	TSharedPtr<SWidget> Widget = IntroWidget;
	RemoveOverlay(Widget);
	IntroWidget.Reset();
	SetGameMode();
	ShowMessage(TEXT("Mission : entraînez un réseau qui résout XOR"), FLinearColor(0.3f, 0.85f, 1.f), 4.f);
}

void ANFPlayerController::CloseOverlays()
{
	TSharedPtr<SWidget> Intro = IntroWidget;
	TSharedPtr<SWidget> End = EndScreenWidget;
	RemoveOverlay(Intro);
	RemoveOverlay(End);
	IntroWidget.Reset();
	EndScreenWidget.Reset();
	GetWorldTimerManager().ClearTimer(EndScreenTimer);
	SetGameMode();
}

// ---------------------------------------------------------------------------
// Fin de niveau

void ANFPlayerController::HandleLevelCompleted()
{
	ShowMessage(TEXT("Niveau terminé !"), FLinearColor(0.3f, 1.f, 0.45f), 3.f);
	GetWorldTimerManager().SetTimer(EndScreenTimer, this, &ANFPlayerController::ShowEndScreen, EndScreenDelay, false);
}

void ANFPlayerController::ShowEndScreen()
{
	if (EndScreenWidget.IsValid() || !IsLocalController())
	{
		return;
	}
	if (IntroWidget.IsValid())
	{
		TSharedPtr<SWidget> Intro = IntroWidget;
		RemoveOverlay(Intro);
		IntroWidget.Reset();
	}

	FNFMissionSummary Summary;
	Summary.Seconds = GetWorld()->GetTimeSeconds() - MissionStartTime;
	if (const ANFNetworkBuilder* Builder = FindFirstActor<ANFNetworkBuilder>(GetWorld()))
	{
		Summary.Architecture = Builder->HasSucceeded() ? Builder->GetSuccessArchitecture() : Builder->DescribeNetwork();
		Summary.LearningRate = Builder->GetSuccessLearningRate();
		Summary.Epochs = Builder->GetSuccessProgress().Epoch;
		Summary.FinalLoss = Builder->GetSuccessProgress().Loss;
		Summary.Attempts = Builder->GetTrainingAttempts();
	}

	EndScreenWidget = SNew(SNFEndScreenWidget)
		.Summary(Summary)
		.OnReplay(FSimpleDelegate::CreateUObject(this, &ANFPlayerController::Replay))
		.OnQuit(FSimpleDelegate::CreateUObject(this, &ANFPlayerController::Quit));
	AddOverlay(EndScreenWidget.ToSharedRef(), EndScreenZOrder);
	SetUIMode(EndScreenWidget);
}

void ANFPlayerController::Replay()
{
	CloseOverlays();
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void ANFPlayerController::Quit()
{
	CloseOverlays();
	UKismetSystemLibrary::QuitGame(GetWorld(), this, EQuitPreference::Quit, false);
}

// ---------------------------------------------------------------------------

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
