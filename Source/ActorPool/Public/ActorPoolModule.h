#pragma once

#include <CoreMinimal.h>
#include <Modules/ModuleInterface.h>
#include <Modules/ModuleManager.h>

class ACTORPOOL_API IActorPoolModule : public IModuleInterface
{

public:
    static IActorPoolModule & Get()
    {
        static auto & singleton = FModuleManager::LoadModuleChecked< IActorPoolModule >( "ActorPool" );
        return singleton;
    }

    static bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded( "ActorPool" );
    }
};
