// Fill out your copyright notice in the Description page of Project Settings.


#include "GoalActor.h"
#include <Kismet/GameplayStatics.h>

// Sets default values
AGoalActor::AGoalActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGoalActor::BeginPlay()
{
	Super::BeginPlay();

	AGridManager* Grid = Cast<AGridManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass())
	);
	if (Grid)
	{
		Grid->Goals.Add(this);
	}
}

void AGoalActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	AGridManager* Grid = Cast<AGridManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass())
	);
	if (Grid)
	{
		Grid->Goals.Remove(this);
	}
}

// Called every frame
void AGoalActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

