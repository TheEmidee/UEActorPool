#pragma once

#include "ActorPoolActor.h"

#include <CoreMinimal.h>
#include <Subsystems/WorldSubsystem.h>

#include "ActorPoolSubSystem.generated.h"

DECLARE_EVENT( UActorPoolSubSystem, FSWOnActorPoolReadyEvent )
DECLARE_DYNAMIC_DELEGATE_OneParam( FAPOnActorGotFromPoolDynamicDelegate, AActor *, Actor );

UCLASS()
class ACTORPOOL_API UActorPoolSubSystem final : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintPure )
    bool IsActorPoolable( AActor * actor ) const;

    UFUNCTION( BlueprintPure )
    bool IsActorClassPoolable( TSubclassOf< AActor > actor_class ) const;

    UFUNCTION( BlueprintCallable )
    FActorPoolRequestHandle GetActorFromPool( TSubclassOf< AActor > actor_class, FAPOnActorGotFromPoolDynamicDelegate on_actor_got_from_pool );

    UFUNCTION( BlueprintCallable, DisplayName = "GetActorFromPool - WithTransform" )
    FActorPoolRequestHandle GetActorFromPoolWithTransform( TSubclassOf< AActor > actor_class, FTransform transform, FAPOnActorGotFromPoolDynamicDelegate on_actor_got_from_pool );

    UFUNCTION( BlueprintCallable )
    bool ReturnActorToPool( AActor * actor );

    UFUNCTION( BlueprintCallable )
    bool FinishAcquireActor( FActorPoolRequestHandle handle );

    void RegisterActorPoolActor( AActorPoolActor * actor_pool_actor );
    bool IsActorPoolReady() const;
    void OnActorPoolReady_RegisterAndCall( FSimpleMulticastDelegate::FDelegate delegate );

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
    void DestroyUnusedInstancesInPools();
    void DumpPoolInfos( FOutputDevice & output_device ) const;
#endif

private:
    struct PendingActorRequest
    {
        PendingActorRequest( const FAPOnActorGotFromPoolDynamicDelegate & callback, AActor * actor, const FTransform & transform ) :
            Callback( callback ),
            Actor( actor ),
            Transform( transform ),
            Handle( FActorPoolRequestHandle ::GenerateNewHandle() )
        {}

        FAPOnActorGotFromPoolDynamicDelegate Callback;
        TWeakObjectPtr< AActor > Actor;
        FTransform Transform;
        FActorPoolRequestHandle Handle;
    };

    UPROPERTY()
    AActorPoolActor * ActorPoolActor;

    FSimpleMulticastDelegate OnActorPoolReadyEvent;
    TArray< PendingActorRequest > PendingActorRequests;
};

FORCEINLINE bool UActorPoolSubSystem::IsActorPoolReady() const
{
    return ActorPoolActor != nullptr;
}
