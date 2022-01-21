#pragma once

#include <CoreMinimal.h>
#include <Engine/DeveloperSettings.h>
#include <Templates/SubclassOf.h>

#include "ActorPoolSettings.generated.h"

class AAPPooledActor;

USTRUCT()
struct FActorPoolInfos
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY( EditAnywhere )
    TSubclassOf< AAPPooledActor > ActorClass;

    UPROPERTY( EditAnywhere )
    int Count;
};

UCLASS( config = Game, defaultconfig, meta = ( DisplayName = "ActorPool" ) )
class ACTORPOOL_API UActorPoolSettings final : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    FName GetCategoryName() const override;

    UPROPERTY( EditAnywhere )
    TArray< FActorPoolInfos > PoolInfos;
};
