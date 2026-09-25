#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "PMCleanerSubsystem.generated.h"

class AFGDropPod;
class UFGResearchTree;
class UFGSchematic;

/**
 * Subsystem for resource cleanup during runtime. 
 */
UCLASS()
class PERIODICMADNESS_API APMCleanerSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	APMCleanerSubsystem();

	static APMCleanerSubsystem* Get(UWorld* World);

	UFUNCTION(BlueprintPure, Category = "Periodic Madness|Resource Cleaner", DisplayName = "GetPMResourceCleanerSubsystem", Meta = (DefaultToSelf = "WorldContext"))
	static APMCleanerSubsystem* Get(const UObject* WorldContext);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Update the resource scanner to pick up any changes. */
	UFUNCTION(BlueprintCallable, Category = "Periodic Madness|Resource Cleaner")
	void UpdateResourceScanner();

	/** Modify the CDO of a research tree when it is removed. */
	void ResearchTreeRemovalModifyCDO(const TSubclassOf<UFGResearchTree> ResearchTree, bool bReplay = false);

	/** Modify the CDO of a schematic when it is removed. */
	void SchematicRemovalModifyCDO(const TSubclassOf<UFGSchematic> Schematic, bool bReplay = false);

	/** Cleanup the CDO of a schematic from unwanted resource scanner pairs. */
	void SchematicCleanup(const TSubclassOf<UFGSchematic> Schematic, bool bReplay = false);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Handle removal/cleanup of things when the streaming state is updated. */
	void OnStreamingStateUpdated();

	/** Replay CDO modifications on clients. */
	void ReplayCDOModifications();

	/** Remove all static meshes on the cleanlist. */
	void RemoveStaticMeshes();

	/** Remove all resource deposits, except allowlisted ones. */
	void RemoveResourceDeposits();

	/** Replace targeted resource deposit classes with their replacements. */
	void ReplaceResourceDeposits();

	/** Remove all foliage item drops, except allowlisted ones. */
	void RemoveFoliageItemDrops();

	/** Replace targeted foliage item drops with their replacements. */
	void ReplaceFoliageItemDrops();

	/** Replace targeted water volume resource classes with their replacements. */
	void ReplaceWaterVolumes();

	/** Cleanup crash sites. */
	void CleanupCrashSites();

	/** Cleanup dropped items around crash sites. */
	void CleanupDroppedItems(AFGDropPod* DropPod);

private:
	/** Removed research trees which where CDO modified. */
	UPROPERTY(Replicated)
	TArray<TSubclassOf<UFGResearchTree>> mRemovedResearchTrees;

	/** Removed schematics which where CDO modified. */
	UPROPERTY(Replicated)
	TArray<TSubclassOf<UFGSchematic>> mRemovedSchematics;

	/** Cleaned schematics which where CDO modified. */
	UPROPERTY(Replicated)
	TArray<TSubclassOf<UFGSchematic>> mCleanedSchematics;

	/** Cached CDOs to prevent garbage collection. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> mCachedCDO;
};
