#pragma once

#include <CoreMinimal.h>
#include <Engine/DeveloperSettings.h>
#include <Engine/EngineTypes.h>
#include <Templates/SubclassOf.h>

#include "ActorPoolSettings.generated.h"

class AActor;

UENUM()
enum class EAPPoolingPolicy : uint8
{
    CreateNewInstances,
    LoopInstances
};

USTRUCT()
struct FAPPooledActorAcquireFromPoolSettings
{
    GENERATED_USTRUCT_BODY()

    FAPPooledActorAcquireFromPoolSettings();

    UPROPERTY( EditAnywhere )
    uint8 bShowActor : 1;

    UPROPERTY( EditAnywhere )
    uint8 bEnableCollision : 1;

    UPROPERTY( EditAnywhere )
    uint8 bDisableNetDormancy : 1;

    UPROPERTY( EditAnywhere, meta = ( EditCondition = "bDisableNetDormancy" ) )
    TEnumAsByte< ENetDormancy > NetDormancy;
};

USTRUCT()
struct FActorPoolInfos
{
    GENERATED_USTRUCT_BODY()

    FActorPoolInfos();

    UPROPERTY( EditAnywhere )
    TSoftClassPtr< AActor > ActorClass;

    UPROPERTY( EditAnywhere )
    int Count;

    UPROPERTY( EditAnywhere )
    EAPPoolingPolicy PoolingPolicy;

    UPROPERTY( EditAnywhere )
    FAPPooledActorAcquireFromPoolSettings AcquireFromPoolSettings;

    UPROPERTY( EditAnywhere )
    uint8 bSpawnOnServer : 1;

    UPROPERTY( EditAnywhere )
    uint8 bSpawnOnClients : 1;
};

UCLASS( config = Game, defaultconfig, meta = ( DisplayName = "ActorPool" ) )
class ACTORPOOL_API UActorPoolSettings final : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    FName GetCategoryName() const override;

    UPROPERTY( EditAnywhere, config )
    TArray< FActorPoolInfos > PoolInfos;
};
