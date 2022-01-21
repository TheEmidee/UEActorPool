#pragma once

#include <CoreMinimal.h>
#include <Subsystems/WorldSubsystem.h>

#include "ActorPoolSubSystem.generated.h"

class AAPPooledActor;
struct FActorPoolInfos;
class AGameModeBase;

USTRUCT()
struct FActorPoolInstances
{
    GENERATED_USTRUCT_BODY()

    AAPPooledActor * GetAvailableInstance();
    void ReturnActor( AAPPooledActor * actor );

    UPROPERTY()
    TArray< AAPPooledActor * > Instances;

    int AvailableInstanceIndex;
};

UCLASS()
class ACTORPOOL_API UActorPoolSubSystem final : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void Initialize( FSubsystemCollectionBase & collection ) override;
    void Deinitialize() override;

    UFUNCTION( BlueprintCallable )
    AAPPooledActor * GetActorFromPool( TSubclassOf< AAPPooledActor > actor_class );

    UFUNCTION( BlueprintCallable )
    void ReturnActorToPool( AAPPooledActor * actor );

private:

    FActorPoolInstances CreateActorPoolInstance( const FActorPoolInfos & pool_infos ) const;

    UPROPERTY()
    TMap< TSubclassOf< AAPPooledActor >, FActorPoolInstances > ActorPools;
};
