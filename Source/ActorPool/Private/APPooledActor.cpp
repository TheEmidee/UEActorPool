#include "APPooledActor.h"

void AAPPooledActor::AcquiredFromPool()
{
    SetActorHiddenInGame( false );
    SetActorEnableCollision( true );
    SetNetDormancy( ENetDormancy::DORM_Awake );

    ReceiveAcquiredFromPool();
}

void AAPPooledActor::ReturnToPool()
{
    SetActorHiddenInGame( true );
    SetActorEnableCollision( false );
    SetNetDormancy( ENetDormancy::DORM_DormantAll );

    ReceiveReturnToPool();
}
