#include "ActorPoolActor.h"

#include "APPooledActorInterface.h"
#include "ActorPoolSubSystem.h"

#include <Engine/World.h>

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
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

AActor * FActorPoolInstances::GetAvailableInstance( UWorld * world )
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
            SpawnActorAndAddToInstances( world );
        }
    }

    auto * result = Instances[ AvailableInstanceIndex ];

    result->SetActorHiddenInGame( !PoolInfos.AcquireFromPoolSettings.bShowActor );
    result->SetActorEnableCollision( PoolInfos.AcquireFromPoolSettings.bEnableCollision );

    if ( PoolInfos.AcquireFromPoolSettings.bDisableNetDormancy )
    {
        result->SetNetDormancy( PoolInfos.AcquireFromPoolSettings.NetDormancy );
    }

    if ( Cast< IAPPooledActorInterface >( result ) )
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

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
void FActorPoolInstances::DumpPoolInfos( FOutputDevice & output_device ) const
{
    output_device.Logf( ELogVerbosity::Verbose, TEXT( "Pool for class %s" ), *PoolInfos.ActorClass.ToString() );
    output_device.Logf( ELogVerbosity::Verbose, TEXT( "   Total Instance Count : %i" ), PoolInfos.Count );
    output_device.Logf( ELogVerbosity::Verbose, TEXT( "   Alive Instance Count : %i" ), AvailableInstanceIndex );
    output_device.Logf( ELogVerbosity::Verbose, TEXT( "   Available Instance Count : %i" ), ( PoolInfos.Count - AvailableInstanceIndex ) );
}
#endif

void FActorPoolInstances::DisableActor( AActor * actor ) const
{
    actor->SetActorHiddenInGame( true );
    actor->SetActorEnableCollision( false );
    actor->SetNetDormancy( ENetDormancy::DORM_DormantAll );

    if ( Cast< IAPPooledActorInterface >( actor ) )
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

AActorPoolActor::AActorPoolActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AActorPoolActor::BeginPlay()
{
    Super::BeginPlay();

    if ( !HasAuthority() )
    {
        return;
    }

    //Initialize the pools
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

    // Register itself to the subsystem
    if ( auto * actor_pool_system = GetWorld()->GetSubsystem< UActorPoolSubSystem >() )
    {
        actor_pool_system->RegisterActorPoolActor( this );
        actor_pool_system->OnActorPoolReady().Broadcast();
    }
}

void AActorPoolActor::EndPlay( const EEndPlayReason::Type end_play_reason )
{
    for ( auto & key_pair : ActorPools )
    {
        key_pair.Value.DestroyActors();
    }

    Super::EndPlay( end_play_reason );
}

bool AActorPoolActor::IsActorClassPoolable( TSubclassOf< AActor > actor_class ) const
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

AActor * AActorPoolActor::GetActorFromPool( TSubclassOf< AActor > actor_class )
{
    if ( auto * actor_instances = ActorPools.Find( actor_class ) )
    {
        return actor_instances->GetAvailableInstance( GetWorld() );
    }

    return nullptr;
}

bool AActorPoolActor::ReturnActorToPool( AActor * actor )
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
void AActorPoolActor::DestroyUnusedInstancesInPools()
{
    for ( auto & key_pair : ActorPools )
    {
        key_pair.Value.DestroyUnusedInstances();
    }
}

void AActorPoolActor::DumpPoolInfos( FOutputDevice & output_device ) const
{
    output_device.Logf( ELogVerbosity::Verbose, TEXT( "Dumping Actor Pool Infos :" ) );

    for ( auto & key_pair : ActorPools )
    {
        key_pair.Value.DumpPoolInfos( output_device );
    }
}
#endif

FActorPoolInstances AActorPoolActor::CreateActorPoolInstance( const FActorPoolInfos & pool_infos ) const
{
    return FActorPoolInstances( GetWorld(), pool_infos );
}
