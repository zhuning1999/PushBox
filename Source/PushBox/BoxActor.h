// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridManager.h"
#include "GridTypes.h"
#include "BoxActor.generated.h"

UCLASS()
class PUSHBOX_API ABoxActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABoxActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called when the actor is being removed from the level
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "PushBox")
	FGridPos Pos;
	UPROPERTY(VisibleAnywhere, Category = "PushBox")
	AGridManager* Grid;

	bool bIsMoving = false;
	FVector StartLocation;
	FVector TargetLocation;
	float MoveAlpha = 0.f;
	float MoveSpeed = 5.f;

	void StartMove(FVector TargetLocation);

	// 溜冰逻辑
	FGridPos SlideDirection;
	bool bIsSliding = false;
	void StartSlide(FGridPos Direction);
};
