#pragma once

#include "CoreMinimal.h"
#include "Patching/NativeHookManager.h"
#include "UObject/NoExportTypes.h"
#include "PMProductionBoostHooks.generated.h"

class AFGBuildableFactory;
class AFGBuildableManufacturer;
class UFGRecipe;

UCLASS()
class PERIODICMADNESS_API UPMProductionBoostHooks : public UObject
{
	GENERATED_BODY()
	
public:
	static void ConfigureHooks();

private:
	static void BuildableFactorySetPendingProductionBoost(TCallScope<void(*)(AFGBuildableFactory*, float)>& Scope, AFGBuildableFactory* Self, float NewPendingProductionBoost);
	static void BuildableManufacturerSetRecipe(TCallScope<void(*)(AFGBuildableManufacturer*, TSubclassOf<UFGRecipe>)>& Scope, AFGBuildableManufacturer* Self, TSubclassOf<UFGRecipe> Recipe);
};
