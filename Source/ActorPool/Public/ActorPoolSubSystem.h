#pragma once

#include "ActorPoolSettings.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h"

#include <CoreMinimal.h>
#include <Subsystems/WorldSubsystem.h>

#include "ActorPoolSubSystem.generated.h"

class UGameInstance;
class AActor;
struct FActorPoolInfos;
class AGameModeBase;
struct FWorldContext;

USTRUCT()
struct FActorPoolInstances
{
    GENERATED_USTRUCT_BODY()

public:

    FActorPoolInstances();
    FActorPoolInstances( UWorld * world, const FActorPoolInfos & pool_infos );

    AActor * GetAvailableInstance();
    void ReturnActor( AActor * actor );
    void DestroyActors();

private:

    void DisableActor( AActor * actor ) const;

    UPROPERTY()
    TArray< AActor * > Instances;

    int AvailableInstanceIndex;
    FAPPooledActorAcquireFromPoolSettings AcquireFromPoolSettings;
};

UCLASS()
class ACTORPOOL_API UActorPoolSubSystem final : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UActorPoolSubSystem();

    void Initialize( FSubsystemCollectionBase & collection ) override;
    void Deinitialize() override;

    UFUNCTION( BlueprintCallable )
    AActor * GetActorFromPool( TSubclassOf< AActor > actor_class );

    UFUNCTION( BlueprintCallable, DisplayName = "GetActorFromPool - WithTransform" )
    AActor * GetActorFromPoolWithTransform( TSubclassOf< AActor > actor_class, FTransform transform );

    UFUNCTION( BlueprintCallable )
    void ReturnActorToPool( AActor * actor );

private:

    void OnGameModeInitialized( AGameModeBase * game_mode );
    FActorPoolInstances CreateActorPoolInstance( const FActorPoolInfos & pool_infos ) const;

    UPROPERTY()
    TMap< TSubclassOf< AActor >, FActorPoolInstances > ActorPools;

    FDelegateHandle GameModeInitializedEventDelegateHandle;
};
