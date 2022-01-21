#include "ActorPoolSettings.h"

FName UActorPoolSettings::GetCategoryName() const
{
    static const FName CustomCategoryName( TEXT( "Game" ) );
    return CustomCategoryName;
}
