#include "Subsystems/PMCleanerSubsystem.h"
#include "Engine/AssetUserData.h"
#include "Engine/StaticMeshActor.h"
#include "Equipment/FGResourceScanner.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Resources/FGResourceDeposit.h"
#include "Settings/PMCleanerSettings.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Unlocks/FGUnlock.h"
#include "Unlocks/FGUnlockRecipe.h"
#include "Unlocks/FGUnlockScannableResource.h"
#include "WorldPartition/WorldPartitionSubsystem.h"
#include "FGCharacterPlayer.h"
#include "FGDropPod.h"
#include "FGFoliagePickup.h"
#include "FGFoliageResourceUserData.h"
#include "FGItemPickup_Spawnable.h"
#include "FGResearchTree.h"
#include "FGSchematic.h"
#include "FGWaterVolume.h"
#include "ItemDrop.h"
#include "PeriodicMadnessLogChannels.h"

APMCleanerSubsystem::APMCleanerSubsystem()
{
	PrimaryActorTick.bCanEverTick = false;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

APMCleanerSubsystem* APMCleanerSubsystem::Get(UWorld* World)
{
	USubsystemActorManager* SubsystemActorManager = World->GetSubsystem<USubsystemActorManager>();
	check(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<APMCleanerSubsystem>();
}

APMCleanerSubsystem* APMCleanerSubsystem::Get(const UObject* WorldContext)
{
	UWorld* WorldObject = GEngine->GetWorldFromContextObjectChecked(WorldContext);
	return Get(WorldObject);
}

void APMCleanerSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APMCleanerSubsystem, mRemovedResearchTrees);
	DOREPLIFETIME(APMCleanerSubsystem, mRemovedSchematics);
	DOREPLIFETIME(APMCleanerSubsystem, mCleanedSchematics);
}

void APMCleanerSubsystem::UpdateResourceScanner()
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (const APlayerController* PC = Iterator->Get())
		{
			if (AFGCharacterPlayer* Player = Cast<AFGCharacterPlayer>(PC->GetPawn()))
			{
				if (AFGResourceScanner* Scanner = Player->GetResourceScanner())
				{
					PM_LOG_ARGS(Verbose, TEXT("Updating resource scanner for player %s"), *UKismetSystemLibrary::GetPathName(Player));

					Scanner->mNodeClusters.Empty();
					Scanner->GenerateNodeClusters();
				}
			}
		}
	}
}

void APMCleanerSubsystem::ResearchTreeRemovalModifyCDO(const TSubclassOf<UFGResearchTree> ResearchTree, bool bReplay)
{
	PM_LOG_ARGS(Verbose, TEXT("Modifying research tree CDO: %s"), *UKismetSystemLibrary::GetPathName(ResearchTree));

	UFGResearchTree* ResearchTreeCDO = GetMutableDefault<UFGResearchTree>(ResearchTree);
	ResearchTreeCDO->mPreUnlockDisplayName = FText();
	ResearchTreeCDO->mDisplayName = FText();
	ResearchTreeCDO->mPreUnlockDescription = FText();
	ResearchTreeCDO->mPostUnlockDescription = FText();

	if (!bReplay)
	{
		mRemovedResearchTrees.Add(ResearchTree);
	}

	mCachedCDO.Add(ResearchTreeCDO);
}

void APMCleanerSubsystem::SchematicRemovalModifyCDO(const TSubclassOf<UFGSchematic> Schematic, bool bReplay)
{
	PM_LOG_ARGS(Verbose, TEXT("Modifying schematic CDO: %s"), *UKismetSystemLibrary::GetPathName(Schematic));

	UFGSchematic* SchematicCDO = GetMutableDefault<UFGSchematic>(Schematic);
	SchematicCDO->mType = ESchematicType::EST_Custom;

	TArray<UFGUnlock*> UnlocksToRemove;
	UnlocksToRemove.Append(SchematicCDO->mUnlocks);

	for (UFGUnlock* Unlock : UnlocksToRemove)
	{
		if (Unlock)
		{
			if (UFGUnlockRecipe* UnlockRecipe = Cast<UFGUnlockRecipe>(Unlock))
			{
				UnlockRecipe->mRecipes.Empty();
			}

			if (UFGUnlockScannableResource* UnlockScannableResource = Cast<UFGUnlockScannableResource>(Unlock))
			{
				UnlockScannableResource->mResourcePairsToAddToScanner.Empty();
			}

			SchematicCDO->mUnlocks.Remove(Unlock);
			mCachedCDO.Add(Unlock);
		}
	}

	if (!bReplay)
	{
		mRemovedSchematics.Add(Schematic);
	}

	mCachedCDO.Add(SchematicCDO);
}

void APMCleanerSubsystem::SchematicCleanup(const TSubclassOf<UFGSchematic> Schematic, bool bReplay)
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();

	FPMSchematicCleanup CleanupData;
	if (CleanerSettings->ShouldCleanupSchematic(Schematic, CleanupData))
	{
		UFGSchematic* SchematicCDO = GetMutableDefault<UFGSchematic>(Schematic);
		for (UFGUnlock* Unlock : SchematicCDO->mUnlocks)
		{
			if (Unlock)
			{
				if (UFGUnlockScannableResource* UnlockScannableResource = Cast<UFGUnlockScannableResource>(Unlock))
				{
					TArray<FScannableResourcePair> ResourcePairsToRemove;
					ResourcePairsToRemove.Append(UnlockScannableResource->mResourcePairsToAddToScanner);

					for (const FScannableResourcePair& ResourcePair : ResourcePairsToRemove)
					{
						for (const TSubclassOf<UFGResourceDescriptor>& ResourceClass : CleanupData.ScannableResourceClassCleanlist)
						{
							if (ResourcePair.ResourceDescriptor == ResourceClass)
							{
								PM_LOG_ARGS(Verbose, TEXT("Removing scannable resource: %s from schematic: %s"), *UKismetSystemLibrary::GetPathName(ResourceClass), *UKismetSystemLibrary::GetPathName(Schematic));
								UnlockScannableResource->mResourcePairsToAddToScanner.Remove(ResourcePair);
								break;
							}
						}
					}
				}
			}
		}

		if (!bReplay)
		{
			mCleanedSchematics.Add(Schematic);
		}

		mCachedCDO.Add(SchematicCDO);
	}
}

void APMCleanerSubsystem::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		ReplayCDOModifications();
	}

	RemoveStaticMeshes();
	RemoveResourceDeposits();
	ReplaceResourceDeposits();
	RemoveFoliageItemDrops();
	ReplaceFoliageItemDrops();
	ReplaceWaterVolumes();
	CleanupCrashSites();

	UWorldPartitionSubsystem* WorldPartitionSubsystem = GetWorld()->GetSubsystem<UWorldPartitionSubsystem>();
	if (WorldPartitionSubsystem)
	{
		WorldPartitionSubsystem->OnStreamingStateUpdated().AddUObject(this, &APMCleanerSubsystem::OnStreamingStateUpdated);
	}
}

void APMCleanerSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UWorldPartitionSubsystem* WorldPartitionSubsystem = GetWorld()->GetSubsystem<UWorldPartitionSubsystem>();
	if (WorldPartitionSubsystem)
	{
		WorldPartitionSubsystem->OnStreamingStateUpdated().RemoveAll(this);
	}
}

void APMCleanerSubsystem::OnStreamingStateUpdated()
{
	RemoveStaticMeshes();
	RemoveResourceDeposits();
	ReplaceResourceDeposits();
	RemoveFoliageItemDrops();
	ReplaceFoliageItemDrops();
	CleanupCrashSites();
}

void APMCleanerSubsystem::ReplayCDOModifications()
{
	for (const TSubclassOf<UFGResearchTree>& ResearchTree : mRemovedResearchTrees)
	{
		ResearchTreeRemovalModifyCDO(ResearchTree, true);
	}

	for (const TSubclassOf<UFGSchematic>& Schematic : mRemovedSchematics)
	{
		SchematicRemovalModifyCDO(Schematic, true);
	}

	for (const TSubclassOf<UFGSchematic>& Schematic : mCleanedSchematics)
	{
		SchematicCleanup(Schematic, true);
	}
}

void APMCleanerSubsystem::RemoveStaticMeshes()
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();

	TArray<AActor*> StaticMeshActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStaticMeshActor::StaticClass(), StaticMeshActors);

	for (AActor* Actor : StaticMeshActors)
	{
		AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Actor);
		if (StaticMeshActor && StaticMeshActor->GetStaticMeshComponent())
		{
			if (UStaticMesh* Mesh = StaticMeshActor->GetStaticMeshComponent()->GetStaticMesh())
			{
				if (CleanerSettings->ShouldRemoveStaticMesh(Mesh))
				{
					PM_LOG_ARGS(Verbose, TEXT("Destroying static mesh actor: %s, Mesh: %s"), *UKismetSystemLibrary::GetPathName(StaticMeshActor), *UKismetSystemLibrary::GetPathName(Mesh));
					StaticMeshActor->Destroy();
				}
				else
				{
					TArray<UMaterialInterface*> UsedMaterials = StaticMeshActor->GetStaticMeshComponent()->GetMaterials();
					if (CleanerSettings->ShouldRemoveStaticMeshByMaterial(Mesh, UsedMaterials))
					{
						PM_LOG_ARGS(Verbose, TEXT("Destroying static mesh actor by material: %s, Mesh: %s"), *UKismetSystemLibrary::GetPathName(StaticMeshActor), *UKismetSystemLibrary::GetPathName(Mesh));
						StaticMeshActor->Destroy();
					}
				}
			}
		}
	}
}

void APMCleanerSubsystem::RemoveResourceDeposits()
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();

	TArray<AActor*> ResourceDepositActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFGResourceDeposit::StaticClass(), ResourceDepositActors);

	for (AActor* Actor : ResourceDepositActors)
	{
		AFGResourceDeposit* ResourceDeposit = Cast<AFGResourceDeposit>(Actor);
		if (ResourceDeposit && ResourceDeposit->GetResourceClass())
		{
			if (CleanerSettings->ShouldRemoveResourceClass(ResourceDeposit->GetResourceClass()))
			{
				PM_LOG_ARGS(Verbose, TEXT("Removing resource: %s, Deposit: %s"), *UKismetSystemLibrary::GetPathName(ResourceDeposit->GetResourceClass()), *UKismetSystemLibrary::GetPathName(ResourceDeposit));

				AActor* MeshActor = ResourceDeposit->GetMeshActor();
				if (MeshActor)
				{
					MeshActor->Destroy();
				}

				ResourceDeposit->Destroy();
			}
		}
	}
}

void APMCleanerSubsystem::ReplaceResourceDeposits()
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();

	TArray<AActor*> ResourceDepositActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFGResourceDeposit::StaticClass(), ResourceDepositActors);

	for (AActor* Actor : ResourceDepositActors)
	{
		AFGResourceDeposit* ResourceDeposit = Cast<AFGResourceDeposit>(Actor);
		if (ResourceDeposit && ResourceDeposit->GetResourceClass())
		{
			TSubclassOf<UFGResourceDescriptor> ReplacementResourceClass;
			if (CleanerSettings->ShouldReplaceResourceClass(ResourceDeposit->GetResourceClass(), ReplacementResourceClass))
			{
				PM_LOG_ARGS(Verbose, TEXT("Replacing resource: %s with %s, Deposit: %s"), *UKismetSystemLibrary::GetPathName(ResourceDeposit->GetResourceClass()), *UKismetSystemLibrary::GetPathName(ReplacementResourceClass), *UKismetSystemLibrary::GetPathName(ResourceDeposit));
				ResourceDeposit->SetResourceClass(ReplacementResourceClass);
			}
		}
	}
}

void APMCleanerSubsystem::RemoveFoliageItemDrops()
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();

	TArray<AActor*> FoliagePickupActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFGFoliagePickup::StaticClass(), FoliagePickupActors);

	for (AActor* Actor : FoliagePickupActors)
	{
		AFGFoliagePickup* FoliagePickup = Cast<AFGFoliagePickup>(Actor);
		if (FoliagePickup)
		{
			if (UHierarchicalInstancedStaticMeshComponent* PickupComponent = FoliagePickup->GetPickupComponent().Get())
			{
				if (UStaticMesh* Mesh = PickupComponent->GetStaticMesh())
				{
					if (UAssetUserData* UserData = Mesh->GetAssetUserDataOfClass(UFGFoliageResourceUserData::StaticClass()))
					{
						if (UFGFoliageResourceUserData* FoliageResourceUserData = Cast<UFGFoliageResourceUserData>(UserData))
						{
							TArray<FItemDropWithChance> PickupItems;
							PickupItems.Append(FoliageResourceUserData->mPickupItems);

							for (int32 i = PickupItems.Num() - 1; i >= 0; --i)
							{
								const FItemDropWithChance& ItemDrop = PickupItems[i];
								if (CleanerSettings->ShouldRemoveFoliageItemClass(ItemDrop.Drop.ItemClass))
								{
									PM_LOG_ARGS(Verbose, TEXT("Removing foliage item drop: %s, FoliagePickup: %s, Mesh: %s"), *UKismetSystemLibrary::GetPathName(ItemDrop.Drop.ItemClass), *UKismetSystemLibrary::GetPathName(FoliagePickup), *UKismetSystemLibrary::GetPathName(Mesh));
									FoliageResourceUserData->mPickupItems.RemoveAt(i);
								}
							}
						}
					}
				}
			}
		}
	}
}

void APMCleanerSubsystem::ReplaceFoliageItemDrops()
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();

	TArray<AActor*> FoliagePickupActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFGFoliagePickup::StaticClass(), FoliagePickupActors);

	for (AActor* Actor : FoliagePickupActors)
	{
		AFGFoliagePickup* FoliagePickup = Cast<AFGFoliagePickup>(Actor);
		if (FoliagePickup)
		{
			if (UHierarchicalInstancedStaticMeshComponent* PickupComponent = FoliagePickup->GetPickupComponent().Get())
			{
				if (UStaticMesh* Mesh = PickupComponent->GetStaticMesh())
				{
					if (UAssetUserData* UserData = Mesh->GetAssetUserDataOfClass(UFGFoliageResourceUserData::StaticClass()))
					{
						if (UFGFoliageResourceUserData* FoliageResourceUserData = Cast<UFGFoliageResourceUserData>(UserData))
						{
							for (FItemDropWithChance& ItemDrop : FoliageResourceUserData->mPickupItems)
							{
								TSubclassOf<UFGItemDescriptor> ReplacementItemClass;
								if (CleanerSettings->ShouldReplaceFoliageItemClass(ItemDrop.Drop.ItemClass, ReplacementItemClass))
								{
									PM_LOG_ARGS(Verbose, TEXT("Replacing foliage item drop: %s with %s, FoliagePickup: %s, Mesh: %s"), *UKismetSystemLibrary::GetPathName(ItemDrop.Drop.ItemClass), *UKismetSystemLibrary::GetPathName(ReplacementItemClass), *UKismetSystemLibrary::GetPathName(FoliagePickup), *UKismetSystemLibrary::GetPathName(Mesh));
									ItemDrop.Drop.ItemClass = ReplacementItemClass;
								}
							}
						}
					}
				}
			}
		}
	}
}

void APMCleanerSubsystem::ReplaceWaterVolumes()
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();

	TArray<AActor*> WaterVolumeActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFGWaterVolume::StaticClass(), WaterVolumeActors);

	for (AActor* Actor : WaterVolumeActors)
	{
		AFGWaterVolume* WaterVolume = Cast<AFGWaterVolume>(Actor);
		if (WaterVolume)
		{
			TSubclassOf<UFGResourceDescriptor> ReplacementResourceClass;
			if (CleanerSettings->ShouldReplaceWaterVolumeResourceClass(WaterVolume->mResourceClass, ReplacementResourceClass))
			{
				PM_LOG_ARGS(Verbose, TEXT("Replacing water volume resource class: %s with %s, WaterVolume: %s"), *UKismetSystemLibrary::GetPathName(WaterVolume->mResourceClass), *UKismetSystemLibrary::GetPathName(ReplacementResourceClass), *UKismetSystemLibrary::GetPathName(WaterVolume));
				WaterVolume->mResourceClass = ReplacementResourceClass;
			}
		}
	}
}

void APMCleanerSubsystem::CleanupCrashSites()
{
	TArray<AActor*> DropPodActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFGDropPod::StaticClass(), DropPodActors);

	for (AActor* Actor : DropPodActors) {
		AFGDropPod* DropPod = Cast<AFGDropPod>(Actor);
		if (DropPod)
		{
			if (DropPod->mUnlockCost.CostType != EFGDropPodUnlockCostType::None)
			{
				PM_LOG_ARGS(Verbose, TEXT("Resetting drop pod unlock cost for: %s"), *UKismetSystemLibrary::GetPathName(DropPod));
				DropPod->mUnlockCost = FFGDropPodUnlockCost();
			}

			for (AFGItemPickup_Spawnable* ItemPickup : DropPod->mSpawnedPickups)
			{
				if (ItemPickup)
				{
					PM_LOG_ARGS(Verbose, TEXT("Destroying drop pod spawned item pickup: %s"), *UKismetSystemLibrary::GetPathName(ItemPickup));
					ItemPickup->Destroy();
				}
			}

			CleanupDroppedItems(DropPod);
		}
	}
}

void APMCleanerSubsystem::CleanupDroppedItems(AFGDropPod* DropPod)
{
	FVector CenterPosition = DropPod->GetActorLocation(); // Center of the radius
	float Radius = 5000.f;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(DropPod);

	TArray<AActor*> OutActors;
	bool bResult = UKismetSystemLibrary::SphereOverlapActors(GetWorld(), CenterPosition, Radius, ObjectTypes, AFGItemPickup_Spawnable::StaticClass(), ActorsToIgnore, OutActors);

	if (bResult)
	{
		for (AActor* Actor : OutActors)
		{
			if (AFGItemPickup_Spawnable* ItemPickup = Cast<AFGItemPickup_Spawnable>(Actor))
			{
				if (!UKismetSystemLibrary::GetPathName(ItemPickup).Contains(TEXT("/PeriodicMadness/Resources/"), ESearchCase::CaseSensitive))
				{
					PM_LOG_ARGS(Verbose, TEXT("Destroying dropped item pickup: %s"), *UKismetSystemLibrary::GetPathName(ItemPickup));
					ItemPickup->Destroy();
				}
			}
		}
	}
}
