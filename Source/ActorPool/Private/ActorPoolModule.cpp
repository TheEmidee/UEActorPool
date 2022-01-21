#include "ActorPoolModule.h"

class FActorPoolModule final : public IActorPoolModule
{
public:
    void StartupModule() override;
    void ShutdownModule() override;
};

IMPLEMENT_MODULE( FActorPoolModule, ActorPool )

void FActorPoolModule::StartupModule()
{
}

void FActorPoolModule::ShutdownModule()
{
}