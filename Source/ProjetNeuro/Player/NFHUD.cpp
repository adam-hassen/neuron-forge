// Neuron Forge - ENSIM

#include "Player/NFHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/Pawn.h"
#include "Interaction/NFInteractable.h"
#include "Interaction/NFGrabberComponent.h"
#include "Interaction/NFInteractorComponent.h"

void ANFHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	DrawCrosshairAndPrompt();
	DrawMessages(GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f);
}

void ANFHUD::ShowMessage(const FString& Text, FLinearColor Color, float Duration)
{
	// Un même message n'est pas empilé : on relance simplement sa durée
	for (FMessage& Message : Messages)
	{
		if (Message.Text == Text)
		{
			Message.Color = Color;
			Message.TimeRemaining = Duration;
			return;
		}
	}

	Messages.Add({ Text, Color, Duration });
	constexpr int32 MaxMessages = 4;
	if (Messages.Num() > MaxMessages)
	{
		Messages.RemoveAt(0);
	}
}

void ANFHUD::DrawCrosshairAndPrompt()
{
	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;

	APawn* Pawn = GetOwningPawn();

	// Pièce en main : le réticule indique où elle sera posée ou insérée
	const UNFGrabberComponent* Grabber = Pawn ? Pawn->FindComponentByClass<UNFGrabberComponent>() : nullptr;
	if (Grabber && Grabber->IsHolding())
	{
		const bool bSnap = Grabber->GetSnapTarget() != nullptr;
		const FLinearColor HoldColor = bSnap ? FLinearColor(0.2f, 1.f, 0.5f) : FocusCrosshairColor;
		DrawRect(HoldColor, CenterX - 5.f, CenterY - 5.f, 10.f, 10.f);
		DrawCenteredText(FString::Printf(TEXT("[E] %s"), *Grabber->GetDropPrompt().ToString()), HoldColor, CenterX, CenterY + 22.f, 1.2f);
		return;
	}

	const UNFInteractorComponent* Interactor = Pawn ? Pawn->FindComponentByClass<UNFInteractorComponent>() : nullptr;
	AActor* Focus = Interactor ? Interactor->GetFocusedActor() : nullptr;
	const bool bCanInteract = Focus && Interactor->CanInteractWithFocus();

	const FLinearColor Color = Focus ? (bCanInteract ? FocusCrosshairColor : DisabledColor) : IdleCrosshairColor;
	const float Size = Focus ? 8.f : 4.f;
	DrawRect(Color, CenterX - Size * 0.5f, CenterY - Size * 0.5f, Size, Size);

	if (Focus)
	{
		const FText Prompt = INFInteractable::Execute_GetInteractionPrompt(Focus);
		const FString Text = bCanInteract
			? FString::Printf(TEXT("[E] %s"), *Prompt.ToString())
			: FString::Printf(TEXT("%s (indisponible)"), *Prompt.ToString());
		DrawCenteredText(Text, Color, CenterX, CenterY + 22.f, 1.2f);
	}
}

void ANFHUD::DrawMessages(float DeltaSeconds)
{
	float Y = Canvas->ClipY * 0.12f;
	for (int32 i = 0; i < Messages.Num(); ++i)
	{
		FMessage& Message = Messages[i];
		Message.TimeRemaining -= DeltaSeconds;

		// Fondu sur la dernière demi-seconde
		FLinearColor Color = Message.Color;
		Color.A *= FMath::Clamp(Message.TimeRemaining / 0.5f, 0.f, 1.f);
		DrawCenteredText(Message.Text, Color, Canvas->ClipX * 0.5f, Y, 1.6f);
		Y += 34.f;
	}
	Messages.RemoveAll([](const FMessage& Message) { return Message.TimeRemaining <= 0.f; });
}

void ANFHUD::DrawCenteredText(const FString& Text, const FLinearColor& Color, float CenterX, float Y, float Scale)
{
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!Font)
	{
		return;
	}

	float Width = 0.f;
	float Height = 0.f;
	GetTextSize(Text, Width, Height, Font, Scale);

	// Ombre portée pour rester lisible sur tous les fonds
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.8f), CenterX - Width * 0.5f + 2.f, Y + 2.f, Font, Scale);
	DrawText(Text, Color, CenterX - Width * 0.5f, Y, Font, Scale);
}
