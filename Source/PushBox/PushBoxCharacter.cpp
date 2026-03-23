// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "BoxActor.h"
#include "GridTypes.h"
#include <Kismet/GameplayStatics.h>
#include "LevelEditorManager.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// APushBoxCharacter

APushBoxCharacter::APushBoxCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 600.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = false; // Rotate the arm based on the controller
	CameraBoom->SetRelativeRotation(FRotator(-75.f, 0.f, 0.f)); // Adjust the boom rotation to look down at the character

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void APushBoxCharacter::SetCurrentPos(FGridPos Pos)
{
	CurrentPos = Pos;
}

//////////////////////////////////////////////////////////////////////////
// Input

void APushBoxCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void APushBoxCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APushBoxCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &APushBoxCharacter::OnMoveReleased);

		// Looking
		// EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APushBoxCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

FVector APushBoxCharacter::GridToWorld(const FGridPos& GridPosition) const
{
    return FVector(GridPosition.X * Grid->GridSize, GridPosition.Y * Grid->GridSize, -20.f);
}

void APushBoxCharacter::TryMove(const FGridPos& Direction)
{
	if (!Grid || bIsMoving) return;

	FGridPos Target = CurrentPos + Direction;

	// 先判断墙/边界
	if (!Grid->IsInside(Target) || Grid->IsWallAt(Target)) return;

	ABoxActor* Box = Grid->GetBoxAt(Target);
	// 如果是空地
	if (!Box)
	{
		CurrentPos = Target;
		StartMove(CurrentPos);

		// 如果走到冰上，就开始滑
		if (Grid->IsIce(CurrentPos))
		{
			bIsSliding = true;
			SlideDirection = Direction;
		}

		return;
	}

	// 如果是箱子
	if (Box)
	{
		FGridPos BoxTarget = Target + Direction;
		// 如果箱子前面是空地
		if (Grid->IsWalkable(BoxTarget))
		{
			// 箱子走一步
			Box->Pos = BoxTarget;
			Box->StartMove(Grid->GridToWorld(BoxTarget)); // 这里为了方便，就直接转好成世界坐标后再给他

			// 如果箱子走到冰上，标记箱子滑
			if (Grid->IsIce(BoxTarget))
			{
				Box->StartSlide(Direction); // 开始溜冰
			}

			// 玩家走一步
			CurrentPos = Target;
			StartMoveBox(CurrentPos);

			if (Grid->CheckWin())
			{
				ALevelEditorManager* Manager = Cast<ALevelEditorManager>(
					UGameplayStatics::GetActorOfClass(GetWorld(), ALevelEditorManager::StaticClass())
				);

				if (Manager)
				{
					Manager->NextLevel();
				}
			}
		}
	}
}

void APushBoxCharacter::OnMoveReleased(const FInputActionValue& Value)
{
	bCanMove = true;
}

void APushBoxCharacter::StartMoveBox(FGridPos Target) // 目前和StartMove一样，但如果需要区分玩家和箱子的移动逻辑，可以在这里修改
{
	StartLocation = GetActorLocation(); // 当前位置作为起点
	TargetLocation = Grid->GridToWorld(Target); // 目标位置是格子坐标转成的世界坐标，也就是Target

	MoveAlpha = 0.f; // 移动进度重置为0
	bIsMoving = true;
}

void APushBoxCharacter::StartMove(FGridPos Target)
{
	StartLocation = GetActorLocation(); // 当前位置作为起点
	TargetLocation = Grid->GridToWorld(Target); // 目标位置是格子坐标转成的世界坐标，也就是Target

	MoveAlpha = 0.f; // 移动进度重置为0
	bIsMoving = true;
}

void APushBoxCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->GravityScale = 0.f;
	// Initialize character position
	Grid = Cast<AGridManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass())
	);

	FVector Loc = GetActorLocation();

	int32 X = FMath::RoundToInt(Loc.X / Grid->GridSize);
	int32 Y = FMath::RoundToInt(Loc.Y / Grid->GridSize);

	CurrentPos = FGridPos(X, Y);
	SetActorLocation(GridToWorld(CurrentPos));

	// 把鼠标显示出来，方便编辑器里测试，并且不锁定中心
	auto PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
	}
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	PC->SetInputMode(InputMode);
}

void APushBoxCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsMoving) return;

	MoveAlpha += DeltaTime * MoveSpeed;

	float Alpha = FMath::Clamp(MoveAlpha, 0.f, 1.f);

	FVector NewLoc = FMath::Lerp(StartLocation, TargetLocation, Alpha);
	SetActorLocation(NewLoc);

	if (Alpha >= 1.f)
	{
		bIsMoving = false;

		// 判断是否继续溜冰
		if (bIsSliding)
		{
			FGridPos Next = CurrentPos + SlideDirection;

			if (Grid->IsWalkable(Next))
			{
				bool bNextIsIce = Grid->IsIce(Next);

				CurrentPos = Next;
				StartMove(CurrentPos);

				// ❗决定是否继续滑
				bIsSliding = bNextIsIce;
			}
			else
			{
				bIsSliding = false;
			}
		}
	}
}

void APushBoxCharacter::Move(const FInputActionValue& Value)
{
	if (!bCanMove || bIsMoving) return;
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (MovementVector.IsNearlyZero()) return;

	bCanMove = false;

	if (MovementVector.Y > 0) TryMove({ 1,0 });
	if (MovementVector.Y < 0) TryMove({ -1,0 });
	if (MovementVector.X < 0) TryMove({ 0,-1 });
	if (MovementVector.X > 0) TryMove({ 0,1 });
}

void APushBoxCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
