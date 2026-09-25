#include "Holograms/PMSeawaterPumpHologram.h"
#include "Buildables/PMBuildableSeawaterPump.h"
#include "Resources/PMSeawaterVolume.h"
#include "FGWaterVolume.h"
#include "PeriodicMadnessLogChannels.h"

APMSeawaterPumpHologram::APMSeawaterPumpHologram()
	: Super()
{
	mGridSnapSize = 200.f;
}

void APMSeawaterPumpHologram::ConfigureActor(AFGBuildable* inBuildable) const
{
	AFGWaterVolume* WaterVolume = CastChecked<AFGWaterVolume>(mSnappedExtractableResource.GetObject());
	APMBuildableSeawaterPump* SeawaterPump = CastChecked<APMBuildableSeawaterPump>(inBuildable);

	FActorSpawnParameters SeawaterVolumeSpawnParameters;
	SeawaterVolumeSpawnParameters.Owner = nullptr;
	SeawaterVolumeSpawnParameters.Instigator = nullptr;
	SeawaterVolumeSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APMSeawaterVolume* SeawaterVolume = GetWorld()->SpawnActor<APMSeawaterVolume>(APMSeawaterVolume::StaticClass(), GetActorLocation(), GetActorRotation(), SeawaterVolumeSpawnParameters);
	SeawaterVolume->SetSeawaterResourceClass(SeawaterPump->mSeawaterResourceClass);

	FAttachmentTransformRules AttachmentRules = FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, true);
	SeawaterVolume->AttachToActor(SeawaterPump, AttachmentRules);

	PM_LOG_ARGS(Verbose, TEXT("Spawned SeawaterVolume %s for SeawaterPump %s"), *GetPathNameSafe(SeawaterVolume), *GetPathNameSafe(SeawaterPump));

	SeawaterPump->SetExtractableResource(SeawaterVolume);
}
