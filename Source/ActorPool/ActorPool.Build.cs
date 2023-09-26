using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
    public class ActorPool : ModuleRules
    {
        public ActorPool( ReadOnlyTargetRules Target )
            : base( Target )
        {
            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
            
            PrivatePCHHeaderFile = "Private/ActorPoolPCH.h";

            PrivateIncludePaths.Add("ActorPool/Private");
            
            PublicDependencyModuleNames.AddRange(
                new string[] { 
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "DeveloperSettings",
                    "GameFeatures"
                }
            );
        }
    }
}
