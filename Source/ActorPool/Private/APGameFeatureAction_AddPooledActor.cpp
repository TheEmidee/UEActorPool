#include "APGameFeatureAction_AddPooledActor.h"

#include "ActorPoolSubSystem.h"

#include <Kismet/KismetSystemLibrary.h>

void UAPGameFeatureAction_AddPooledActor::OnGameFeatureActivating( FGameFeatureActivatingContext & context )
{
    GameInstanceStartHandles.FindOrAdd( context ) = FWorldDelegates::OnStartGameInstance.AddUObject( this, &ThisClass::HandleGameInstanceStart, FGameFeatureStateChangeContext( context ) );

    for ( const auto & world_context : GEngine->GetWorldContexts() )
    {
        if ( context.ShouldApplyToWorldContext( world_context ) )
        {
            UnregisterPooledActors( world_context );
            AddToWorld( world_context, context );
        }
    }
}

void UAPGameFeatureAction_AddPooledActor::OnGameFeatureDeactivating( FGameFeatureDeactivatingContext & context )
{
    if ( const auto * found_handle = GameInstanceStartHandles.Find( context ); ensure( found_handle ) )
    {
        FWorldDelegates::OnStartGameInstance.Remove( *found_handle );
    }

    for ( const auto & world_context : GEngine->GetWorldContexts() )
    {
        if ( context.ShouldApplyToWorldContext( world_context ) )
        {
            UnregisterPooledActors( world_context );
        }
    }
}

void UAPGameFeatureAction_AddPooledActor::HandleGameInstanceStart( UGameInstance * game_instance, FGameFeatureStateChangeContext change_context )
{
    if ( const FWorldContext * world_context = game_instance->GetWorldContext() )
    {
        if ( change_context.ShouldApplyToWorldContext( *world_context ) )
        {
            AddToWorld( *world_context, change_context );
        }
    }
}

void UAPGameFeatureAction_AddPooledActor::AddToWorld( const FWorldContext & world_context, const FGameFeatureStateChangeContext & change_context )
{
    const auto * world = world_context.World();

    if ( world == nullptr )
    {
        return;
    }

    if ( !world->IsGameWorld() )
    {
        return;
    }

    auto * actor_pool_subsystem = world->GetSubsystem< UActorPoolSubSystem >();
    if ( actor_pool_subsystem == nullptr )
    {
        return;
    }

    actor_pool_subsystem->OnActorPoolReady_RegisterAndCall( FAPOnActorPoolReadyEvent::CreateLambda( [ & ]( AActorPoolActor * actor_pool_actor ) {
        for ( const auto & pool_infos : ActorPoolInfos )
        {
            actor_pool_actor->RegisterPooledActor( pool_infos );
        }
    } ) );
}

void UAPGameFeatureAction_AddPooledActor::UnregisterPooledActors( const FWorldContext & world_context )
{
    const auto * world = world_context.World();

    if ( world == nullptr )
    {
        return;
    }

    if ( !world->IsGameWorld() )
    {
        return;
    }

    auto * actor_pool_subsystem = world->GetSubsystem< UActorPoolSubSystem >();
    if ( actor_pool_subsystem == nullptr )
    {
        return;
    }

    if ( !actor_pool_subsystem->IsActorPoolReady() )
    {
        return;
    }

    for ( const auto & pool_infos : ActorPoolInfos )
    {
        actor_pool_subsystem->UnRegisterPooledActor( pool_infos );
    }
}
