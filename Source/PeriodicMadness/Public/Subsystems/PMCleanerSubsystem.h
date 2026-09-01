#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "PMCleanerSubsystem.generated.h"

class AFGDropPod;

/**
 * Subsystem for resource cleanup during runtime. 
 */
UCLASS()
class PERIODICMADNESS_API APMCleanerSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	APMCleanerSubsystem();

	/** Update the resource scanner to pick up any changes. */
	UFUNCTION(BlueprintCallable, Category = "Periodic Madness|Resource Cleaner")
	void UpdateResourceScanner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Handle removal/cleanup of things when the streaming state is updated. */
	void OnStreamingStateUpdated();

	/** Remove all static meshes on the cleanlist. */
	void RemoveStaticMeshes();

	/** Remove all resource deposits, except allowlisted ones. */
	void RemoveResourceDeposits();

	/** Cleanup crash sites. */
	void CleanupCrashSites();

	/** Cleanup dropped items around crash sites. */
	void CleanupDroppedItems(AFGDropPod* DropPod);

	/** List of static meshes to be cleaned up */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Resource Cleanup")
	TArray<TObjectPtr<UStaticMesh>> mStaticMeshesCleanlist;

	/** List of resource class substrings to block from removal. */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Resource Cleanup")
	TArray<FString> mResourceClassAllowlist;
};
