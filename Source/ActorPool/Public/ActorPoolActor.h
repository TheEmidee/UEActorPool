#pragma once

#include "ActorPoolSettings.h"

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>

#include "ActorPoolActor.generated.h"

DECLARE_DELEGATE_OneParam( FAPOnActorGotFromPoolDelegate, AActor * Actor );

struct FActorPoolInfos;

USTRUCT( BlueprintType )
struct ACTORPOOL_API FActorPoolRequestHandle
{
    GENERATED_USTRUCT_BODY()

    FActorPoolRequestHandle() :
        Handle( INDEX_NONE )
    {
    }

    explicit FActorPoolRequestHandle( int32 handle ) :
        Handle( handle )
    {
    }

    bool IsValid() const
    {
        return Handle != INDEX_NONE;
    }

    bool operator==( const FActorPoolRequestHandle & Other ) const
    {
        return Handle == Other.Handle;
    }

    bool operator!=( const FActorPoolRequestHandle & Other ) const
    {
        return Handle != Other.Handle;
    }

    friend uint32 GetTypeHash( const FActorPoolRequestHandle & InHandle )
    {
        return InHandle.Handle;
    }

    FString ToString() const
    {
        return FString::Printf( TEXT( "%d" ), Handle );
    }

    void Invalidate()
    {
        Handle = INDEX_NONE;
    }

    static FActorPoolRequestHandle GenerateNewHandle()
    {
        static int GHandle = 0;

        return FActorPoolRequestHandle( GHandle++ );
    }

private:
    int32 Handle;
};

USTRUCT()
struct FActorPoolInstances
{
    GENERATED_USTRUCT_BODY()

public:
    FActorPoolInstances();
    FActorPoolInstances( UWorld * world, const FActorPoolInfos & pool_infos );

    AActor * GetAvailableInstance( UWorld * world );
    bool ReturnActor( AActor * actor );
    void DestroyActors();
    void DestroyUnusedInstances();

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
    void DumpPoolInfos( FOutputDevice & output_device ) const;
#endif

private:
    void DisableActor( AActor * actor ) const;
    AActor * SpawnActorAndAddToInstances( UWorld * world );

    UPROPERTY()
    TArray< AActor * > Instances;

    int AvailableInstanceIndex;
    FActorPoolInfos PoolInfos;
};

UCLASS( NotPlaceable, NotBlueprintType, NotBlueprintable )
class ACTORPOOL_API AActorPoolActor : public AActor
{
    GENERATED_BODY()

public:
    AActorPoolActor();

    void BeginPlay() override;
    void EndPlay( const EEndPlayReason::Type end_play_reason ) override;

    bool IsActorClassPoolable( TSubclassOf< AActor > actor_class ) const;

    void RegisterPooledActor( const FActorPoolInfos & actor_pool_infos );
    void UnRegisterPooledActor( const FActorPoolInfos & actor_pool_infos );

    AActor * GetActorFromPool( TSubclassOf< AActor > actor_class );
    void FinishAcquireActor( FActorPoolRequestHandle handle );

    bool ReturnActorToPool( AActor * actor );

#if !( UE_BUILD_SHIPPING || UE_BUILD_TEST )
    void DestroyUnusedInstancesInPools();
    void DumpPoolInfos( FOutputDevice & output_device ) const;
#endif

private:
    FActorPoolInstances CreateActorPoolInstance( const FActorPoolInfos & pool_infos ) const;

    UPROPERTY()
    TMap< TSubclassOf< AActor >, FActorPoolInstances > ActorPools;
};
