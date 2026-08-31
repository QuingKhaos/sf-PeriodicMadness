#include "World/PMCleanerWorldModule.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Resources/FGResourceNodeBase.h"
#include "PeriodicMadnessLogChannels.h"

void UPMCleanerWorldModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
    if (Phase == ELifecyclePhase::CONSTRUCTION) {
        RegisterConstructionPhaseContent();
    }

    if (Phase == ELifecyclePhase::INITIALIZATION) {
        RegisterDefaultContent();
    }

	if (Phase == ELifecyclePhase::CONSTRUCTION) {
		RemoveResourceNodes();
	}

    // Blueprint event logic should be dispatched after our code.
    Super::DispatchLifecycleEvent(Phase);
}

void UPMCleanerWorldModule::RemoveResourceNodes()
{
	TArray<AActor*> ResourceNodeActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFGResourceNodeBase::StaticClass(), ResourceNodeActors);

	for (AActor* Actor : ResourceNodeActors) {
		AFGResourceNodeBase* ResourceNode = Cast<AFGResourceNodeBase>(Actor);
		if (ResourceNode) {
			FString ResourceClassName = UKismetSystemLibrary::GetPathName(ResourceNode->GetResourceClass());
			bool bIsAllowlisted = false;

			for (const FString& AllowlistEntry : mResourceClassAllowlist) {
				if (ResourceClassName.Contains(AllowlistEntry, ESearchCase::CaseSensitive)) {
					bIsAllowlisted = true;
					break;
				}
			}

			if (!bIsAllowlisted) {
				PM_LOG_ARGS(Verbose, TEXT("Removing resource: %s, Node: %s"), *ResourceClassName, *UKismetSystemLibrary::GetPathName(ResourceNode));

				AActor* MeshActor = ResourceNode->GetMeshActor();
				if (MeshActor) {
					MeshActor->Destroy();
				}
			
				ResourceNode->Destroy();
			}
		}
	}

	K2_ResourceNodesRemoved();
}
