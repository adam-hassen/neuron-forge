// Neuron Forge - ENSIM

#include "Player/NFCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Interaction/NFGrabberComponent.h"
#include "Interaction/NFInteractorComponent.h"
#include "ProjetNeuro.h"
#include "UObject/ConstructorHelpers.h"

namespace NFCharacterAssets
{
	// Assets du template First Person réutilisés par le personnage
	const TCHAR* Mesh = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple");
	const TCHAR* FirstPersonAnim = TEXT("/Game/FirstPerson/Anims/ABP_FP_Copy");
	const TCHAR* BodyAnim = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed");
	const TCHAR* DefaultContext = TEXT("/Game/Input/IMC_Default");
	const TCHAR* MouseLookContext = TEXT("/Game/Input/IMC_MouseLook");
	const TCHAR* Jump = TEXT("/Game/Input/Actions/IA_Jump");
	const TCHAR* Move = TEXT("/Game/Input/Actions/IA_Move");
	const TCHAR* Look = TEXT("/Game/Input/Actions/IA_Look");
	const TCHAR* MouseLook = TEXT("/Game/Input/Actions/IA_MouseLook");
}

ANFCharacter::ANFCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// --- Capsule et mouvement (valeurs du template) ---
	GetCapsuleComponent()->InitCapsuleSize(34.f, 96.f);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = 600.f;
	Movement->JumpZVelocity = 420.f;
	Movement->AirControl = 0.6f;
	Movement->BrakingDecelerationWalking = 2048.f;
	Movement->BrakingDecelerationFalling = 1500.f;

	// --- Corps complet : invisible pour le joueur, mais projette son ombre ---
	USkeletalMeshComponent* BodyMesh = GetMesh();
	BodyMesh->SetRelativeLocationAndRotation(FVector(-20.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));
	BodyMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;
	BodyMesh->SetOwnerNoSee(true);

	// --- Bras FP : visibles uniquement par le joueur, rendus avec le FOV first person ---
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(BodyMesh);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->SetCastShadow(false);
	FirstPersonMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	// --- Caméra fixée sur l'os "head" ---
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(FirstPersonMesh, FName(TEXT("head")));
	FirstPersonCamera->SetRelativeLocationAndRotation(FVector(2.79f, 5.88f, 0.f), FRotator(0.f, 90.f, -90.f));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetFieldOfView(90.f);
	FirstPersonCamera->bEnableFirstPersonFieldOfView = true;
	FirstPersonCamera->FirstPersonFieldOfView = 70.f;
	FirstPersonCamera->bEnableFirstPersonScale = true;
	FirstPersonCamera->FirstPersonScale = 0.6f;

	// --- Interaction ---
	Interactor = CreateDefaultSubobject<UNFInteractorComponent>(TEXT("Interactor"));
	Grabber = CreateDefaultSubobject<UNFGrabberComponent>(TEXT("Grabber"));

	// --- Assets du template ---
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(NFCharacterAssets::Mesh);
	static ConstructorHelpers::FClassFinder<UAnimInstance> FirstPersonAnimClass(NFCharacterAssets::FirstPersonAnim);
	static ConstructorHelpers::FClassFinder<UAnimInstance> BodyAnimClass(NFCharacterAssets::BodyAnim);
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultContextAsset(NFCharacterAssets::DefaultContext);
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MouseLookContextAsset(NFCharacterAssets::MouseLookContext);
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpAsset(NFCharacterAssets::Jump);
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveAsset(NFCharacterAssets::Move);
	static ConstructorHelpers::FObjectFinder<UInputAction> LookAsset(NFCharacterAssets::Look);
	static ConstructorHelpers::FObjectFinder<UInputAction> MouseLookAsset(NFCharacterAssets::MouseLook);

	if (MeshAsset.Succeeded())
	{
		BodyMesh->SetSkeletalMeshAsset(MeshAsset.Object);
		FirstPersonMesh->SetSkeletalMeshAsset(MeshAsset.Object);
	}
	if (BodyAnimClass.Succeeded())
	{
		BodyMesh->SetAnimInstanceClass(BodyAnimClass.Class);
	}
	if (FirstPersonAnimClass.Succeeded())
	{
		FirstPersonMesh->SetAnimInstanceClass(FirstPersonAnimClass.Class);
	}
	if (DefaultContextAsset.Succeeded())
	{
		DefaultMappingContexts.Add(DefaultContextAsset.Object);
	}
	if (MouseLookContextAsset.Succeeded())
	{
		DefaultMappingContexts.Add(MouseLookContextAsset.Object);
	}
	JumpAction = JumpAsset.Object;
	MoveAction = MoveAsset.Object;
	LookAction = LookAsset.Object;
	MouseLookAction = MouseLookAsset.Object;
}

void ANFCharacter::EnsureInteractionInput()
{
	if (InteractAction)
	{
		return;
	}

	// Action booléenne créée à l'exécution : aucun asset supplémentaire n'est nécessaire
	InteractAction = NewObject<UInputAction>(this, TEXT("IA_Interact_Runtime"));
	InteractAction->ValueType = EInputActionValueType::Boolean;

	InteractionMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Interaction_Runtime"));
	InteractionMappingContext->MapKey(InteractAction, EKeys::E);
	InteractionMappingContext->MapKey(InteractAction, EKeys::LeftMouseButton);
	InteractionMappingContext->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);
}

void ANFCharacter::AddMappingContexts()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
		: nullptr;
	if (!Subsystem)
	{
		return;
	}

	for (UInputMappingContext* Context : DefaultMappingContexts)
	{
		if (Context && !Subsystem->HasMappingContext(Context))
		{
			Subsystem->AddMappingContext(Context, 0);
		}
	}

	EnsureInteractionInput();
	if (InteractionMappingContext && !Subsystem->HasMappingContext(InteractionMappingContext))
	{
		Subsystem->AddMappingContext(InteractionMappingContext, 1);
	}
}

void ANFCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	AddMappingContexts();
}

void ANFCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogNeuronForge, Error, TEXT("%s : Enhanced Input est requis."), *GetName());
		return;
	}

	EnsureInteractionInput();

	if (JumpAction)
	{
		Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (MoveAction)
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANFCharacter::Move);
	}
	if (LookAction)
	{
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANFCharacter::Look);
	}
	if (MouseLookAction)
	{
		Input->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ANFCharacter::Look);
	}
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &ANFCharacter::Interact);

	// Le contexte d'interaction est créé ici si le contrôleur était déjà assigné
	AddMappingContexts();
}

void ANFCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Controller)
	{
		AddMovementInput(GetActorRightVector(), Axis.X);
		AddMovementInput(GetActorForwardVector(), Axis.Y);
	}
}

void ANFCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Controller)
	{
		AddControllerYawInput(Axis.X);
		AddControllerPitchInput(Axis.Y);
	}
}

void ANFCharacter::Interact()
{
	// Mains pleines : on pose (ou on insère) la pièce tenue
	if (Grabber && Grabber->IsHolding())
	{
		Grabber->Drop();
		return;
	}
	// Sinon on agit sur l'objet visé : une pièce se laisse attraper, un levier se tire...
	if (Interactor)
	{
		Interactor->TryInteract();
	}
}
