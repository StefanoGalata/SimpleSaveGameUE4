// Fill out your copyright notice in the Description page of Project Settings.

#include "SG2SaveGame.h"
#include "Archive.h"
#include "BufferArchive.h"
#include "MemoryReader.h"

#include "GameFramework/MovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"

void CreateActorRecord(AActor* Actor, FActorRecord& Record) {
	UE_LOG(LogTemp, Display, TEXT("Saving Actor: %s"), *Actor->GetName());

	FMemoryWriter MemoryWriter = FMemoryWriter(Record.Data, true);
	FSaveGameArchive Archive = FSaveGameArchive(MemoryWriter);

	//save actor
	Record.Class = Actor->GetClass();
	Record.Name = Actor->GetFName();
	Record.Transform = Actor->GetTransform();
	Record.Level = Actor->GetLevel()->GetPathName();
	Record.Lifespan = Actor->GetLifeSpan();
	Actor->Serialize(Archive);

	//save components
	TArray<UActorComponent*> Components = TArray<UActorComponent*>();
	Actor->GetComponents(Components);
	for (auto Component : Components) {
		int idx = Record.Components.Emplace();
		FComponentRecord& ComponentRecord = Record.Components[idx];

		ComponentRecord.Class = Component->GetClass();
		ComponentRecord.Name = Component->GetFName();

		auto SceneComponent = Cast<USceneComponent>(Component);
		if (SceneComponent != nullptr) {
			ComponentRecord.Transform = SceneComponent->GetRelativeTransform();
			auto PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
			if (PrimitiveComponent != nullptr) {
				ComponentRecord.Velocity = PrimitiveComponent->GetPhysicsLinearVelocity();
				ComponentRecord.AngularVelocity = PrimitiveComponent->GetPhysicsAngularVelocityInDegrees();
			}
		}

		auto MoveComponent = Cast<UMovementComponent>(Component);
		if (MoveComponent != nullptr) {
			ComponentRecord.Velocity = MoveComponent->Velocity;
		}

		FMemoryWriter MemoryWriter = FMemoryWriter(ComponentRecord.Data, true);
		FSaveGameArchive ComponentArchive = FSaveGameArchive(MemoryWriter);
		Component->Serialize(ComponentArchive);
	}
}

void LoadActor(UWorld* World, FActorRecord& Record) {
	UE_LOG(LogTemp, Display, TEXT("Loading Actor: %s"), *Record.Name.ToString());

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = Record.Name;
	SpawnParams.bDeferConstruction = true;
	SpawnParams.bNoFail = true;
	ULevel* LevelThatActorIsPartOf = nullptr;

	//This prevents to destroy actors when we have actors that have the same ID name across multiple level
	for (ULevel* Level : World->GetLevels()) {
		UE_LOG(LogTemp, Warning, TEXT("Found Levels: %s"), *(Level->GetPathName()))
		if (Record.Level == Level->GetPathName())
			LevelThatActorIsPartOf = Level;
	}

	SpawnParams.OverrideLevel = LevelThatActorIsPartOf;//if nullptr default is PersistentLevel;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const auto &Actor = World->SpawnActor(Record.Class, &Record.Transform, SpawnParams);
	Actor->SetLifeSpan(Record.Lifespan);

	FMemoryReader Reader = FMemoryReader(Record.Data, true);
	FSaveGameArchive Archive = FSaveGameArchive(Reader);
	Actor->Serialize(Archive);

	Record.SpawnedActor = Actor; //save it for FinishLoadActor
}

void FinishLoadActor(FActorRecord& Record) {
	AActor* Actor = Record.SpawnedActor;

	TArray<UActorComponent*> Components = TArray<UActorComponent*>();
	Actor->GetComponents(Components);
	for (auto& ComponentRecord : Record.Components) {
		for (auto Component : Components) {
			if (Component->GetFName() == ComponentRecord.Name) {
				FMemoryReader CompReader = FMemoryReader(ComponentRecord.Data, true);
				FSaveGameArchive ComponentArchive = FSaveGameArchive(CompReader);
				Component->Serialize(ComponentArchive);

				auto SceneComponent = Cast<USceneComponent>(Component);
				if (SceneComponent != nullptr) {
					SceneComponent->SetRelativeTransform(ComponentRecord.Transform, false, nullptr, ETeleportType::TeleportPhysics);
					auto PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
					if (PrimitiveComponent != nullptr && PrimitiveComponent->IsAnySimulatingPhysics()) {
						PrimitiveComponent->SetPhysicsLinearVelocity(ComponentRecord.Velocity);
						PrimitiveComponent->SetPhysicsAngularVelocityInDegrees(ComponentRecord.AngularVelocity);
						PrimitiveComponent->GetBodyInstance()->UpdateMassProperties(); //if you changed the scale of actor, mass needs to be updated here
					}
				}
				auto MoveComponent = Cast<UMovementComponent>(Component);
				if (MoveComponent != nullptr) {
					MoveComponent->Velocity = ComponentRecord.Velocity;
					MoveComponent->UpdateComponentVelocity(); //don't know i needed this. sounds reasonable though. B)
				}
				break;
			}
		}
	}

	Actor->FinishSpawning(Record.Transform);
}

void USG2SaveGame::SaveGame(APawn* PlayerActor, UPARAM(ref)TArray<AActor*>& OtherActors) {
	check(PlayerActor != nullptr);
	Actors.Empty(); //this should probably be a brand new USG2SaveGame instance, so should be empty anyway.
	
	CreateActorRecord(PlayerActor, Player);
	PlayerControllerRotator = PlayerActor->GetController()->GetControlRotation();

	for (auto Actor : OtherActors) {
		int idx = Actors.Emplace();
		FActorRecord& Record = Actors[idx];
		CreateActorRecord(Actor, Record);
	}
}

void USG2SaveGame::LoadGame(UObject* WorldContextObject) {
	auto World = WorldContextObject->GetWorld();

	UGameplayStatics::GetPlayerCharacter(World, 0)->Destroy();

	TArray<AActor*> CurrentActors;
	UGameplayStatics::GetAllActorsWithTag(World, FName("Save"), CurrentActors);
	for (auto actor : CurrentActors) {
		actor->Destroy();
	}

	LoadActor(World, Player);
	for (auto& Record : Actors) {
		LoadActor(World, Record);
		FinishLoadActor(Record);
	}

	FinishLoadActor(Player);
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	PlayerController->Possess(Cast<APawn>(Player.SpawnedActor));
	PlayerController->SetControlRotation(PlayerControllerRotator);
	PlayerController->UpdateRotation(123456); //delta time passed is nonsense
}