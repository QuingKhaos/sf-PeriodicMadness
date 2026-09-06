

#pragma once

#include "CoreMinimal.h"
#include "Module/GameWorldModule.h"
#include "PMCleanerWorldModule.generated.h"

class UFGRecipe;

/**
 * Sub world module for resource cleanup.
 */
UCLASS()
class PERIODICMADNESS_API UPMCleanerWorldModule : public UGameWorldModule
{
	GENERATED_BODY()

public:
	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;

protected:
	/** Remove all resource nodes, except allowlisted ones. */
	void RemoveResourceNodes();

	/** Replace targeted resource node classes with their replacements. */
	void ReplaceResources();

	/** Remove all research trees, except allowlisted ones. */
	void RemoveResearchTrees();

	/** Remove all schematics, except allowlisted ones. */
	void RemoveSchematics();

	/** Remove all recipes, except allowlisted ones. */
	void RemoveRecipes();

	/** Remove the actual recipe, except allowlisted ones. */
	void RemoveRecipe(TSubclassOf<UFGRecipe> Recipe);

	/** Remove all items, except allowlisted ones. */
	void RemoveItems();

	/** Called after resource nodes have been removed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Periodic Madness|Resource Cleanup", meta = (DisplayName = "On Resource Nodes Removed"))
	void K2_ResourceNodesRemoved();

	/** Cached CDOs to prevent garbage collection. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> mCachedCDO;
};
