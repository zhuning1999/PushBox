// Copyright Epic Games, Inc. All Rights Reserved.

#include "PushBoxGameMode.h"
#include "PushBoxCharacter.h"
#include "UObject/ConstructorHelpers.h"

APushBoxGameMode::APushBoxGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
