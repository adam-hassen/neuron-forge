// Neuron Forge - ENSIM

#include "Lab/NFExitTrigger.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Player/NFPlayerController.h"
#include "ProjetNeuro.h"

ANFExitTrigger::ANFExitTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = Trigger;
	Trigger->SetBoxExtent(FVector(100.f, 150.f, 150.f));
	Trigger->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->ShapeColor = FColor(80, 255, 120);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Trigger);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(20.f);
	Label->SetText(FText::FromString(TEXT("SORTIE")));
	Label->SetTextRenderColor(FColor(80, 255, 120));
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANFExitTrigger::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ANFExitTrigger::HandleTriggerBegin);
}

void ANFExitTrigger::HandleTriggerBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (bCompleted || !Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	bCompleted = true;
	UE_LOG(LogNeuronForge, Log, TEXT("Niveau terminé."));

	if (CompletionSound)
	{
		UGameplayStatics::PlaySound2D(this, CompletionSound);
	}
	if (ANFPlayerController* PC = Cast<ANFPlayerController>(Pawn->GetController()))
	{
		PC->ShowMessage(CompletionMessage, FLinearColor(0.3f, 1.f, 0.45f), 8.f);
	}
	Label->SetText(FText::FromString(TEXT("BRAVO !")));
	OnLevelCompleted.Broadcast(Pawn);
}
