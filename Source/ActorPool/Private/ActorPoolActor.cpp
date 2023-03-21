#include "ActorPoolActor.h"

#include "APPooledActorInterface.h"
#include "ActorPoolLog.h"
#include "ActorPoolSubSystem.h"

#include <Engine/Engine.h>
#include <Engine/World.h>
#include <Kismet/KismetSystemLibrary.h>

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
static TAutoConsoleVariable< int32 > GActorPoolForceInstanceCreationWhenPoolIsEmpty(
    TEXT( "ActorPool.ForceInstanceCreationWhenPoolIsEmpty" ),
    0,
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
    AvailableInstanceIndex( 0 )
{
}

FActorPoolInstances::FActorPoolInstances( UWorld * world, const FActorPoolInfos & pool_infos ) :
    AvailableInstanceIndex( 0 ),
    PoolInfos( pool_infos )
{
    Instances.Reserve( pool_infos.Count );

    for ( auto index = 0; index < pool_infos.Count; ++index )
    {
        auto * actor = SpawnActorAndAddToInstances( world );
        DisableActor( actor );
    }

    UE_LOG( LogActorPool, Verbose, TEXT( "Created %i instances for %s" ), pool_infos.Count, *PoolInfos.ActorClass.LoadSynchronous()->GetName() );
}

AActor * FActorPoolInstances::GetAvailableInstance( UWorld * world )
{
    if ( AvailableInstanceIndex == Instances.Num() )
    {
#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
        if ( GActorPoolForceInstanceCreationWhenPoolIsEmpty.GetValueOnGameThread() )
        {
            PoolInfos.PoolingPolicy = EAPPoolingPolicy::CreateNewInstances;
        }
#endif

        switch ( PoolInfos.PoolingPolicy )
        {
            case EAPPoolingPolicy::CreateNewInstances:
            {
                SpawnActorAndAddToInstances( world );
            }
            break;
            case EAPPoolingPolicy::LoopInstances:
            {
                AvailableInstanceIndex = 0;
            }
            break;
            default:
            {
                checkNoEntry();
            }
            break;
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

    UE_LOG( LogActorPool, Verbose, TEXT( "GetAvailableInstance : %s - AvailableInstanceIndex : %i" ), *GetNameSafe( result ), AvailableInstanceIndex );

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

    // We can't let AvailableInstanceIndex go below 0. If we're already at 0 this probably means we're returning an actor that has already been returned already
    if ( AvailableInstanceIndex == 0 )
    {
        return false;
    }

    DisableActor( actor );

    check( AvailableInstanceIndex > 0 && AvailableInstanceIndex <= Instances.Num() );

    if ( PoolInfos.PoolingPolicy == EAPPoolingPolicy::LoopInstances && AvailableInstanceIndex == 0 )
    {
        AvailableInstanceIndex = Instances.Num();
    }

    AvailableInstanceIndex--;

    if ( Instances.Num() > 1 )
    {
        Instances.RemoveAt( index, 1, false );
        Instances.Insert( actor, AvailableInstanceIndex );
    }

    UE_LOG( LogActorPool, Verbose, TEXT( "ReturnActor : %s - AvailableInstanceIndex : %i" ), *GetNameSafe( actor ), AvailableInstanceIndex );

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

    // Initialize the pools
#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
    if ( GActorPoolDisable.GetValueOnGameThread() == 0 )
#endif
    {
        if ( auto * settings = GetDefault< UActorPoolSettings >() )
        {
            const auto * world = GetWorld();
            const auto is_standalone = UKismetSystemLibrary::IsStandalone( world );
            auto is_server = IsRunningDedicatedServer();

#if WITH_EDITOR
            checkSlow( game_instance->GetWorldContext() );
            is_server |= world->GetGameInstance()->GetWorldContext()->RunAsDedicated;
#endif

            const auto is_client = !is_server;

            for ( const auto & pool_infos : settings->PoolInfos )
            {
                if ( is_standalone || is_server && pool_infos.bSpawnOnServer || is_client && pool_infos.bSpawnOnClients )
                {
                    ActorPools.Emplace( pool_infos.ActorClass.LoadSynchronous(), CreateActorPoolInstance( pool_infos ) );
                }
            }
        }
    }

    // Register itself to the subsystem
    if ( auto * actor_pool_system = GetWorld()->GetSubsystem< UActorPoolSubSystem >() )
    {
        actor_pool_system->RegisterActorPoolActor( this );
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
    auto * actor_instances = ActorPools.Find( actor_class );

    if ( actor_instances == nullptr )
    {
#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
        if ( GActorPoolForceInstanceCreationWhenPoolIsEmpty.GetValueOnGameThread() == 1 )
        {
            FActorPoolInfos pool_infos;
            pool_infos.ActorClass = actor_class;
            pool_infos.Count = 1;
            pool_infos.PoolingPolicy = EAPPoolingPolicy::CreateNewInstances;

            actor_instances = &ActorPools.Add( actor_class, FActorPoolInstances( GetWorld(), pool_infos ) );
        }
        else
#endif
        {
            return nullptr;
        }
    }

    return actor_instances->GetAvailableInstance( GetWorld() );
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
