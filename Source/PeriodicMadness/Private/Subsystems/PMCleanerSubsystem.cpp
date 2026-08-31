#include "Subsystems/PMCleanerSubsystem.h"
#include "Engine/StaticMeshActor.h"
#include "Equipment/FGResourceScanner.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Resources/FGResourceDeposit.h"
#include "FGCharacterPlayer.h"
#include "PeriodicMadnessLogChannels.h"

APMCleanerSubsystem::APMCleanerSubsystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	PrimaryActorTick.TickInterval = 1.f;

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
}

void APMCleanerSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RemoveStaticMeshes();
	RemoveResourceDeposits();
}

void APMCleanerSubsystem::RemoveStaticMeshes()
{
	TArray<AActor*> StaticMeshActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStaticMeshActor::StaticClass(), StaticMeshActors);

	for (AActor* Actor : StaticMeshActors)
	{
		AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Actor);
		if (StaticMeshActor && StaticMeshActor->GetStaticMeshComponent())
		{
			UStaticMesh* Mesh = StaticMeshActor->GetStaticMeshComponent()->GetStaticMesh();
			if (Mesh && mStaticMeshesCleanlist.Contains(Mesh))
			{
				PM_LOG_ARGS(Verbose, TEXT("Destroying static mesh actor: %s, Mesh: %s"), *UKismetSystemLibrary::GetPathName(StaticMeshActor), *UKismetSystemLibrary::GetPathName(Mesh));
				StaticMeshActor->Destroy();
			}
		}
	}
}

void APMCleanerSubsystem::RemoveResourceDeposits()
{
	TArray<AActor*> ResourceDepositActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFGResourceDeposit::StaticClass(), ResourceDepositActors);

	for (AActor* Actor : ResourceDepositActors)
	{
		AFGResourceDeposit* ResourceDeposit = Cast<AFGResourceDeposit>(Actor);
		if (ResourceDeposit)
		{
			FString ResourceClassName = UKismetSystemLibrary::GetPathName(ResourceDeposit->GetResourceClass());
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
				PM_LOG_ARGS(Verbose, TEXT("Removing resource: %s, Deposit: %s"), *ResourceClassName, *UKismetSystemLibrary::GetPathName(ResourceDeposit));

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
