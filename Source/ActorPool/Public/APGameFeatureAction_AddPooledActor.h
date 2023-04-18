#pragma once

#include <CoreMinimal.h>
#include <GameFeatureAction.h>
#include <GameFeaturesSubsystem.h>

#include "APGameFeatureAction_AddPooledActor.generated.h"

UCLASS( DisplayName = "Add Pooled Actor" )
class ACTORPOOL_API UAPGameFeatureAction_AddPooledActor final : public UGameFeatureAction
{
    GENERATED_BODY()

public:
    void OnGameFeatureActivating( FGameFeatureActivatingContext & context ) override;
    void OnGameFeatureDeactivating( FGameFeatureDeactivatingContext & context ) override;

private:
    void HandleGameInstanceStart( UGameInstance * game_instance, FGameFeatureStateChangeContext change_context );
    void AddToWorld( const FWorldContext & world_context, const FGameFeatureStateChangeContext & change_context );
    void UnregisterPooledActors( const FWorldContext & world_context );

    UPROPERTY( EditAnywhere, Category = "Pool" )
    TArray< FActorPoolInfos > ActorPoolInfos;

    TMap< FGameFeatureStateChangeContext, FDelegateHandle > GameInstanceStartHandles;
    TMap< FGameFeatureStateChangeContext, FActorPoolInfos > ContextData;
};
