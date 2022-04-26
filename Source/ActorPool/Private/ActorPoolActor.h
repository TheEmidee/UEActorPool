#pragma once

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>

#include "ActorPoolActor.generated.h"

UCLASS()
class ACTORPOOL_API AActorPoolActor : public AActor
{
    GENERATED_BODY()

public:
    AActorPoolActor();

    void BeginPlay() override;
};
