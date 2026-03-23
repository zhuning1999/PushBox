#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridTypes.h"
#include "GridManager.generated.h"

class ABoxActor;
class AGoalActor;

UENUM()
enum class ETileType : uint8
{
    Empty,
    Wall,
    Ice
};

UCLASS()
class PUSHBOX_API AGridManager : public AActor
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere)
    int32 Width = 10;

    UPROPERTY(EditAnywhere)
    int32 Height = 10;

    UPROPERTY(EditAnywhere)
    float GridSize = 100.f;

    // 格子初始化为一维数组
	// 0 = 空地, 1 = 墙, 2 = 冰面
    TArray<ETileType> GridData;

	// 场景中所有箱子
    UPROPERTY(VisibleAnywhere)
    TArray<ABoxActor*> Boxes;
    // 目标点
    UPROPERTY(VisibleAnywhere)
    TArray<AGoalActor*> Goals;
    // 墙的位置
    UPROPERTY(VisibleAnywhere)
    TArray<FGridPos> Walls;
	// 冰面的位置
    UPROPERTY(VisibleAnywhere)
    TArray<FGridPos> IceTiles;

    virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

    bool IsInside(FGridPos Pos) const;
    bool IsWalkable(FGridPos Pos) const;

	// 根据格子坐标获取该坐标箱子
	ABoxActor* GetBoxAt(FGridPos Pos) const;

    // 检查是否胜利
	bool CheckWin() const;

	// 把格子坐标转换成世界坐标
    FVector GridToWorld(FGridPos Pos) const;

	// 判断指定位置是否有墙
    bool IsWallAt(FGridPos Pos) const { return Walls.Contains(Pos); }
    // 判断指定位置是否有目标点
	bool IsGoal(FGridPos Pos) const;

    ETileType GetTile(FGridPos Pos) const { return GridData[Pos.Y * Width + Pos.X]; }
    bool IsIce(FGridPos Pos) const{ return GetTile(Pos) == ETileType::Ice; }
};