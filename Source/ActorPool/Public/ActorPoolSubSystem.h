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

    AActor * GetAvailableInstance( const UObject * world_context );
    bool ReturnActor( AActor * actor );
    void DestroyActors();
    void DestroyUnusedInstances();

private:
    void DisableActor( AActor * actor ) const;
    AActor * SpawnActorAndAddToInstances( UWorld * world );

    UPROPERTY()
    TArray< AActor * > Instances;

    int AvailableInstanceIndex;
    FActorPoolInfos PoolInfos;
};

UCLASS()
class ACTORPOOL_API UActorPoolSubSystem final : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UActorPoolSubSystem();

    void Initialize( FSubsystemCollectionBase & collection ) override;
    void Deinitialize() override;

    UFUNCTION( BlueprintPure )
    bool IsActorPoolable( AActor * actor ) const;

    UFUNCTION( BlueprintPure )
    bool IsActorClassPoolable( TSubclassOf< AActor > actor_class ) const;

    UFUNCTION( BlueprintCallable, meta = ( WorldContext = "world_context", DeterminesOutputType = "actor_class" ) )
    AActor * GetActorFromPool( UObject * world_context, TSubclassOf< AActor > actor_class );

    template < typename _ACTOR_CLASS_ >
    _ACTOR_CLASS_ * GetActorFromPool( UObject * world_context, const TSubclassOf< AActor > actor_class )
    {
        return Cast< _ACTOR_CLASS_ >( GetActorFromPool( world_context, actor_class ) );
    }

    UFUNCTION( BlueprintCallable, DisplayName = "GetActorFromPool - WithTransform", meta = ( WorldContext = "world_context", DeterminesOutputType = "actor_class" ) )
    AActor * GetActorFromPoolWithTransform( UObject * world_context, TSubclassOf< AActor > actor_class, FTransform transform );

    template < typename _ACTOR_CLASS_ >
    _ACTOR_CLASS_ * GetActorFromPoolWithTransform( UObject * world_context, const TSubclassOf< AActor > actor_class, const FTransform & transform )
    {
        return Cast< _ACTOR_CLASS_ >( GetActorFromPoolWithTransform( world_context, actor_class, transform ) );
    }

    UFUNCTION( BlueprintCallable )
    bool ReturnActorToPool( AActor * actor );

    static void DestroyUnusedInstancesInPools( UWorld * world );

private:
    void OnWorldBeginPlay();
    FActorPoolInstances CreateActorPoolInstance( const FActorPoolInfos & pool_infos ) const;

    UPROPERTY()
    TMap< TSubclassOf< AActor >, FActorPoolInstances > ActorPools;
};
