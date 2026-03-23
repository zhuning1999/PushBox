// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GridManager.h"
#include "GridTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PushBoxCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class APushBoxCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	// Current grid position of the character
	UPROPERTY(EditAnywhere, Category = "PushBox")
	FGridPos CurrentPos;
	UPROPERTY(VisibleAnywhere, Category = "PushBox")
	AGridManager* Grid;

public:
	APushBoxCharacter();
	
	UFUNCTION()
	void SetCurrentPos(FGridPos Pos);
	UFUNCTION()
	FGridPos GetCurrentPos() { return CurrentPos; }

protected:
	/** Called for BeginPlay **/
	void BeginPlay() override;

	/** Called for Tick **/
	void Tick(float DeltaTime) override;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
			

protected:

	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
/****************************************************************************************/
	// Box pushing logic
protected:
	UFUNCTION()
	FVector GridToWorld(const FGridPos& GridPosition) const;
	UFUNCTION()
	void TryMove(const FGridPos& Direction);
	UFUNCTION()
	void OnMoveReleased(const FInputActionValue& Value);

	void StartMoveBox(FGridPos TargetLocation);
	void StartMove(FGridPos TargetLocation);
public:
	bool bCanMove = true;
	bool bIsMoving = false;
	
	FVector StartLocation;
	FVector TargetLocation;

	float MoveSpeed = 5.f; // Speed of movement interpolation, higher is faster
	float MoveAlpha = 0.f; // 0 to 1, used for interpolation during movement

	// 溜冰相关
	FGridPos SlideDirection;
	bool bIsSliding = false;

};

