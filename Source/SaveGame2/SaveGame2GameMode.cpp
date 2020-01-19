// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SaveGame2GameMode.h"
#include "SaveGame2Character.h"
#include "UObject/ConstructorHelpers.h"

ASaveGame2GameMode::ASaveGame2GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
