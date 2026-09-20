#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PMProductionBoostStatics.generated.h"

class UFGRecipe;

UCLASS()
class PERIODICMADNESS_API UPMProductionBoostStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Periodic Madness|Production Boost", meta = (WorldContext = "WorldContextObject"))
	static bool IsProductionBoostDisabled(const UObject* WorldContextObject, const TSubclassOf<UFGRecipe> Recipe);
};
