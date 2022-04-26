#include "ActorPoolSubSystem.h"

#include "ActorPoolActor.h"

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
    if ( !ensureMsgf( ActorPoolActor != nullptr, TEXT( "%s - ActorPoolActor is not valid!" ), TEXT( __FUNCTION__ ) ) )
    {
        return false;
    }

    return ActorPoolActor->IsActorClassPoolable( actor_class );
}

AActor * UActorPoolSubSystem::GetActorFromPool( const TSubclassOf< AActor > actor_class )
{
    if ( !ensureMsgf( ActorPoolActor != nullptr, TEXT( "%s - ActorPoolActor is not valid!" ), TEXT( __FUNCTION__ ) ) )
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
    if ( !ensureMsgf( ActorPoolActor != nullptr, TEXT( "%s - ActorPoolActor is not valid!" ), TEXT( __FUNCTION__ ) ) )
    {
        return false;
    }

    return ActorPoolActor->ReturnActorToPool( actor );
}

void UActorPoolSubSystem::RegisterActorPoolActor( AActorPoolActor * actor_pool_actor )
{
    ActorPoolActor = actor_pool_actor;
    ensureAlwaysMsgf( ActorPoolActor != nullptr, TEXT( "Actor Pool Actor is not valid!" ) );
}

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
void UActorPoolSubSystem::DestroyUnusedInstancesInPools()
{
    if ( !ensureMsgf( ActorPoolActor != nullptr, TEXT( "%s - ActorPoolActor is not valid!" ), TEXT( __FUNCTION__ ) ) )
    {
        return;
    }

    ActorPoolActor->DestroyUnusedInstancesInPools();
}

void UActorPoolSubSystem::DumpPoolInfos( FOutputDevice & output_device ) const
{
    if ( !ensureMsgf( ActorPoolActor != nullptr, TEXT( "%s - ActorPoolActor is not valid!" ), TEXT( __FUNCTION__ ) ) )
    {
        return;
    }

    ActorPoolActor->DumpPoolInfos( output_device );
}
#endif
