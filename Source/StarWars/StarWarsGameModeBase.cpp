// Copyright Epic Games, Inc. All Rights Reserved.

#include "StarWarsGameModeBase.h"
#include "OutlawCharacter.h"

AStarWarsGameModeBase::AStarWarsGameModeBase()
{
	DefaultPawnClass = AOutlawCharacter::StaticClass();
}
