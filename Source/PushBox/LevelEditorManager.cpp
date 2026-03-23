// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelEditorManager.h"
#include "Kismet/GameplayStatics.h"
#include "BoxActor.h"
#include "GoalActor.h"
#include "GridManager.h"
#include "GameFramework/Character.h"
#include "WallActor.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PushBoxCharacter.h"
#include "IceActor.h"
#include <EngineUtils.h>

// Sets default values
ALevelEditorManager::ALevelEditorManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ALevelEditorManager::BeginPlay()
{
	Super::BeginPlay();

	// 获取 GridManager 引用
    Grid = Cast<AGridManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass())
    );

    PC = GetWorld()->GetFirstPlayerController();
    
    CurrentLevel = 1;
    LoadLevel();
}

// Called every frame
void ALevelEditorManager::Tick(float DeltaTime)
{

	Super::Tick(DeltaTime);

    if (!PC) return;
    
	// 如果不在放置模式，显示鼠标所在格子的绿色框
    if (!bIsPlacing) 
    {
        FGridPos Pos = GetMouseGridPos();
        FVector Loc = Grid->GridToWorld(Pos);

        DrawDebugBox(
            GetWorld(),
            Loc,
            FVector(Grid->GridSize * 0.5f),
            FColor::Green,
            false,
            0.f,
            0,
            2.f
        );
    }

	// 更新预览位置，使其跟随鼠标
    if (bIsPlacing && PreviewActor && Grid)
    {
        FGridPos Pos = GetMouseGridPos();
        FVector Loc = Grid->GridToWorld(Pos);

        PreviewActor->SetActorLocation(Loc);

        PreviewPos = Pos;
        // 更新材质颜色
        bool bValid = CanPlace(PreviewPos);

        UStaticMeshComponent* Mesh = PreviewActor->FindComponentByClass<UStaticMeshComponent>();

        if (Mesh && ValidMat && InvalidMat)
        {
            Mesh->SetMaterial(0, bValid ? ValidMat : InvalidMat);
        }
    }

    // 鼠标左键
    if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
    {
        if (bIsPlacing)
        {
			// 确保鼠标在Grid上，没有飞到天上去
            FHitResult Hit;
            if (!PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
            {
                return;
            }
			// 如果格子上已经有东西了，就别放了
            if (!CanPlace(PreviewPos))
            {
                return;
            }
            ConfirmPlacement();
        }
    }
	// 鼠标右键
    if (PC->WasInputKeyJustPressed(EKeys::RightMouseButton))
    {
        float Now = GetWorld()->GetTimeSeconds();

        if (Now - LastRightClickTime < DoubleClickThreshold)
        {
            // 双击触发删除
            if (!bIsPlacing)
            {
                DeleteAt(GetMouseGridPos());
            }

            LastRightClickTime = -1.f; // 重置
        }
        else
        {
            // 第一次点击
            LastRightClickTime = Now;

            // 如果在放置模式，仍然允许取消
            if (bIsPlacing)
            {
                CancelPlacement();
            }
        }
    }

    // 切换类型
    if (PC->WasInputKeyJustPressed(EKeys::One))
    {
        CurrentType = EPlaceType::Wall;
        StartPlacing(WallClass);
    }

    if (PC->WasInputKeyJustPressed(EKeys::Two))
    {
        CurrentType = EPlaceType::Box;
        StartPlacing(BoxClass);
    }

    if (PC->WasInputKeyJustPressed(EKeys::Three))
    {
        CurrentType = EPlaceType::Goal;
        StartPlacing(GoalClass);
    }
    if (PC->WasInputKeyJustPressed(EKeys::Four))
    {
        CurrentType = EPlaceType::Ice;
        StartPlacing(IceClass);
    }
    if (PC->WasInputKeyJustPressed(EKeys::Five))  CurrentType = EPlaceType::Player;

    // 保存 / 加载 / 清场
    if (PC->WasInputKeyJustPressed(EKeys::Q)) SaveLevel();
    if (PC->WasInputKeyJustPressed(EKeys::E)) LoadLevel();
    if (PC->WasInputKeyJustPressed(EKeys::C)) ClearLevel();
}

void ALevelEditorManager::ConfirmPlacement()
{
    if (!Grid) return;
	// 在预览位置放置物品
    SpawnAt(PreviewPos);
	// 放完就丢掉预览
    CancelPlacement();
    //if (PreviewActor)
    //{
    //    PreviewActor->Destroy();
    //    PreviewActor = nullptr;
    //}
    //
    //bIsPlacing = false;
}

void ALevelEditorManager::CancelPlacement()
{
    if (PreviewActor)
    {
        PreviewActor->Destroy();
        PreviewActor = nullptr;
    }

    bIsPlacing = false;
}

void ALevelEditorManager::DeleteAt(FGridPos Pos)
{
    if (!Grid || !Grid->IsInside(Pos)) return;

    // 删除Box
    if (ABoxActor* Box = Grid->GetBoxAt(Pos))
    {
        Box->Destroy();
        Grid->Boxes.Remove(Box);
        return;
    }

    // 删除Goal
    for (AGoalActor* Goal : Grid->Goals)
    {
        if (Goal && Goal->Pos == Pos)
        {
            Goal->Destroy();
            Grid->Goals.Remove(Goal);
            return;
        }
    }

    // 删除Wall
    for (AActor* Actor : TActorRange<AWallActor>(GetWorld()))
    {
        AWallActor* Wall = Cast<AWallActor>(Actor);
        if (Wall && Wall->Pos == Pos)
        {
            Wall->Destroy();
            Grid->Walls.Remove(Pos);
            return;
        }
    }

    // 删除Ice
    int Index = Pos.Y * Grid->Width + Pos.X;
    if (Grid->IceTiles.Contains(Pos))
    {
        Grid->IceTiles.Remove(Pos);

        if (Grid->GridData.IsValidIndex(Index))
        {
            Grid->GridData[Index] = ETileType::Empty;
        }
        
        for (AActor* Actor : TActorRange<AIceActor>(GetWorld()))
        {
            AIceActor* Ice = Cast<AIceActor>(Actor);
            if (Ice && Ice->Pos == Pos)
            {
                Ice->Destroy();
                break;
            }
        }

        return;
    }
}

// 点击Grid事件并返回坐标
FGridPos ALevelEditorManager::GetMouseGridPos()
{
    FHitResult Hit;
    PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    FVector Loc = Hit.Location;

    int32 X = FMath::RoundToInt(Loc.X / Grid->GridSize);
    int32 Y = FMath::RoundToInt(Loc.Y / Grid->GridSize);

    return FGridPos(X, Y);
}

void ALevelEditorManager::SpawnAt(FGridPos Pos)
{
    switch (CurrentType)
    {
    case EPlaceType::Wall:
        SpawnWall(Pos);
        break;

    case EPlaceType::Box:
        SpawnBox(Pos);
        break;

    case EPlaceType::Goal:
        SpawnGoal(Pos);
        break;

    case EPlaceType::Player:
        SetPlayerStart(Pos);
        break;

    case EPlaceType::Ice:
        SpawnIce(Pos);
        break;
    }
}

void ALevelEditorManager::SpawnBox(FGridPos Pos)
{
    if (!BoxClass) return;
    UE_LOG(LogTemp, Warning, TEXT("Spawn Box!"));

    FVector Loc = Grid->GridToWorld(Pos);

    ABoxActor* Box = GetWorld()->SpawnActor<ABoxActor>(BoxClass, Loc, FRotator::ZeroRotator);
    if (Box)
    {
        Box->Pos = Pos;              // 更新坐标
        Grid->Boxes.Add(Box);        // 注册
    }
}

void ALevelEditorManager::SpawnGoal(FGridPos Pos)
{
    if (!GoalClass) return;
    UE_LOG(LogTemp, Warning, TEXT("Spawn Goal!"));

    FVector Loc = Grid->GridToWorld(Pos);

    AGoalActor* Goal = GetWorld()->SpawnActor<AGoalActor>(GoalClass, Loc, FRotator::ZeroRotator);
    if (Goal)
    {
        Goal->Pos = Pos;              // 更新坐标
        Grid->Goals.Add(Goal);        // 注册
    }
}

void ALevelEditorManager::SpawnWall(FGridPos Pos)
{
    if (!WallClass) return;
    UE_LOG(LogTemp, Warning, TEXT("Spawn Wall!"));

    FVector Loc = Grid->GridToWorld(Pos);

    AWallActor* Wall = GetWorld()->SpawnActor<AWallActor>(WallClass, Loc, FRotator::ZeroRotator);

    if (Wall)
    {
        Wall->Pos = Pos;
        Grid->Walls.Add(Pos); // ⭐关键
    }
}

void ALevelEditorManager::SpawnIce(FGridPos Pos)
{
    if (!IceClass) return;

    FVector Loc = Grid->GridToWorld(Pos);

    AIceActor* Ice = GetWorld()->SpawnActor<AIceActor>(IceClass, Loc, FRotator::ZeroRotator);

    if (Ice)
    {
        Ice->Pos = Pos;

        int Index = Pos.Y * Grid->Width + Pos.X;

        if (Grid->GridData.IsValidIndex(Index))
        {
            Grid->GridData[Index] = ETileType::Ice;
        }
        Grid->IceTiles.Add(Pos);
    }
}

void ALevelEditorManager::SetPlayerStart(FGridPos Pos)
{
    APushBoxCharacter* Player = Cast<APushBoxCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

    if (Player)
    {
        FVector Loc = Grid->GridToWorld(Pos);

        Player->GetCharacterMovement()->StopMovementImmediately();
        Player->bIsMoving = false;
        Player->MoveAlpha = 0.f;

        //瞬移
        Player->SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);
		Player->SetCurrentPos(Pos);
    }
}
void ALevelEditorManager::StartPlacing(TSubclassOf<AActor> InClass)
{
    if (!Grid || !InClass) return;

    // 如果已经在放，先清掉旧的
    if (PreviewActor)
    {
        PreviewActor->Destroy();
        PreviewActor = nullptr;
    }

    bIsPlacing = true;

    // 生成预览
    PreviewActor = GetWorld()->SpawnActor<AActor>(
        InClass,
        FVector::ZeroVector,
        FRotator::ZeroRotator
    );

    if (PreviewActor)
    {
        PreviewActor->SetActorEnableCollision(false);
    }
}
bool ALevelEditorManager::CanPlace(FGridPos Pos)
{
    if (!Grid->IsInside(Pos)) return false;

    if (Grid->IsWallAt(Pos)) return false;

    if (Grid->GetBoxAt(Pos)) return false;

    if (Grid->IsGoal(Pos)) return false;

    if (Grid->IsIce(Pos)) return false;

    APushBoxCharacter* Player =
        Cast<APushBoxCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

    if (Player && Player->GetCurrentPos() == Pos)
    {
        return false;
    }

    return true;
}
// 保存关卡
void ALevelEditorManager::SaveLevel()
{
    FLevelData Data;

    // 收集箱子
    TArray<AActor*> Boxes;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABoxActor::StaticClass(), Boxes);

    for (AActor* Actor : Boxes)
    {
        ABoxActor* Box = Cast<ABoxActor>(Actor);
        if (Box)
        {
            Data.Boxes.Add(Box->Pos);
        }
    }

    // 收集目标点
    TArray<AActor*> Goals;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGoalActor::StaticClass(), Goals);

    for (AActor* Actor : Goals)
    {
        AGoalActor* Goal = Cast<AGoalActor>(Actor);
        if (Goal)
        {
            Data.Goals.Add(Goal->Pos);
        }
    }

    // 收集箱子
    TArray<AActor*> Walls;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWallActor::StaticClass(), Walls);

    for (AActor* Actor : Walls)
    {
        AWallActor* Wall = Cast<AWallActor>(Actor);
        if (Wall)
        {
            Data.Walls.Add(Wall->Pos);
        }
    }

    // 收集冰
    TArray<AActor*> Ices;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIceActor::StaticClass(), Ices);

    for (AActor* Actor : Ices)
    {
        AIceActor* Ice = Cast<AIceActor>(Actor);
        if (Ice)
        {
            Data.Ice.Add(Ice->Pos);
        }
    }

    // 保存出生点
    APushBoxCharacter* Player = Cast<APushBoxCharacter>(
        UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)
    );

    if (Player)
    {
        Data.PlayerStart = Player->GetCurrentPos();
    }

    FString Json;
    FJsonObjectConverter::UStructToJsonObjectString(Data, Json);

    FString FileName = FString::Printf(TEXT("SavedLevel_%d.json"), CurrentLevel);

    FString Path = FPaths::ProjectDir() + FileName;

    FFileHelper::SaveStringToFile(Json, *Path);

    UE_LOG(LogTemp, Warning, TEXT("Saved Level %d! Path: %s"), CurrentLevel, *Path);
}
// 加载关卡
void ALevelEditorManager::LoadLevel()
{
    FString Path = FPaths::ProjectDir() + FString::Printf(TEXT("SavedLevel_%d.json"), CurrentLevel);

    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *Path)) return;

    FLevelData Data;
    FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Data, 0, 0);

    ClearLevel();

    for (auto& Pos : Data.Ice)
    {
        SpawnIce(Pos);
    }

    for (auto& Pos : Data.Boxes)
    {
        SpawnBox(Pos);
    }

    for (auto& Pos : Data.Goals)
    {
        SpawnGoal(Pos);
    }

    for (auto& Pos : Data.Walls)
    {
        SpawnWall(Pos);
    }

    SetPlayerStart(Data.PlayerStart);

    UE_LOG(LogTemp, Warning, TEXT("Loaded!"));
}
// 删除当前关卡所有目标
void ALevelEditorManager::ClearLevel()
{
    TArray<AActor*> Actors;
    // Box
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABoxActor::StaticClass(), Actors);
    for (AActor* A : Actors) A->Destroy();

    // Goal
    Actors.Empty();
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGoalActor::StaticClass(), Actors);
    for (AActor* A : Actors) A->Destroy();

    // Wall
    Actors.Empty();
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWallActor::StaticClass(), Actors);
    for (AActor* A : Actors) A->Destroy();

    // Ice
    Actors.Empty();
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIceActor::StaticClass(), Actors);
    for (AActor* A : Actors) A->Destroy();

    // Grid数据也要清
    Grid->Boxes.Empty();
    Grid->Goals.Empty();
    Grid->Walls.Empty();
    Grid->IceTiles.Empty();
    for (int i = 0; i < Grid->GridData.Num(); i++)
    {
        Grid->GridData[i] = ETileType::Empty;
    }

    UE_LOG(LogTemp, Warning, TEXT("Cleared!"));
}

void ALevelEditorManager::NextLevel()
{
    CurrentLevel++;

    if (CurrentLevel > MaxLevel)
    {
        UE_LOG(LogTemp, Warning, TEXT("ALL LEVEL COMPLETE!"));
        return;
    }

    LoadLevel();
}
