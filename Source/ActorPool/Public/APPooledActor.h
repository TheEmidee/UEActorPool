#pragma once

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>

#include "APPooledActor.generated.h"

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
};
