#include "Resources/PMSeawaterVolume.h"
#include "Net/UnrealNetwork.h"
#include "Resources/FGResourceDescriptor.h"
#include "FGWaterVolume.h"

APMSeawaterVolume::APMSeawaterVolume()
	: Super()
{
	bReplicates = true;
	bAlwaysRelevant = true;
}

void APMSeawaterVolume::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APMSeawaterVolume, mSeawaterResourceClass);
}

void APMSeawaterVolume::SetSeawaterResourceClass(TSubclassOf<class UFGResourceDescriptor> ResourceClass)
{
	mSeawaterResourceClass = ResourceClass;
}

//~ Begin IFGSaveInterface Interface
bool APMSeawaterVolume::ShouldSave_Implementation() const
{
	return true;
}

bool APMSeawaterVolume::NeedTransform_Implementation()
{
	return true;
}
//~ End IFGSaveInterface Interface

//~ Begin IFGExtractableResourceInterface Interface
void APMSeawaterVolume::SetIsOccupied(bool Occupied)
{
	// no-op
}

bool APMSeawaterVolume::IsOccupied() const
{
	return false;
}

bool APMSeawaterVolume::CanBecomeOccupied() const
{
	return true;
}

bool APMSeawaterVolume::HasAnyResources() const
{
	return true;
}

TSubclassOf<UFGResourceDescriptor> APMSeawaterVolume::GetResourceClass() const
{
	return mSeawaterResourceClass;
}

bool APMSeawaterVolume::DoesContainResource(TSubclassOf<UFGResourceDescriptor> ResourceClass) const
{
	return ResourceClass == mSeawaterResourceClass;
}

int32 APMSeawaterVolume::ExtractResource(int32 Amount)
{
	return Amount;
}

float APMSeawaterVolume::GetExtractionSpeedMultiplier() const
{
	return UFGResourceDescriptor::GetCollectSpeedMultiplier(mSeawaterResourceClass);
}

FVector APMSeawaterVolume::GetPlacementLocation(const FVector& HitLocation) const
{
	return GetActorLocation();
}

FRotator APMSeawaterVolume::GetPlacementRotation(const FVector& HitLocation) const
{
	return GetActorRotation();
}

bool APMSeawaterVolume::CanPlaceResourceExtractor() const
{
	return true;
}
//~ End IFGExtractableResourceInterface Interface
