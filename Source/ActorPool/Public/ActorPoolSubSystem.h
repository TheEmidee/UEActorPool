#pragma once

#include "ActorPoolActor.h"

#include <CoreMinimal.h>
#include <Subsystems/WorldSubsystem.h>

#include "ActorPoolSubSystem.generated.h"

DECLARE_DELEGATE_OneParam( FAPOnActorPoolReadyEvent, AActorPoolActor * actor_pool_actor );

UCLASS()
class ACTORPOOL_API UActorPoolSubSystem final : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintPure )
    bool IsActorPoolable( AActor * actor ) const;

    UFUNCTION( BlueprintPure )
    bool IsActorClassPoolable( TSubclassOf< AActor > actor_class ) const;

    AActor * GetActorFromPool( FActorPoolRequestHandle & request_handle, TSubclassOf< AActor > actor_class );
    AActor * GetActorFromPoolWithTransform( FActorPoolRequestHandle & request_handle, TSubclassOf< AActor > actor_class, FTransform transform );

    UFUNCTION( BlueprintCallable, DisplayName = "GetActorFromPool", meta = ( DeterminesOutputType = "actor_class" ) )
    AActor * K2_GetActorFromPool( FActorPoolRequestHandle & request_handle, TSubclassOf< AActor > actor_class );

    UFUNCTION( BlueprintCallable, DisplayName = "GetActorFromPool - WithTransform", meta = ( DeterminesOutputType = "actor_class" ) )
    AActor * K2_GetActorFromPoolWithTransform( FActorPoolRequestHandle & request_handle, TSubclassOf< AActor > actor_class, FTransform transform );

    // Gets an actor from the pool and returns it immediately.
    // Use this function only when you are sure that the actor you acquire does not have a delayed initialization and does not call FinishAcquireActor
    UFUNCTION( BlueprintCallable, DisplayName = "GetActorFromPool - WithTransform - NoDeferred", meta = ( DeterminesOutputType = "actor_class" ) )
    AActor * GetActorFromPoolWithTransformNoDeferred( TSubclassOf< AActor > actor_class, FTransform transform );

    UFUNCTION( BlueprintCallable )
    bool ReturnActorToPool( AActor * actor );

    UFUNCTION( BlueprintCallable )
    bool FinishAcquireActor( FActorPoolRequestHandle handle );

    void RegisterActorPoolActor( AActorPoolActor * actor_pool_actor );
    bool IsActorPoolReady() const;
    void OnActorPoolReady_RegisterAndCall( FAPOnActorPoolReadyEvent delegate );
    void RegisterPooledActor( const FActorPoolInfos & actor_pool_infos );
    void UnRegisterPooledActor( const FActorPoolInfos & actor_pool_infos );

#if !UE_BUILD_SHIPPING
    void DestroyUnusedInstancesInPools();
    void DumpPoolInfos( FOutputDevice & output_device ) const;
#endif

private:
    struct PendingActorRequest
    {
        PendingActorRequest( AActor * actor, const FTransform & transform ) :
            Actor( actor ),
            Transform( transform ),
            Handle( FActorPoolRequestHandle ::GenerateNewHandle() )
        {}

        TWeakObjectPtr< AActor > Actor;
        FTransform Transform;
        FActorPoolRequestHandle Handle;
    };

    void BroadcastOnActorPoolReadyEvent();

    UPROPERTY()
    AActorPoolActor * ActorPoolActor;

    TArray< FAPOnActorPoolReadyEvent > OnActorPoolReadyEvents;
    TArray< PendingActorRequest > PendingActorRequests;
};

FORCEINLINE bool UActorPoolSubSystem::IsActorPoolReady() const
{
    return ActorPoolActor != nullptr;
}
