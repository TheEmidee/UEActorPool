#pragma once

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>

#include "APPooledActor.generated.h"

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

UCLASS()
class ACTORPOOL_API AAPPooledActor : public AActor
{
    GENERATED_BODY()

public:
    virtual void AcquiredFromPool();
    virtual void ReturnToPool();

protected:
    UFUNCTION( BlueprintImplementableEvent )
    void ReceiveAcquiredFromPool();

    UFUNCTION( BlueprintImplementableEvent )
    void ReceiveReturnToPool();

    UPROPERTY( EditAnywhere, Category = "Pooled Actor" )
    FAPPooledActorAcquireFromPoolSettings AcquireFromPoolSettings;
};
