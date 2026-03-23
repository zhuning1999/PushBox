// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridManager.h"
#include "GridTypes.h"
#include "LevelData.generated.h"

/**
 * 
 */
USTRUCT()
struct FLevelData
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FGridPos> Walls;

    UPROPERTY()
    TArray<FGridPos> Boxes;

    UPROPERTY()
    TArray<FGridPos> Goals;

    UPROPERTY()
    FGridPos PlayerStart;

    UPROPERTY()
    TArray<FGridPos> Ice;
};