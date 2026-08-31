#include "World/PMCleanerWorldModule.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Resources/FGResourceNodeBase.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "FGResearchManager.h"
#include "FGResearchTree.h"
#include "PeriodicMadnessLogChannels.h"

void UPMCleanerWorldModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
    if (Phase == ELifecyclePhase::CONSTRUCTION)
    {
        RegisterConstructionPhaseContent();
    }

    if (Phase == ELifecyclePhase::INITIALIZATION)
	{
        RegisterDefaultContent();
    }

	if (Phase == ELifecyclePhase::POST_INITIALIZATION)
	{
		RemoveResourceNodes();
		RemoveResearchTrees();
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
		if (ResourceNode)
		{
			FString ResourceClassName = UKismetSystemLibrary::GetPathName(ResourceNode->GetResourceClass());
			bool bIsAllowlisted = false;

			for (const FString& AllowlistEntry : mResourceClassAllowlist)
			{
				if (ResourceClassName.Contains(AllowlistEntry, ESearchCase::CaseSensitive))
				{
					bIsAllowlisted = true;
					break;
				}
			}

			if (!bIsAllowlisted)
			{
				PM_LOG_ARGS(Verbose, TEXT("Removing resource: %s, Node: %s"), *ResourceClassName, *UKismetSystemLibrary::GetPathName(ResourceNode));

				AActor* MeshActor = ResourceNode->GetMeshActor();
				if (MeshActor)
				{
					MeshActor->Destroy();
				}
			
				ResourceNode->Destroy();
			}
		}
	}

	K2_ResourceNodesRemoved();
}

void UPMCleanerWorldModule::RemoveResearchTrees()
{
	AFGResearchManager* ResearchManager = AFGResearchManager::Get(GetWorld());
	UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

	AssetDataSubsystem->EnsureRegistryScanned();
	AssetDataSubsystem->EnsureCategoryResolved(10);

	for (TSubclassOf<UFGResearchTree> ResearchTree : AssetDataSubsystem->GetAllResearchTrees())
	{
		if (ResearchTree && ResearchManager->mAvailableResearchTrees.Contains(ResearchTree))
		{
			FString ResearchTreeClassName = UKismetSystemLibrary::GetPathName(ResearchTree);
			bool bIsAllowlisted = false;

			for (const FString& AllowlistEntry : mResearchTreeClassAllowlist)
			{
				if (ResearchTreeClassName.Contains(AllowlistEntry, ESearchCase::CaseSensitive)) {
					bIsAllowlisted = true;
					break;
				}
			}

			if (!bIsAllowlisted)
			{
				PM_LOG_ARGS(Verbose, TEXT("Removing research tree: %s"), *ResearchTreeClassName);
				ResearchManager->mAvailableResearchTrees.Remove(ResearchTree);

				if (ResearchManager->mUnlockedResearchTrees.Contains(ResearchTree))
				{
					ResearchManager->mUnlockedResearchTrees.Remove(ResearchTree);
				}

				UFGResearchTree* ResearchTreeCDO = GetMutableDefault<UFGResearchTree>(ResearchTree);
				ResearchTreeCDO->mPreUnlockDisplayName = FText();
				ResearchTreeCDO->mDisplayName = FText();
				ResearchTreeCDO->mPreUnlockDescription = FText();
				ResearchTreeCDO->mPostUnlockDescription = FText();
			}
		}
	}
}
