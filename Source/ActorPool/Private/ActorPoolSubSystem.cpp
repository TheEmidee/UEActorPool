#include "ActorPoolSubSystem.h"

#include "APPooledActorInterface.h"
#include "ActorPoolActor.h"

#include <Engine/World.h>
#include <HAL/IConsoleManager.h>

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

AActor * UActorPoolSubSystem::GetActorFromPool( FActorPoolRequestHandle & request_handle, TSubclassOf< AActor > actor_class )
{
    return GetActorFromPoolWithTransform( request_handle, actor_class, FTransform::Identity );
}

AActor * UActorPoolSubSystem::GetActorFromPoolWithTransform( FActorPoolRequestHandle & request_handle, TSubclassOf< AActor > actor_class, FTransform transform )
{
    if ( !ensureMsgf( ActorPoolActor != nullptr, TEXT( "%s - ActorPoolActor is not valid!" ), StringCast< TCHAR >( __FUNCTION__ ).Get() ) )
    {
        request_handle = FActorPoolRequestHandle();
        return nullptr;
    }

    if ( auto * actor = GetActorFromPoolWithTransformNoDeferred( actor_class, transform ) )
    {
        if ( actor->Implements< UAPPooledActorInterface >() )
        {
            if ( IAPPooledActorInterface::Execute_IsUsingDeferredAcquisitionFromPool( actor ) )
            {
                const auto & request = PendingActorRequests.Emplace_GetRef( actor, transform );
                request_handle = request.Handle;
                return actor;
            }
        }
    }

    request_handle = FActorPoolRequestHandle();
    return nullptr;
}

AActor * UActorPoolSubSystem::K2_GetActorFromPool( FActorPoolRequestHandle & request_handle, TSubclassOf< AActor > actor_class )
{
    return GetActorFromPool( request_handle, actor_class );
}
AActor * UActorPoolSubSystem::K2_GetActorFromPoolWithTransform( FActorPoolRequestHandle & request_handle, TSubclassOf< AActor > actor_class, FTransform transform )
{
    return GetActorFromPoolWithTransform( request_handle, actor_class, transform );
}

AActor * UActorPoolSubSystem::GetActorFromPoolWithTransformNoDeferred( TSubclassOf< AActor > actor_class, FTransform transform )
{
    if ( !ensureMsgf( ActorPoolActor != nullptr, TEXT( "%s - ActorPoolActor is not valid!" ), StringCast< TCHAR >( __FUNCTION__ ).Get() ) )
    {
        return nullptr;
    }

    if ( auto * actor = ActorPoolActor->GetActorFromPool( actor_class ) )
    {
        actor->SetActorLocation( transform.GetLocation() );
        actor->SetActorRotation( transform.GetRotation() );
        return actor;
    }

    return nullptr;
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
            IAPPooledActorInterface::Execute_OnAquiredFromPoolDeferred( request.Actor.Get(), request.Handle );
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
    BroadcastOnActorPoolReadyEvent();
}

void UActorPoolSubSystem::OnActorPoolReady_RegisterAndCall( FAPOnActorPoolReadyEvent delegate )
{
    if ( IsActorPoolReady() )
    {
        delegate.ExecuteIfBound( ActorPoolActor );
    }
    else
    {
        OnActorPoolReadyEvents.Emplace( MoveTemp( delegate ) );
    }
}

void UActorPoolSubSystem::RegisterPooledActor( const FActorPoolInfos & actor_pool_infos )
{
    ensureAlways( ActorPoolActor != nullptr );
    ActorPoolActor->RegisterPooledActor( actor_pool_infos );
}

void UActorPoolSubSystem::UnRegisterPooledActor( const FActorPoolInfos & actor_pool_infos )
{
    ensureAlways( ActorPoolActor != nullptr );
    ActorPoolActor->UnRegisterPooledActor( actor_pool_infos );
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

void UActorPoolSubSystem::BroadcastOnActorPoolReadyEvent()
{
    for ( const auto & event : OnActorPoolReadyEvents )
    {
        event.ExecuteIfBound( ActorPoolActor );
    }
}