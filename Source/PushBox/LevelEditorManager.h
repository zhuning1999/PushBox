// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridTypes.h"
#include "LevelData.h"
#include "LevelEditorManager.generated.h"

// 放置类型
UENUM()
enum class EPlaceType : uint8
{
	Wall,
	Box,
	Goal,
	Player,
    Ice
};

UCLASS()
class PUSHBOX_API ALevelEditorManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALevelEditorManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
    int32 CurrentLevel = 1;
    UPROPERTY(EditAnywhere)
    int32 MaxLevel = 3;

// ===== 编辑器功能 =====
private:

    // 当前放置类型
    EPlaceType CurrentType = EPlaceType::Wall;

    // 类引用（蓝图指定）
    UPROPERTY(EditAnywhere)
    TSubclassOf<class ABoxActor> BoxClass;

    UPROPERTY(EditAnywhere)
    TSubclassOf<class AGoalActor> GoalClass;

    UPROPERTY(EditAnywhere)
    TSubclassOf<class AActor> WallClass;

    UPROPERTY(EditAnywhere)
    TSubclassOf<class AActor> IceClass;

    UPROPERTY(EditAnywhere)
    class AGridManager* Grid;

	// 本质是借助玩家控制器来获取鼠标输入和位置
    APlayerController* PC;

    // 是否在放置模式
    bool bIsPlacing = false;

    // 预览位置
    FGridPos PreviewPos;

    // 预览Actor
    UPROPERTY()
    AActor* PreviewActor = nullptr;
    // 预览颜色
    UPROPERTY(EditAnywhere)
	UMaterialInterface* ValidMat; // 合法位置预览材质
    UPROPERTY(EditAnywhere)
	UMaterialInterface* InvalidMat; // 非法位置预览材质
	// 右键双击检测
    float LastRightClickTime = -1.f;
    float DoubleClickThreshold = 0.25f; // 250ms

    // ===== 核心功能 =====
	// 左键放置，右键取消
    void ConfirmPlacement();
	void CancelPlacement();
    // 双击右键删除
	void DeleteAt(FGridPos Pos);
    FGridPos GetMouseGridPos();

    void SpawnAt(FGridPos Pos);

    void SpawnBox(FGridPos Pos);
    void SpawnGoal(FGridPos Pos);
    void SpawnWall(FGridPos Pos);
    void SpawnIce(FGridPos Pos);
    void SetPlayerStart(FGridPos Pos);
	// 放置各类物品的公共函数
	void StartPlacing(TSubclassOf<AActor> ActorClass);
	bool CanPlace(FGridPos Pos);

    // ===== 存档 =====

    void SaveLevel();
    void LoadLevel();
    void ClearLevel();
public:
    void NextLevel();

};
