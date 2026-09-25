#include "Buildables/PMBuildableSeawaterPump.h"
#include "Resources/PMSeawaterVolume.h"
#include "PeriodicMadnessLogChannels.h"

void APMBuildableSeawaterPump::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason != EEndPlayReason::RemovedFromWorld)
	{
		AActor* ExtractableResourceActor = GetExtractableResourceActor();
		if (ExtractableResourceActor && ExtractableResourceActor->IsA<APMSeawaterVolume>())
		{
			PM_LOG_ARGS(Verbose, TEXT("Destroying SeawaterVolume %s"), *GetPathNameSafe(ExtractableResourceActor));
			ExtractableResourceActor->Destroy();
		}
	}

	Super::EndPlay(EndPlayReason);
}
