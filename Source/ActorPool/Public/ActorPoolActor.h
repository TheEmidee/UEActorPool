#pragma once

#include "ActorPoolSettings.h"

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>

#include "ActorPoolActor.generated.h"

struct FActorPoolInfos;

USTRUCT()
struct FActorPoolInstances
{
    GENERATED_USTRUCT_BODY()

public:
    FActorPoolInstances();
    FActorPoolInstances( UWorld * world, const FActorPoolInfos & pool_infos );

    AActor * GetAvailableInstance( UWorld * world );
    bool ReturnActor( AActor * actor );
    void DestroyActors();
    void DestroyUnusedInstances();

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
    void DumpPoolInfos( FOutputDevice & output_device ) const;
#endif

private:
    void DisableActor( AActor * actor ) const;
    AActor * SpawnActorAndAddToInstances( UWorld * world );

    UPROPERTY()
    TArray< AActor * > Instances;

    int AvailableInstanceIndex;
    FActorPoolInfos PoolInfos;
};

UCLASS()
class ACTORPOOL_API AActorPoolActor : public AActor
{
    GENERATED_BODY()

public:
    AActorPoolActor();

    void BeginPlay() override;
    void EndPlay( const EEndPlayReason::Type end_play_reason ) override;

    bool IsActorClassPoolable( TSubclassOf< AActor > actor_class ) const;

    AActor * GetActorFromPool( TSubclassOf< AActor > actor_class );

    bool ReturnActorToPool( AActor * actor );

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
    void DestroyUnusedInstancesInPools();
    void DumpPoolInfos( FOutputDevice & output_device ) const;
#endif

private:
    FActorPoolInstances CreateActorPoolInstance( const FActorPoolInfos & pool_infos ) const;

    UPROPERTY()
    TMap< TSubclassOf< AActor >, FActorPoolInstances > ActorPools;
};
