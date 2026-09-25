#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Resources/FGExtractableResourceInterface.h"
#include "FGSaveInterface.h"
#include "PMSeawaterVolume.generated.h"

class AFGWaterVolume;

/**
 * Replacement for water volumes that are considered seawater.
 */
UCLASS()
class PERIODICMADNESS_API APMSeawaterVolume : public AActor, public IFGSaveInterface, public IFGExtractableResourceInterface
{
	GENERATED_BODY()

public:
	APMSeawaterVolume();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetSeawaterResourceClass(TSubclassOf<class UFGResourceDescriptor> ResourceClass);

	//~ Begin IFGSaveInterface Interface
	virtual bool ShouldSave_Implementation() const override;
	virtual bool NeedTransform_Implementation() override;
	//~ End IFGSaveInterface Interface

	//~ Begin IFGExtractableResourceInterface Interface
	virtual void SetIsOccupied(bool Occupied) override;
	virtual bool IsOccupied() const override;
	virtual bool CanBecomeOccupied() const override;
	virtual bool HasAnyResources() const override;
	virtual TSubclassOf<UFGResourceDescriptor> GetResourceClass() const override;
	virtual bool DoesContainResource(TSubclassOf<UFGResourceDescriptor> ResourceClass) const;
	virtual int32 ExtractResource(int32 Amount) override;
	virtual float GetExtractionSpeedMultiplier() const override;
	virtual FVector GetPlacementLocation(const FVector& HitLocation) const override;
	virtual FRotator GetPlacementRotation(const FVector& HitLocation) const override;
	virtual bool CanPlaceResourceExtractor() const override;
	//~ End IFGExtractableResourceInterface Interface

private:
	/** Reference to the Seawater Descriptor. */
	UPROPERTY(SaveGame, Replicated)
	TSubclassOf<UFGResourceDescriptor> mSeawaterResourceClass;
};
