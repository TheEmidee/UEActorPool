#include "APPooledActor.h"

FAPPooledActorAcquireFromPoolSettings::FAPPooledActorAcquireFromPoolSettings():
    bShowActor( true ),
    bEnableCollision( true ),
    bDisableNetDormancy( true ),
    NetDormancy( ENetDormancy::DORM_Awake )
{
}

void AAPPooledActor::AcquiredFromPool()
{
    SetActorHiddenInGame( !AcquireFromPoolSettings.bShowActor );
    SetActorEnableCollision( AcquireFromPoolSettings.bEnableCollision );

    if ( AcquireFromPoolSettings.bDisableNetDormancy )
    {
        SetNetDormancy( AcquireFromPoolSettings.NetDormancy );
    }

    ReceiveAcquiredFromPool();
}

void AAPPooledActor::ReturnToPool()
{
    SetActorHiddenInGame( true );
    SetActorEnableCollision( false );
    SetNetDormancy( ENetDormancy::DORM_DormantAll );

    ReceiveReturnToPool();
}
