#pragma once

#include <CoreMinimal.h>
#include <Subsystems/WorldSubsystem.h>

#include "ActorPoolSubSystem.generated.h"

class AActorPoolActor;

UCLASS()
class ACTORPOOL_API UActorPoolSubSystem final : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintPure )
    bool IsActorPoolable( AActor * actor ) const;

    UFUNCTION( BlueprintPure )
    bool IsActorClassPoolable( TSubclassOf< AActor > actor_class ) const;

    UFUNCTION( BlueprintCallable, meta = ( DeterminesOutputType = "actor_class" ) )
    AActor * GetActorFromPool( TSubclassOf< AActor > actor_class );

    template < typename _ACTOR_CLASS_ >
    _ACTOR_CLASS_ * GetActorFromPool( const TSubclassOf< AActor > actor_class )
    {
        return Cast< _ACTOR_CLASS_ >( GetActorFromPool( actor_class ) );
    }

    UFUNCTION( BlueprintCallable, DisplayName = "GetActorFromPool - WithTransform", meta = ( DeterminesOutputType = "actor_class" ) )
    AActor * GetActorFromPoolWithTransform( TSubclassOf< AActor > actor_class, FTransform transform );

    template < typename _ACTOR_CLASS_ >
    _ACTOR_CLASS_ * GetActorFromPoolWithTransform( const TSubclassOf< AActor > actor_class, const FTransform & transform )
    {
        return Cast< _ACTOR_CLASS_ >( GetActorFromPoolWithTransform( actor_class, transform ) );
    }

    UFUNCTION( BlueprintCallable )
    bool ReturnActorToPool( AActor * actor );

    void RegisterActorPoolActor( AActorPoolActor * actor_pool_actor );

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
    void DestroyUnusedInstancesInPools();
    void DumpPoolInfos( FOutputDevice & output_device ) const;
#endif

private:
    UPROPERTY()
    AActorPoolActor * ActorPoolActor;
};
