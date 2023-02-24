#include "ActorPoolSubSystem.h"

#include "APPooledActorInterface.h"
#include "ActorPoolActor.h"

#include <Engine/World.h>

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
static FAutoConsoleCommandWithWorld GActorPoolDestroyInstancesInPools(
    TEXT( "ActorPool.DestroyUnusedInstancesInPools" ),
    TEXT( "Destroys all actors in the pools which have not been acquired." ),
    FConsoleCommandWithWorldDelegate::CreateLambda( []( const UWorld * world ) {
        if ( auto * system = world->GetSubsystem< UActorPoolSubSystem >() )
        {
            system->DestroyUnusedInstancesInPools();
        }
    } ),
    ECVF_Default );

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GActorPoolDumpPoolInfos(
    TEXT( "ActorPool.DumpPoolInfos" ),
    TEXT( "Dumps infos about the pools." ),
    FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda( []( const TArray< FString > & /*args*/, const UWorld * world, FOutputDevice & output_device ) {
        if ( const auto * system = world->GetSubsystem< UActorPoolSubSystem >() )
        {
            system->DumpPoolInfos( output_device );
        }
    } ),
    ECVF_Default );
#endif

bool UActorPoolSubSystem::IsActorPoolable( AActor * actor ) const
{
    if ( actor == nullptr )
    {
        return false;
    }

    return IsActorClassPoolable( actor->GetClass() );
}

bool UActorPoolSubSystem::IsActorClassPoolable( const TSubclassOf< AActor > actor_class ) const
{
    if ( !ensureMsgf( ActorPoolActor != nullptr, TEXT( "%s - ActorPoolActor is not valid!" ), StringCast< TCHAR >( __FUNCTION__ ).Get() ) )
    {
        return false;
    }

    return ActorPoolActor->IsActorClassPoolable( actor_class );
}

FActorPoolRequestHandle UActorPoolSubSystem::GetActorFromPool( TSubclassOf< AActor > actor_class, FAPOnActorGotFromPoolDynamicDelegate on_actor_got_from_pool )
{
    return GetActorFromPoolWithTransform( actor_class, FTransform(), on_actor_got_from_pool );
}

FActorPoolRequestHandle UActorPoolSubSystem::GetActorFromPoolWithTransform( TSubclassOf< AActor > actor_class, FTransform transform, FAPOnActorGotFromPoolDynamicDelegate on_actor_got_from_pool )
{
    if ( !ensureMsgf( ActorPoolActor != nullptr, TEXT( "%s - ActorPoolActor is not valid!" ), StringCast< TCHAR >( __FUNCTION__ ).Get() ) )
    {
        on_actor_got_from_pool.ExecuteIfBound( nullptr );
        return FActorPoolRequestHandle();
    }

    auto * actor = ActorPoolActor->GetActorFromPool( actor_class );

    if ( actor != nullptr )
    {
        if ( Cast< IAPPooledActorInterface >( actor ) )
        {
            if ( IAPPooledActorInterface::Execute_IsUsingDeferredAcquisitionFromPool( actor ) )
            {
                const auto & request = PendingActorRequests.Emplace_GetRef( on_actor_got_from_pool, actor, transform );
                IAPPooledActorInterface::Execute_OnAquiredFromPoolDeferred( actor, request.Handle );
                return request.Handle;
            }
        }
    }

    actor->SetActorTransform( transform );
    on_actor_got_from_pool.ExecuteIfBound( actor );
    return FActorPoolRequestHandle();
}

bool UActorPoolSubSystem::ReturnActorToPool( AActor * actor )
{
    if ( ActorPoolActor == nullptr )
    {
        return false;
    }

    return ActorPoolActor->ReturnActorToPool( actor );
}

bool UActorPoolSubSystem::FinishAcquireActor( FActorPoolRequestHandle handle )
{
    if ( !handle.IsValid() )
    {
        return false;
    }

    for ( auto index = 0; index < PendingActorRequests.Num(); ++index )
    {
        const auto & request = PendingActorRequests[ index ];
        if ( request.Handle == handle )
        {
            request.Actor->SetActorTransform( request.Transform );
            request.Callback.ExecuteIfBound( request.Actor.Get() );
            PendingActorRequests.RemoveAt( index );
            return true;
        }
    }

    return false;
}

void UActorPoolSubSystem::RegisterActorPoolActor( AActorPoolActor * actor_pool_actor )
{
    if ( !ensureAlwaysMsgf( actor_pool_actor != nullptr, TEXT( "Actor Pool Actor is not valid!" ) ) )
    {
        return;
    }

    if ( !ensureAlwaysMsgf( ActorPoolActor == nullptr, TEXT( "The ActorPoolActor is already set!" ) ) )
    {
        return;
    }

    ActorPoolActor = actor_pool_actor;
    OnActorPoolReadyEvent.Broadcast();
}

void UActorPoolSubSystem::OnActorPoolReady_RegisterAndCall( FSimpleMulticastDelegate::FDelegate delegate )
{
    if ( !OnActorPoolReadyEvent.IsBoundToObject( delegate.GetUObject() ) )
    {
        OnActorPoolReadyEvent.Add( delegate );
    }

    if ( IsActorPoolReady() )
    {
        delegate.Execute();
    }
}

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
void UActorPoolSubSystem::DestroyUnusedInstancesInPools()
{
    if ( !ensureMsgf( ActorPoolActor != nullptr, TEXT( "%s - ActorPoolActor is not valid!" ), StringCast< TCHAR >( __FUNCTION__ ).Get() ) )
    {
        return;
    }

    ActorPoolActor->DestroyUnusedInstancesInPools();
}

void UActorPoolSubSystem::DumpPoolInfos( FOutputDevice & output_device ) const
{
    if ( !ensureMsgf( ActorPoolActor != nullptr, TEXT( "%s - ActorPoolActor is not valid!" ), StringCast< TCHAR >( __FUNCTION__ ).Get() ) )
    {
        return;
    }

    ActorPoolActor->DumpPoolInfos( output_device );
}
#endif
