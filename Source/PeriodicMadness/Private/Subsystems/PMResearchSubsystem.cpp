#include "Subsystems/PMResearchSubsystem.h"
#include "FGRecipe.h"
#include "FGResearchManager.h"
#include "FGSchematic.h"
#include "Net/UnrealNetwork.h"
#include "PeriodicMadnessLogChannels.h"

APMResearchSubsystem::APMResearchSubsystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

void APMResearchSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APMResearchSubsystem, mCurrentResearchSchematic);
	DOREPLIFETIME(APMResearchSubsystem, mOriginalResearchDuration);
}

void APMResearchSubsystem::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	AFGResearchManager* ResearchManager = AFGResearchManager::Get(GetWorld());
	ResearchManager->ResearchStartedDelegate.AddUniqueDynamic(this, &APMResearchSubsystem::HandleOnResearchStarted);
	ResearchManager->ResearchCompletedDelegate.AddUniqueDynamic(this, &APMResearchSubsystem::HandleOnResearchCompleted);
}

void APMResearchSubsystem::Tick(float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	if (mCurrentResearchSchematic)
	{
		AFGResearchManager* ResearchManager = AFGResearchManager::Get(GetWorld());
		FTimerManager& TimerManager = GetWorldTimerManager();

		for (FResearchTime& ResearchTime : ResearchManager->mOngoingResearch)
		{
			if (ResearchTime.ResearchData.Schematic == mCurrentResearchSchematic)
			{
				float AdjustedDuration = mOriginalResearchDuration / CalculateResearchSpeedMultiplier();
				float AdjustedRemainingTime = AdjustedDuration - (GetWorld()->GetTimeSeconds() - (mOriginalResearchCompleteTimestamp - mOriginalResearchDuration));
				AdjustedRemainingTime = FMath::Max(AdjustedRemainingTime, 0.f); // Ensure remaining time is not negative

				if (AdjustedRemainingTime <= KINDA_SMALL_NUMBER)
				{
					ResearchManager->OnResearchTimerComplete(mCurrentResearchSchematic);
				}
				else
				{
					float NewCompleteTimestamp = GetWorld()->GetTimeSeconds() + AdjustedRemainingTime;
					if (FMath::Abs(ResearchTime.ResearchCompleteTimestamp - NewCompleteTimestamp) > 5.f) // Only adjust if the difference is significant
					{
						ResearchTime.ResearchCompleteTimestamp = NewCompleteTimestamp;
						NetMulticast_ResearchTimeAdjusted(mCurrentResearchSchematic, AdjustedDuration);

						TimerManager.SetTimer(ResearchTime.TimerHandle, [ResearchManager, Schematic = ResearchTime.ResearchData.Schematic]()
							{
								ResearchManager->OnResearchTimerComplete(Schematic);
							}, AdjustedRemainingTime, false);
					}
				}

				break;
			}
		}
	}
}

void APMResearchSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		AFGResearchManager* ResearchManager = AFGResearchManager::Get(GetWorld());
		ResearchManager->ResearchStartedDelegate.RemoveDynamic(this, &APMResearchSubsystem::HandleOnResearchStarted);
		ResearchManager->ResearchCompletedDelegate.RemoveDynamic(this, &APMResearchSubsystem::HandleOnResearchCompleted);
	}

	if (mCurrentResearchSchematic)
	{
		if (mCurrentResearchSchematicCDO)
		{
			mCurrentResearchSchematicCDO->mTimeToComplete = mOriginalResearchDuration;
		}

		ResetResearchRecipes();
	}

	Super::EndPlay(EndPlayReason);
}

void APMResearchSubsystem::PreSaveGame_Implementation(int32 SaveVersion, int32 GameVersion)
{
	if (mCurrentResearchSchematic)
	{
		mOriginalRemainingResearchTime = mOriginalResearchCompleteTimestamp - GetWorld()->GetTimeSeconds();
	}
}

void APMResearchSubsystem::PostLoadGame_Implementation(int32 SaveVersion, int32 GameVersion)
{
	if (mCurrentResearchSchematic)
	{
		mOriginalResearchCompleteTimestamp = GetWorld()->GetTimeSeconds() + mOriginalRemainingResearchTime;
		mOriginalRemainingResearchTime = 0.f;

		float AdjustedDuration = mOriginalResearchDuration / CalculateResearchSpeedMultiplier();
		NetMulticast_ResearchTimeAdjusted(mCurrentResearchSchematic, AdjustedDuration);

		NetMulticast_UpdateResearchRecipes();
	}
}

bool APMResearchSubsystem::NeedTransform_Implementation()
{
	return false;
}

bool APMResearchSubsystem::ShouldSave_Implementation() const
{
	return true;
}

void APMResearchSubsystem::HandleOnResearchStarted(TSubclassOf<UFGSchematic> Schematic)
{
	if (!HasAuthority())
	{
		return;
	}

	bool bShouldHandle = false;
	for (const FItemAmount& ItemCost : UFGSchematic::GetCost(Schematic))
	{
		if (mSciencePacksToConsider.Contains(ItemCost.ItemClass))
		{
			bShouldHandle = true;
			break;
		}
	}

	if (bShouldHandle)
	{
		mCurrentResearchSchematic = Schematic;
		mOriginalResearchDuration = UFGSchematic::GetTimeToComplete(Schematic);

		for (const FResearchTime& ResearchTime : AFGResearchManager::Get(GetWorld())->mOngoingResearch)
		{
			if (ResearchTime.ResearchData.Schematic == Schematic)
			{
				mOriginalResearchCompleteTimestamp = ResearchTime.ResearchCompleteTimestamp;
				break;
			}
		}

		NetMulticast_UpdateResearchRecipes();
	}
}

void APMResearchSubsystem::HandleOnResearchCompleted(TSubclassOf<UFGSchematic> Schematic)
{
	if (!HasAuthority())
	{
		return;
	}

	NetMulticast_ResetResearchDuration(Schematic, mOriginalResearchDuration);
	mCurrentResearchSchematic = nullptr;

	mOriginalResearchDuration = 0.f;
	mOriginalResearchCompleteTimestamp = 0.f;
}

float APMResearchSubsystem::CalculateResearchSpeedMultiplier() const
{
	return 2.f;
}

void APMResearchSubsystem::NetMulticast_ResearchTimeAdjusted_Implementation(TSubclassOf<UFGSchematic> Schematic, float AdjustedDuration)
{
	if (mCurrentResearchSchematic == Schematic)
	{
		if (!mCurrentResearchSchematicCDO)
		{
			mCurrentResearchSchematicCDO = GetMutableDefault<UFGSchematic>(Schematic);
		}

		mCurrentResearchSchematicCDO->mTimeToComplete = AdjustedDuration;

		K2_ResearchTimeAdjusted(AdjustedDuration);
	}

}

void APMResearchSubsystem::NetMulticast_ResetResearchDuration_Implementation(TSubclassOf<UFGSchematic> Schematic, float OriginalDuration)
{
	if (mCurrentResearchSchematicCDO->GetClass() == Schematic)
	{
		mCurrentResearchSchematicCDO->mTimeToComplete = OriginalDuration;
		mCurrentResearchSchematicCDO = nullptr;
	}
}

void APMResearchSubsystem::NetMulticast_UpdateResearchRecipes_Implementation()
{
	for (const FPMResearchRecipe& ResearchRecipe : mResearchRecipes)
	{
		bool bShouldUpdate = true;
		TArray<FItemAmount> NewRecipeIngredients;
		for (const FItemAmount& ItemCost : UFGSchematic::GetCost(mCurrentResearchSchematic))
		{
			if (mSciencePacksToConsider.Contains(ItemCost.ItemClass))
			{
				if (!ResearchRecipe.AllowedSciencePacks.Contains(ItemCost.ItemClass))
				{
					bShouldUpdate = false;
					break;
				}
				else
				{
					FItemAmount NewIngredient(ItemCost.ItemClass, 1);
					NewRecipeIngredients.Add(NewIngredient);
				}
			}
		}

		if (bShouldUpdate)
		{
			UFGRecipe* RecipeCDO = mResearchRecipeCDOs.FindRef(ResearchRecipe.Recipe);
			if (!RecipeCDO)
			{
				RecipeCDO = GetMutableDefault<UFGRecipe>(ResearchRecipe.Recipe);
				mResearchRecipeCDOs.Add(ResearchRecipe.Recipe, RecipeCDO);
			}

			RecipeCDO->mIngredients = NewRecipeIngredients;
		}
	}
}

void APMResearchSubsystem::ResetResearchRecipes()
{
	for (const FPMResearchRecipe& ResearchRecipe : mResearchRecipes)
	{
		UFGRecipe* RecipeCDO = mResearchRecipeCDOs.FindRef(ResearchRecipe.Recipe);
		if (RecipeCDO)
		{
			RecipeCDO->mIngredients.Empty();
		}
	}
}
