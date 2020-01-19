// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "SG2SaveGame.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct SAVEGAME2_API FComponentRecord
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(BlueprintReadWrite)
		UClass* Class;

	UPROPERTY(BlueprintReadWrite)
		FName Name;

	UPROPERTY(BlueprintReadWrite)
		TArray<uint8> Data;

	//only for USceneComponents
	UPROPERTY(BlueprintReadWrite)
		FTransform Transform;

	//only for primitive and movement components
	UPROPERTY(BlueprintReadWrite)
		FVector Velocity;

	//only for primitive components
	UPROPERTY(BlueprintReadWrite)
		FVector AngularVelocity;
};

USTRUCT(BlueprintType)
struct SAVEGAME2_API FActorRecord
{
	GENERATED_USTRUCT_BODY()

		AActor* SpawnedActor;

	UPROPERTY(BlueprintReadWrite)
		UClass* Class;

	UPROPERTY(BlueprintReadWrite)
		FString Level;

	UPROPERTY(BlueprintReadWrite)
		FName Name;

	UPROPERTY(BlueprintReadWrite)
		TArray<uint8> Data;

	UPROPERTY(BlueprintReadWrite)
		TArray<FComponentRecord> Components;

	UPROPERTY(BlueprintReadWrite)
		FTransform Transform;

	UPROPERTY(BlueprintReadWrite)
		float Lifespan;

	//UPROPERTY(BlueprintReadWrite)
	//FVector AIDestination;
};

class SAVEGAME2_API FSaveGameArchive : public FObjectAndNameAsStringProxyArchive
{
public:
	FSaveGameArchive(FArchive & InInnerArchive)
		: FObjectAndNameAsStringProxyArchive(InInnerArchive, true)
	{
		ArIsSaveGame = true;
	}
};

UCLASS()
class SAVEGAME2_API USG2SaveGame : public USaveGame
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite)
		FRotator PlayerControllerRotator;

	UPROPERTY(BlueprintReadWrite)
		FActorRecord Player;

	UPROPERTY(BlueprintReadWrite)
		TArray<FActorRecord> Actors;

	UFUNCTION(BlueprintCallable)
		void SaveGame(APawn* PlayerActor, UPARAM(ref) TArray<AActor*>& OtherActors);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
		void LoadGame(UObject* WorldContextObject);
};
