#include "ActorPoolActor.h"

AActorPoolActor::AActorPoolActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AActorPoolActor::BeginPlay()
{
    Super::BeginPlay();
}
