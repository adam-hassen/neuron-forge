// Neuron Forge - ENSIM

#include "Lab/NFLayerModule.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "NeuronForgeModule"

namespace
{
	constexpr float ModuleSize = 50.f;		// cube de 50 cm
	constexpr float OrbRingRadius = 46.f;	// couronne des orbes autour du cube
}

ANFLayerModule::ANFLayerModule()
{
	PieceType = ENFPieceType::LayerModule;
	GlowColor = FLinearColor(0.15f, 0.7f, 1.f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	Mesh->SetStaticMesh(CubeMesh.Object);
	Mesh->SetRelativeScale3D(FVector(ModuleSize / 100.f));
	Mesh->SetMassOverrideInKg(NAME_None, 20.f, true);

	// Les emplacements sont attachés au mesh (mis à l'échelle 0.5) : positions en unités locales
	const float LocalScale = 100.f / ModuleSize;
	for (int32 i = 0; i < MaxNeurons; ++i)
	{
		const float Angle = 2.f * PI * i / MaxNeurons;
		UNFSocketComponent* OrbSocket = CreateDefaultSubobject<UNFSocketComponent>(*FString::Printf(TEXT("OrbSocket%d"), i));
		OrbSocket->SetupAttachment(Mesh);
		OrbSocket->SetRelativeLocation(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * OrbRingRadius * LocalScale);
		OrbSocket->SetAbsolute(false, false, true);
		OrbSocket->AcceptedType = ENFPieceType::NeuronOrb;
		OrbSocket->SnapRadius = 75.f;
		OrbSocket->SocketName = LOCTEXT("OrbSocket", "le module");
		OrbSockets.Add(OrbSocket);
	}

	CrystalSocket = CreateDefaultSubobject<UNFSocketComponent>(TEXT("CrystalSocket"));
	CrystalSocket->SetupAttachment(Mesh);
	CrystalSocket->SetRelativeLocation(FVector(0.f, 0.f, (ModuleSize * 0.5f + 14.f) * LocalScale));
	CrystalSocket->SetAbsolute(false, false, true);
	CrystalSocket->AcceptedType = ENFPieceType::ActivationCrystal;
	CrystalSocket->SnapRadius = 70.f;
	CrystalSocket->SocketName = LOCTEXT("CrystalSocket", "le dessus du module");

	Summary = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Summary"));
	Summary->SetupAttachment(Mesh);
	Summary->SetAbsolute(false, true, true);
	Summary->SetRelativeLocation(FVector(0.f, 0.f, (ModuleSize * 0.5f + 42.f) * LocalScale));
	Summary->SetHorizontalAlignment(EHTA_Center);
	Summary->SetVerticalAlignment(EVRTA_TextBottom);
	Summary->SetWorldSize(9.f);
	Summary->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

int32 ANFLayerModule::GetNeuronCount() const
{
	int32 Count = 0;
	for (const UNFSocketComponent* OrbSocket : OrbSockets)
	{
		Count += (OrbSocket && !OrbSocket->IsFree()) ? 1 : 0;
	}
	return Count;
}

bool ANFLayerModule::HasCrystal() const
{
	return CrystalSocket && !CrystalSocket->IsFree();
}

ENFActivation ANFLayerModule::GetLayerActivation() const
{
	const ANFPiece* Crystal = HasCrystal() ? CrystalSocket->GetPiece() : nullptr;
	return Crystal ? Crystal->Activation : ENFActivation::Linear;
}

void ANFLayerModule::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Summary)
	{
		return;
	}

	// Le résumé fait face au joueur
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const FVector ToPlayer = Player->GetActorLocation() - Summary->GetComponentLocation();
		Summary->SetWorldRotation(FRotator(0.f, ToPlayer.Rotation().Yaw, 0.f));
	}

	const int32 Neurons = GetNeuronCount();
	const FString Text = FString::Printf(TEXT("%d neurone%s · %s"), Neurons, Neurons > 1 ? TEXT("s") : TEXT(""),
		*NFActivation::ToString(GetLayerActivation()));
	Summary->SetText(FText::FromString(Text));
	Summary->SetTextRenderColor((Neurons > 0 ? NFActivation::GetColor(GetLayerActivation()) : FLinearColor(1.f, 0.4f, 0.3f)).ToFColor(true));
}

#undef LOCTEXT_NAMESPACE
