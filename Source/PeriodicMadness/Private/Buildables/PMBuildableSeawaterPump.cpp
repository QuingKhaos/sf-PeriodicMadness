#include "Buildables/PMBuildableSeawaterPump.h"
#include "Resources/PMSeawaterVolume.h"

void APMBuildableSeawaterPump::OnConstruction(const FTransform& Transform)
{
	AFGBuildableWaterPump::OnConstruction(Transform);

	if (AFGWaterVolume* WaterVolume = Cast<AFGWaterVolume>(GetExtractableResourceActor()))
	{
		FActorSpawnParameters SeawaterVolumeSpawnParameters;
		SeawaterVolumeSpawnParameters.Owner = this;
		SeawaterVolumeSpawnParameters.bAllowDuringConstructionScript = true;
		SeawaterVolumeSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APMSeawaterVolume* SeawaterVolume = GetWorld()->SpawnActor<APMSeawaterVolume>(APMSeawaterVolume::StaticClass(), Transform.GetLocation(), Transform.GetRotation().Rotator(), SeawaterVolumeSpawnParameters);
		SeawaterVolume->SetDecoratedWaterVolume(WaterVolume);
		SeawaterVolume->SetSeawaterResourceClass(mSeawaterResourceClass);
		SeawaterVolume->SetActorHiddenInGame(true);
		SeawaterVolume->SetActorEnableCollision(false);
		SeawaterVolume->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

		SetExtractableResource(SeawaterVolume);
	}
}
