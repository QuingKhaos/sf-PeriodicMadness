#include "Hooks/PMProductionBoostHooks.h"
#include "Buildables/FGBuildableFactory.h"
#include "Buildables/FGBuildableManufacturer.h"
#include "Statics/PMProductionBoostStatics.h"
#include "PeriodicMadnessLogChannels.h"

void UPMProductionBoostHooks::ConfigureHooks()
{
	if (!WITH_EDITOR) {
		SUBSCRIBE_UOBJECT_METHOD(AFGBuildableFactory, SetPendingProductionBoost, &UPMProductionBoostHooks::BuildableFactorySetPendingProductionBoost);
		SUBSCRIBE_UOBJECT_METHOD(AFGBuildableManufacturer, SetRecipe, &UPMProductionBoostHooks::BuildableManufacturerSetRecipe);
	}
}

void UPMProductionBoostHooks::BuildableFactorySetPendingProductionBoost(TCallScope<void(*)(AFGBuildableFactory*, float)>& Scope, AFGBuildableFactory* Self, float NewPendingProductionBoost)
{
	if(const AFGBuildableManufacturer* Manufacturer = Cast<AFGBuildableManufacturer>(Self))
	{
		const TSubclassOf<UFGRecipe> Recipe = Manufacturer->GetCurrentRecipe();
		if (Recipe && UPMProductionBoostStatics::IsProductionBoostDisabled(Self, Recipe))
		{
			Scope(Self, 1.f);
		}
	}
}

void UPMProductionBoostHooks::BuildableManufacturerSetRecipe(TCallScope<void(*)(AFGBuildableManufacturer*, TSubclassOf<UFGRecipe>)>& Scope, AFGBuildableManufacturer* Self, TSubclassOf<UFGRecipe> Recipe)
{
	if (UPMProductionBoostStatics::IsProductionBoostDisabled(Self, Recipe))
	{
		Self->SetPendingProductionBoost(1.f);
		Self->SetCurrentProductionBoost(1.f);
	}
	else
	{
		const float MaxProductionBoost = Self->GetMaxProductionBoost();
		Self->SetPendingProductionBoost(MaxProductionBoost);
	}
}
