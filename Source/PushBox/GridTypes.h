// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridTypes.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FGridPos
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 X;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Y;

    FGridPos() : X(0), Y(0) {}
    FGridPos(int32 InX, int32 InY) : X(InX), Y(InY) {}

    FGridPos operator+(const FGridPos& Other) const
    {
        return FGridPos(X + Other.X, Y + Other.Y);
    }

    bool operator==(const FGridPos& Other) const
    {
        return X == Other.X && Y == Other.Y;
    }
};