#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildableWaterPump.h"
#include "PMBuildableSeawaterPump.generated.h"

class UFGResourceDescriptor;

/**
 * Specialized water pump that can extract seawater from water volumes.
 */
UCLASS()
class PERIODICMADNESS_API APMBuildableSeawaterPump : public AFGBuildableWaterPump
{
	GENERATED_BODY()
	
public:
	//~ Begin UObject Interface
	virtual void OnConstruction(const FTransform& Transform) override;
	//~ End UObject Interface

private:
	/** Reference to the Seawater Descriptor to set on the Seawater Volume */
	UPROPERTY(EditDefaultsOnly, Category = "Extraction")
	TSubclassOf<UFGResourceDescriptor> mSeawaterResourceClass;
};
