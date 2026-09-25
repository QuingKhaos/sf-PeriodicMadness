#pragma once

#include "CoreMinimal.h"
#include "Hologram/FGWaterPumpHologram.h"
#include "PMSeawaterPumpHologram.generated.h"

/**
 * Replaces the water volume with a decorator, which provides the seawater resource class.
 */
UCLASS()
class PERIODICMADNESS_API APMSeawaterPumpHologram : public AFGWaterPumpHologram
{
	GENERATED_BODY()

public:
	APMSeawaterPumpHologram();

protected:
	// Begin AFGBuildableHologram Interface
	virtual void ConfigureActor(class AFGBuildable* inBuildable) const override;
	// End AFGBuildableHologram Interface
};
