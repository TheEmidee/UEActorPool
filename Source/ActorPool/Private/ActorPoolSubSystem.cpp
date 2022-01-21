#include "ActorPoolSubSystem.h"

#include "APPooledActor.h"
#include "ActorPoolSettings.h"

#include <Engine/World.h>

AAPPooledActor * FActorPoolInstances::GetAvailableInstance()
{
    if ( AvailableInstanceIndex == Instances.Num() )
    {
        return nullptr;
    }

    auto * result = Instances[ AvailableInstanceIndex ];
    AvailableInstanceIndex++;

    return result;
}

void FActorPoolInstances::ReturnActor( AAPPooledActor * actor )
{
    if ( actor == nullptr )
    {
        return;
    }

    const auto index = Instances.Find( actor );

    if ( index == INDEX_NONE )
    {
        return;
    }

    check( AvailableInstanceIndex > 0 && AvailableInstanceIndex <= Instances.Num() );

    Instances.Swap( index, Instances.Num() - 1 );
    AvailableInstanceIndex--;
}

void UActorPoolSubSystem::Initialize( FSubsystemCollectionBase & collection )
{
    Super::Initialize( collection );

    if ( auto * settings = GetDefault< UActorPoolSettings >() )
    {
        for ( const auto & pool_infos : settings->PoolInfos )
        {
            ActorPools.Emplace( pool_infos.ActorClass, CreateActorPoolInstance( pool_infos ) );
        }
    }
}

void UActorPoolSubSystem::Deinitialize()
{
    for ( auto & key_pair : ActorPools )
    {
        for ( auto * instance : key_pair.Value.Instances )
        {
            GetWorld()->DestroyActor( instance );
        }
    }

    Super::Deinitialize();
}

AAPPooledActor * UActorPoolSubSystem::GetActorFromPool( const TSubclassOf< AAPPooledActor > actor_class )
{
    if ( auto * actor_instances = ActorPools.Find( actor_class ) )
    {
        return actor_instances->GetAvailableInstance();
    }

    return nullptr;
}

void UActorPoolSubSystem::ReturnActorToPool( AAPPooledActor * actor )
{
    if ( actor == nullptr )
    {
        return;
    }

    if ( auto * actor_instances = ActorPools.Find( actor->GetClass() ) )
    {
        actor_instances->ReturnActor( actor );
    }
}

FActorPoolInstances UActorPoolSubSystem::CreateActorPoolInstance( const FActorPoolInfos & pool_infos ) const
{
    FActorPoolInstances result;
    result.Instances.Reserve( pool_infos.Count );

    FActorSpawnParameters spawn_parameters;
    spawn_parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    for ( auto index = 0; index < pool_infos.Count; ++index )
    {
        auto * actor = GetWorld()->SpawnActor< AAPPooledActor >( pool_infos.ActorClass, spawn_parameters );
        result.Instances.Add( actor );
    }

    result.AvailableInstanceIndex = 0;
    return result;
}
