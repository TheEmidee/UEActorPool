#include "ActorPoolSettings.h"

FAPPooledActorAcquireFromPoolSettings::FAPPooledActorAcquireFromPoolSettings():
    bShowActor( true ),
    bEnableCollision( true ),
    bDisableNetDormancy( true ),
    NetDormancy( ENetDormancy::DORM_Awake )
{}

FActorPoolInfos::FActorPoolInfos():
    Count( 0 )
{}

FName UActorPoolSettings::GetCategoryName() const
{
    static const FName CustomCategoryName( TEXT( "Game" ) );
    return CustomCategoryName;
}
