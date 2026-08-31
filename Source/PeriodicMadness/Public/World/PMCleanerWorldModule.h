

#pragma once

#include "CoreMinimal.h"
#include "Module/GameWorldModule.h"
#include "PMCleanerWorldModule.generated.h"

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

	/** Called after resource nodes have been removed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Periodic Madness|Resource Cleanup", meta = (DisplayName = "On Resource Nodes Removed"))
	void K2_ResourceNodesRemoved();

	/** List of resource class substrings to block from removal. */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Resource Cleanup")
	TArray<FString> mResourceClassAllowlist;
};
