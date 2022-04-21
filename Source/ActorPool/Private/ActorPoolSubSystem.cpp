#include "ActorPoolSubSystem.h"

#include "APPooledActorInterface.h"
#include "ActorPoolSettings.h"

#include <Engine/Engine.h>
#include <Engine/World.h>
#include <GameFramework/GameModeBase.h>

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
static FAutoConsoleCommandWithWorld GActorPoolDestroyInstancesInPools(
    TEXT( "ActorPool.DestroyUnusedInstancesInPools" ),
    TEXT( "Destroys all actors in the pools which have not been acquired." ),
    FConsoleCommandWithWorldDelegate::CreateStatic( &UActorPoolSubSystem::DestroyUnusedInstancesInPools ),
    ECVF_Default );

static TAutoConsoleVariable< int32 > GActorPoolForceInstanceCreationWhenPoolIsEmpty(
    TEXT( "ActorPool.ForceInstanceCreationWhenPoolIsEmpty" ),
    1,
    TEXT( "When on, will force to create actor instances when the pool is empty.\n" )
        TEXT( "0: Disable, 1: Enable" ),
    ECVF_Default );

static TAutoConsoleVariable< int32 > GActorPoolDisable(
    TEXT( "ActorPool.Disable" ),
    0,
    TEXT( "When on, will not create any instances.\n" )
        TEXT( "0: Enable the pools, 1: Disable the pools" ),
    ECVF_Default );
#endif

FActorPoolInstances::FActorPoolInstances() :
    AvailableInstanceIndex( INDEX_NONE )
{
}

FActorPoolInstances::FActorPoolInstances( UWorld * world, const FActorPoolInfos & pool_infos ) :
    AvailableInstanceIndex( INDEX_NONE ),
    PoolInfos( pool_infos )
{
    Instances.Reserve( pool_infos.Count );

    for ( auto index = 0; index < pool_infos.Count; ++index )
    {
        auto * actor = SpawnActorAndAddToInstances( world );
        DisableActor( actor );
    }

    AvailableInstanceIndex = 0;
}

AActor * FActorPoolInstances::GetAvailableInstance( const UObject * world_context )
{
    if ( AvailableInstanceIndex == Instances.Num() )
    {
        const auto can_create_instance = PoolInfos.bAllowNewInstancesWhenPoolIsEmpty
#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
                                         || GActorPoolForceInstanceCreationWhenPoolIsEmpty.GetValueOnGameThread()
#endif
            ;

        if ( can_create_instance )
        {
            SpawnActorAndAddToInstances( world_context->GetWorld() );
        }
    }

    auto * result = Instances[ AvailableInstanceIndex ];

    result->SetActorHiddenInGame( !PoolInfos.AcquireFromPoolSettings.bShowActor );
    result->SetActorEnableCollision( PoolInfos.AcquireFromPoolSettings.bEnableCollision );

    if ( PoolInfos.AcquireFromPoolSettings.bDisableNetDormancy )
    {
        result->SetNetDormancy( PoolInfos.AcquireFromPoolSettings.NetDormancy );
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

    if ( Instances.Num() > 1 )
    {
        Instances.RemoveAt( index, 1, false );
        Instances.Insert( actor, AvailableInstanceIndex - 1 );
    }

    AvailableInstanceIndex--;

    return true;
}

void FActorPoolInstances::DestroyActors()
{
    for ( auto * instance : Instances )
    {
        if ( IsValid( instance ) )
        {
            instance->Destroy();
        }
    }

    Instances.Reset();
    AvailableInstanceIndex = 0;
}

void FActorPoolInstances::DestroyUnusedInstances()
{
    for ( auto index = AvailableInstanceIndex; index < Instances.Num(); index++ )
    {
        if ( auto * instance = Instances[ index ] )
        {
            instance->Destroy();
        }
    }

    Instances.SetNum( AvailableInstanceIndex );
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

AActor * FActorPoolInstances::SpawnActorAndAddToInstances( UWorld * world )
{
    FActorSpawnParameters spawn_parameters;
    spawn_parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    auto * actor = world->SpawnActor< AActor >( PoolInfos.ActorClass.LoadSynchronous(), spawn_parameters );
    Instances.Add( actor );

    return actor;
}

UActorPoolSubSystem::UActorPoolSubSystem()
{
}

void UActorPoolSubSystem::Initialize( FSubsystemCollectionBase & collection )
{
    Super::Initialize( collection );

    if ( auto * world = GetWorld()  )
    {
        if ( world->IsGameWorld() )
        {
            world->OnWorldBeginPlay.AddUObject( this, &UActorPoolSubSystem::OnWorldBeginPlay );
        }
    }
}

void UActorPoolSubSystem::Deinitialize()
{
    for ( auto & key_pair : ActorPools )
    {
        key_pair.Value.DestroyActors();
    }

    if ( auto * world = GetWorld() )
    {
        world->OnWorldBeginPlay.RemoveAll( this );
    }

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

bool UActorPoolSubSystem::IsActorClassPoolable( const TSubclassOf< AActor > actor_class ) const
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

AActor * UActorPoolSubSystem::GetActorFromPool( UObject * world_context, const TSubclassOf< AActor > actor_class )
{
    if ( auto * actor_instances = ActorPools.Find( actor_class ) )
    {
        return actor_instances->GetAvailableInstance( world_context );
    }

    return nullptr;
}

AActor * UActorPoolSubSystem::GetActorFromPoolWithTransform( UObject * world_context, const TSubclassOf< AActor > actor_class, const FTransform transform )
{
    if ( auto * result = GetActorFromPool( world_context, actor_class ) )
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

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
void UActorPoolSubSystem::DestroyUnusedInstancesInPools( UWorld * world )
{
    if ( world == nullptr )
    {
        return;
    }

    if ( auto * subsystem = world->GetSubsystem< UActorPoolSubSystem >() )
    {
        for ( auto & key_pair : subsystem->ActorPools )
        {
            key_pair.Value.DestroyUnusedInstances();
        }
    }
}
#endif

void UActorPoolSubSystem::OnWorldBeginPlay()
{
#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
    if ( GActorPoolDisable.GetValueOnGameThread() == 1 )
    {
        return;
    }
#endif

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
