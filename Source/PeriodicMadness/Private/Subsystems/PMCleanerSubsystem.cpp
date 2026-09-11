#include "Subsystems/PMCleanerSubsystem.h"
#include "Engine/AssetUserData.h"
#include "Engine/StaticMeshActor.h"
#include "Equipment/FGResourceScanner.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Resources/FGResourceDeposit.h"
#include "Settings/PMCleanerSettings.h"
#include "FGCharacterPlayer.h"
#include "FGDropPod.h"
#include "FGFoliagePickup.h"
#include "FGFoliageResourceUserData.h"
#include "FGItemPickup_Spawnable.h"
#include "ItemDrop.h"
#include "PeriodicMadnessLogChannels.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

APMCleanerSubsystem::APMCleanerSubsystem()
{
	PrimaryActorTick.bCanEverTick = false;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
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

void APMCleanerSubsystem::BeginPlay()
{
	Super::BeginPlay();

	RemoveStaticMeshes();
	RemoveResourceDeposits();
	ReplaceResourceDeposits();
	RemoveFoliageItemDrops();
	ReplaceFoliageItemDrops();
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
			UStaticMesh* Mesh = StaticMeshActor->GetStaticMeshComponent()->GetStaticMesh();
			if (Mesh && CleanerSettings->ShouldRemoveStaticMesh(Mesh))
			{
				PM_LOG_ARGS(Verbose, TEXT("Destroying static mesh actor: %s, Mesh: %s"), *UKismetSystemLibrary::GetPathName(StaticMeshActor), *UKismetSystemLibrary::GetPathName(Mesh));
				StaticMeshActor->Destroy();
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
		if (ResourceDeposit)
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
		if (ResourceDeposit)
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
