// Fill out your copyright notice in the Description page of Project Settings.


#include "BoxActor.h"
#include "GridTypes.h"
#include "GridManager.h"
#include <Kismet/GameplayStatics.h>
#include "LevelEditorManager.h"

// Sets default values
ABoxActor::ABoxActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ABoxActor::BeginPlay()
{
	Super::BeginPlay();

	Grid = Cast<AGridManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass())
	);
	if (Grid)
	{
		Grid->Boxes.Add(this);
	}
}

void ABoxActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	Grid = Cast<AGridManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass())
	);
	if (Grid)
	{
		Grid->Boxes.Remove(this);
	}
}

// Called every frame
void ABoxActor::Tick(float DeltaTime)
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

		// 同人一样的溜冰逻辑
		if (bIsSliding)
		{
			//FGridPos Next = Pos + SlideDirection;

			//if (Grid->IsWalkable(Next))
			//{
			//	Pos = Next;
			//	StartMove(Grid->GridToWorld(Pos));
			//}
			//else
			//{
			//	bIsSliding = false;
			//}

			FGridPos Next = Pos + SlideDirection;

			if (Grid->IsWalkable(Next))
			{
				bool bNextIsIce = Grid->IsIce(Next);

				Pos = Next;
				StartMove(Grid->GridToWorld(Pos));

				bIsSliding = bNextIsIce;
			} else
			{
				bIsSliding = false;	
			}
		}

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

void ABoxActor::StartMove(FVector NewTarget)
{
	StartLocation = GetActorLocation();
	TargetLocation = NewTarget;

	MoveAlpha = 0.f;
	bIsMoving = true;
}

void ABoxActor::StartSlide(FGridPos Direction)
{
	bIsSliding = true;
	SlideDirection = Direction;
}
