// Fill out your copyright notice in the Description page of Project Settings.


#include "GridManager.h"
#include "Kismet/GameplayStatics.h"
#include "BoxActor.h"
#include "GoalActor.h"

void AGridManager::BeginPlay()
{
    Super::BeginPlay();

    GridData.SetNum(Width * Height);

    // 初始化为全空地
    for (int i = 0; i < GridData.Num(); i++)
    {
        GridData[i] = ETileType::Empty;
    }

    for (auto& Pos : IceTiles)
    {
        int Index = Pos.Y * Width + Pos.X;
        if (GridData.IsValidIndex(Index))
        {
            GridData[Index] = ETileType::Ice;
        }
    }

    //// 简单造一圈墙（边界）
    //for (int x = 0; x < Width; x++)
    //{
    //    GridData[x] = 1; // 下边
    //    GridData[x + (Height - 1) * Width] = 1; // 上边
    //}

    //for (int y = 0; y < Height; y++)
    //{
    //    GridData[y * Width] = 1; // 左
    //    GridData[y * Width + Width - 1] = 1; // 右
    //}

	// 场景中所有箱子都放到 TempActors 数组里
    TArray<AActor*> TempActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABoxActor::StaticClass(), TempActors);
    for (AActor* Actor : TempActors)
    {
        if (ABoxActor* Box = Cast<ABoxActor>(Actor))
        {
            Boxes.Add(Box);

            //// 把当前盒子的世界坐标转成Grid坐标
            FVector Loc = Box->GetActorLocation();

            int32 X = FMath::RoundToInt(Loc.X / GridSize);
            int32 Y = FMath::RoundToInt(Loc.Y / GridSize);

            Box->Pos = FGridPos(X, Y);

			// 把盒子移动到格子位置
            FVector SnapLoc = GridToWorld(Box->Pos);
            Box->SetActorLocation(SnapLoc);
        }
    }

	// 初始化目标点
	// 场景中所有目标点都放到 FoundGoals 数组里
    TArray<AActor*> FoundGoals;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGoalActor::StaticClass(), FoundGoals);

    for (AActor* Actor : FoundGoals)
    {
        AGoalActor* Goal = Cast<AGoalActor>(Actor);
        if (Goal)
        {
            Goals.Add(Goal);

            FVector Loc = Goal->GetActorLocation();

            int32 X = FMath::RoundToInt(Loc.X / GridSize);
            int32 Y = FMath::RoundToInt(Loc.Y / GridSize);

            Goal->Pos = FGridPos(X, Y);

            FVector SnapLoc = GridToWorld(Goal->Pos);
            Goal->SetActorLocation(SnapLoc);
        }
    }
}

void AGridManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    Boxes.RemoveAll([](ABoxActor* Box)
    {
        return !IsValid(Box);
    });
    Goals.RemoveAll([](AGoalActor* Goal)
    {
        return !IsValid(Goal);
    });
}

bool AGridManager::IsInside(FGridPos Pos) const
{
    return Pos.X >= 0 && Pos.X < Width &&
        Pos.Y >= 0 && Pos.Y < Height;
}

bool AGridManager::IsWalkable(FGridPos Pos) const
{
    if (!IsInside(Pos)) // 空气墙
        return false;

    if (IsWallAt(Pos)) // 有墙
        return false;

    if (GetBoxAt(Pos)) // 有箱子，并获取Box*
        return false;
    
    return true; // 主要是判断是不是空地
}

ABoxActor* AGridManager::GetBoxAt(FGridPos Pos) const
{
    for (auto Box : Boxes)
    {
        if (Box && Box->Pos.X == Pos.X && Box->Pos.Y == Pos.Y) 
        {
			return Box;
        }
    }

    return nullptr;
}

bool AGridManager::CheckWin() const
{
    for (auto Goal : Goals)
    {
        if (!GetBoxAt(Goal->Pos))
        {
            return false;
        }
    }

    return true;
}

FVector AGridManager::GridToWorld(FGridPos Pos) const
{
	return FVector(Pos.X * GridSize, Pos.Y * GridSize, -20.f);
}

bool AGridManager::IsGoal(FGridPos Pos) const
{
    for (AGoalActor* Goal : Goals)
    {
        if (!IsValid(Goal)) continue;

        if (Goal->Pos == Pos)
            return true;
    }

    return false;
}
