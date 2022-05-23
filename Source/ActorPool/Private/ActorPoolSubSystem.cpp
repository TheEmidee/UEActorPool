#include "ActorPoolSubSystem.h"

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

AActor * UActorPoolSubSystem::GetActorFromPool( const TSubclassOf< AActor > actor_class )
{
    if ( !ensureMsgf( ActorPoolActor != nullptr, TEXT( "%s - ActorPoolActor is not valid!" ), StringCast< TCHAR >( __FUNCTION__ ).Get() ) )
    {
        return nullptr;
    }

    return ActorPoolActor->GetActorFromPool( actor_class );
}

AActor * UActorPoolSubSystem::GetActorFromPoolWithTransform( const TSubclassOf< AActor > actor_class, const FTransform transform )
{
    if ( auto * result = GetActorFromPool( actor_class ) )
    {
        result->SetActorTransform( transform, false, nullptr, ETeleportType::ResetPhysics );
        return result;
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

void UActorPoolSubSystem::RegisterActorPoolActor( AActorPoolActor * actor_pool_actor )
{
    ActorPoolActor = actor_pool_actor;

    if ( ensureAlwaysMsgf( ActorPoolActor != nullptr, TEXT( "Actor Pool Actor is not valid!" ) ) )
    {
        OnActorPoolReadyEvent.Broadcast();
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
