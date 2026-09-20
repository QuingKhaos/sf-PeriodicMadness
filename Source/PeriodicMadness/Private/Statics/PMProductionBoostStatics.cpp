#include "Statics/PMProductionBoostStatics.h"
#include "Registry/ContentTagRegistry.h"
#include "FGRecipe.h"

bool UPMProductionBoostStatics::IsProductionBoostDisabled(const UObject* WorldContextObject, const TSubclassOf<UFGRecipe> Recipe)
{
	const FGameplayTag ProductionBoostDisabledTag = FGameplayTag::RequestGameplayTag(FName("Recipe.DisableProductionBoost"));
	UContentTagRegistry* TagRegistry = UContentTagRegistry::Get(WorldContextObject);

	const FGameplayTagContainer RecipeTags = TagRegistry->GetGameplayTagContainerFor(Recipe);

	return RecipeTags.HasTagExact(ProductionBoostDisabledTag);
}
