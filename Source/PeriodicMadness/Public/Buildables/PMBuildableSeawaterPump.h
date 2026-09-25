#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildableWaterPump.h"
#include "Resources/FGResourceDescriptor.h"
#include "PMBuildableSeawaterPump.generated.h"

/**
 * Specialized water pump that can extract seawater from water volumes.
 */
UCLASS()
class PERIODICMADNESS_API APMBuildableSeawaterPump : public AFGBuildableWaterPump
{
	GENERATED_BODY()

public:
	//~ Begin AActor Interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

private:
	friend class APMSeawaterPumpHologram;

	/** Reference to the Seawater Descriptor to set on the Seawater Volume */
	UPROPERTY(EditDefaultsOnly, Category = "Extraction")
	TSubclassOf<UFGResourceDescriptor> mSeawaterResourceClass;
};
