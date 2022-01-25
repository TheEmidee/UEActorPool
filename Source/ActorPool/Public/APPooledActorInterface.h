#pragma once

#include <CoreMinimal.h>
#include <UObject/Interface.h>

#include "APPooledActorInterface.generated.h"

UINTERFACE( MinimalAPI )
class UAPPooledActorInterface : public UInterface
{
    GENERATED_BODY()
};

class ACTORPOOL_API IAPPooledActorInterface
{
    GENERATED_BODY()

public:

    UFUNCTION( BlueprintNativeEvent, BlueprintCallable )
    void OnAcquiredFromPool();

    UFUNCTION( BlueprintNativeEvent, BlueprintCallable )
    void OnReturnedToPool();
};
