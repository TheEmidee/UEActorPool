#include "ActorPoolSubSystem.h"

#include "APPooledActorInterface.h"
#include "ActorPoolSettings.h"

#include <Engine/Engine.h>
#include <Engine/World.h>
#include <GameFramework/GameModeBase.h>

FActorPoolInstances::FActorPoolInstances() :
    AvailableInstanceIndex( INDEX_NONE )
{
}

FActorPoolInstances::FActorPoolInstances( UWorld * world, const FActorPoolInfos & pool_infos )
{
    AcquireFromPoolSettings = pool_infos.AcquireFromPoolSettings;
    Instances.Reserve( pool_infos.Count );

    FActorSpawnParameters spawn_parameters;
    spawn_parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    for ( auto index = 0; index < pool_infos.Count; ++index )
    {
        auto * actor = world->SpawnActor< AActor >( pool_infos.ActorClass.LoadSynchronous(), spawn_parameters );
        Instances.Add( actor );

        DisableActor( actor );
    }

    AvailableInstanceIndex = 0;
}

AActor * FActorPoolInstances::GetAvailableInstance()
{
    if ( AvailableInstanceIndex == Instances.Num() )
    {
        return nullptr;
    }

    auto * result = Instances[ AvailableInstanceIndex ];

    result->SetActorHiddenInGame( !AcquireFromPoolSettings.bShowActor );
    result->SetActorEnableCollision( AcquireFromPoolSettings.bEnableCollision );

    if ( AcquireFromPoolSettings.bDisableNetDormancy )
    {
        result->SetNetDormancy( AcquireFromPoolSettings.NetDormancy );
    }

    if ( auto * pooled_actor_interface = Cast< IAPPooledActorInterface >( result ) )
    {
        IAPPooledActorInterface::Execute_OnAcquiredFromPool( result );
    }

    AvailableInstanceIndex++;

    return result;
}

bool FActorPoolInstances::ReturnActor( AActor * actor )
{
    if ( actor == nullptr )
    {
        return false;
    }

    const auto index = Instances.Find( actor );

    if ( index == INDEX_NONE )
    {
        return false;
    }

    DisableActor( actor );

    check( AvailableInstanceIndex >= 0 && AvailableInstanceIndex <= Instances.Num() );

    Instances.Swap( index, Instances.Num() - 1 );
    AvailableInstanceIndex--;

    return true;
}

void FActorPoolInstances::DestroyActors()
{
    for ( auto * instance : Instances )
    {
        instance->Destroy();
    }

    Instances.Reset();
    AvailableInstanceIndex = 0;
}

void FActorPoolInstances::DisableActor( AActor * actor ) const
{
    actor->SetActorHiddenInGame( true );
    actor->SetActorEnableCollision( false );
    actor->SetNetDormancy( ENetDormancy::DORM_DormantAll );

    if ( auto * pooled_actor_interface = Cast< IAPPooledActorInterface >( actor ) )
    {
        IAPPooledActorInterface::Execute_OnReturnedToPool( actor );
    }
}

UActorPoolSubSystem::UActorPoolSubSystem()
{
}

void UActorPoolSubSystem::Initialize( FSubsystemCollectionBase & collection )
{
    Super::Initialize( collection );

    if ( GetWorld()->IsGameWorld() )
    {
        GameModeInitializedEventDelegateHandle = FGameModeEvents::GameModeInitializedEvent.AddUObject( this, &UActorPoolSubSystem::OnGameModeInitialized );
    }
}

void UActorPoolSubSystem::Deinitialize()
{
    for ( auto & key_pair : ActorPools )
    {
        key_pair.Value.DestroyActors();
    }

    FGameModeEvents::GameModeInitializedEvent.Remove( GameModeInitializedEventDelegateHandle );

    Super::Deinitialize();
}

bool UActorPoolSubSystem::IsActorPoolable( AActor * actor ) const
{
    if ( actor == nullptr )
    {
        return false;
    }

    return IsActorClassPoolable( actor->GetClass() );
}

bool UActorPoolSubSystem::IsActorClassPoolable( const TSubclassOf<AActor> actor_class ) const
{
    if ( actor_class == nullptr )
    {
        return false;
    }

    for ( const auto & key_pair : ActorPools )
    {
        if ( actor_class == key_pair.Key )
        {
            return true;
        }
    }

    return false;
}

AActor * UActorPoolSubSystem::GetActorFromPool( const TSubclassOf< AActor > actor_class )
{
    if ( auto * actor_instances = ActorPools.Find( actor_class ) )
    {
        return actor_instances->GetAvailableInstance();
    }

    return nullptr;
}

AActor * UActorPoolSubSystem::GetActorFromPoolWithTransform( const TSubclassOf<AActor> actor_class, const FTransform transform )
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
    if ( actor == nullptr )
    {
        return false;
    }

    if ( auto * actor_instances = ActorPools.Find( actor->GetClass() ) )
    {
        return actor_instances->ReturnActor( actor );
    }

    return false;
}

void UActorPoolSubSystem::OnGameModeInitialized( AGameModeBase * game_mode )
{
    if ( auto * settings = GetDefault< UActorPoolSettings >() )
    {
        for ( const auto & pool_infos : settings->PoolInfos )
        {
            ActorPools.Emplace( pool_infos.ActorClass.LoadSynchronous(), CreateActorPoolInstance( pool_infos ) );
        }
    }
}

FActorPoolInstances UActorPoolSubSystem::CreateActorPoolInstance( const FActorPoolInfos & pool_infos ) const
{
    return FActorPoolInstances( GetWorld(), pool_infos );
}
