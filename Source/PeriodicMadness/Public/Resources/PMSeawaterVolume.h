#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Resources/FGExtractableResourceInterface.h"
#include "FGSaveInterface.h"
#include "PMSeawaterVolume.generated.h"

class AFGWaterVolume;

/**
 * Decorator for water volumes that are considered seawater.
 */
UCLASS()
class PERIODICMADNESS_API APMSeawaterVolume : public AActor, public IFGSaveInterface, public IFGExtractableResourceInterface
{
	GENERATED_BODY()

public:
	void SetDecoratedWaterVolume(AFGWaterVolume* DecoratedWaterVolume);
	void SetSeawaterResourceClass(TSubclassOf<class UFGResourceDescriptor> ResourceClass);

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void PostUnregisterAllComponents(void) override;
	virtual void PostRegisterAllComponents() override;
	//~ End AActor Interface

	//~ Begin IFGSaveInterface Interface
	virtual bool ShouldSave_Implementation() const override;
	virtual bool NeedTransform_Implementation() override;
	//~ End IFGSaveInterface Interface

	//~ Begin IFGExtractableResourceInterface Interface
	virtual void SetIsOccupied(bool occupied) override;
	virtual bool IsOccupied() const override;
	virtual bool CanBecomeOccupied() const override;
	virtual bool HasAnyResources() const override;
	virtual TSubclassOf<UFGResourceDescriptor> GetResourceClass() const override;
	virtual bool DoesContainResource(TSubclassOf<UFGResourceDescriptor> ResourceClass) const;
	virtual int32 ExtractResource(int32 amount) override;
	virtual float GetExtractionSpeedMultiplier() const override;
	virtual FVector GetPlacementLocation(const FVector& hitLocation) const override;
	virtual FRotator GetPlacementRotation(const FVector& hitLocation) const;
	virtual bool CanPlaceResourceExtractor() const override;
	//~ End IFGExtractableResourceInterface Interface

private:
	UPROPERTY(SaveGame)
	AFGWaterVolume* mDecoratedWaterVolume;

	/** Reference to the Seawater Descriptor. */
	UPROPERTY(SaveGame)
	TSubclassOf<UFGResourceDescriptor> mSeawaterResourceClass;
};
