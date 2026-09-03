#include "World/PMCleanerWorldModule.h"
#include "Buildables/FGBuildable.h"
#include "Equipment/FGBuildGun.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Resources/FGBuildDescriptor.h"
#include "Resources/FGResourceNodeBase.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Unlocks/FGUnlock.h"
#include "Unlocks/FGUnlockRecipe.h"
#include "FGCustomizationRecipe.h"
#include "FGRecipe.h"
#include "FGRecipeManager.h"
#include "FGResearchManager.h"
#include "FGResearchTree.h"
#include "FGSchematic.h"
#include "FGSchematicManager.h"
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
		RemoveSchematics();
		RemoveRecipes();
		RemoveItems();
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

				mCachedCDO.Add(ResearchTreeCDO);
			}
		}
	}
}

void UPMCleanerWorldModule::RemoveSchematics()
{
	AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld());
	UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

	AssetDataSubsystem->EnsureRegistryScanned();
	AssetDataSubsystem->EnsureCategoryResolved(0);

	for (TSubclassOf<UFGSchematic> Schematic : AssetDataSubsystem->GetAllSchematics())
	{
		if (Schematic && SchematicManager->mAllSchematics.Contains(Schematic))
		{
			FString SchematicClassName = UKismetSystemLibrary::GetPathName(Schematic);
			bool bIsAllowlisted = false;

			for (const FString& AllowlistEntry : mSchematicClassAllowlist)
			{
				if (SchematicClassName.Contains(AllowlistEntry, ESearchCase::CaseSensitive)) {
					bIsAllowlisted = true;
					break;
				}
			}

			if (!bIsAllowlisted)
			{
				PM_LOG_ARGS(Verbose, TEXT("Removing schematic: %s"), *SchematicClassName);
				SchematicManager->mAllSchematics.Remove(Schematic);

				if (SchematicManager->mPurchasedSchematics.Contains(Schematic))
				{
					SchematicManager->mPurchasedSchematics.Remove(Schematic);
				}

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

						SchematicCDO->mUnlocks.Remove(Unlock);
						mCachedCDO.Add(Unlock);
					}
				}

				mCachedCDO.Add(SchematicCDO);
			}
		}
	}
}

void UPMCleanerWorldModule::RemoveRecipes()
{
	AFGRecipeManager* RecipeManager = AFGRecipeManager::Get(GetWorld());
	UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

	AssetDataSubsystem->EnsureRegistryScanned();
	AssetDataSubsystem->EnsureCategoryResolved(1);

	for (TSubclassOf<UFGRecipe> Recipe : AssetDataSubsystem->GetAllRecipes())
	{
		RemoveRecipe(Recipe);
	}

	TArray<TSubclassOf<UFGRecipe>> RecipesToRemove;
	RecipesToRemove.Append(RecipeManager->mAllRecipes);
	for (TSubclassOf<UFGRecipe> Recipe : RecipesToRemove)
	{
		RemoveRecipe(Recipe);
	}

	RecipeManager->RebuildDerivedAvailableRecipesData();

	for (TSubclassOf<UFGRecipe> Recipe : RecipeManager->mAllRecipes)
	{
		PM_LOG_ARGS(Verbose, TEXT("Remaining all recipes: %s"), *UKismetSystemLibrary::GetPathName(Recipe));
	}

	for (TSubclassOf<UFGRecipe> Recipe : RecipeManager->mAvailableRecipes)
	{
		PM_LOG_ARGS(Verbose, TEXT("Remaining available recipes: %s"), *UKismetSystemLibrary::GetPathName(Recipe));
	}

	for (TSubclassOf<UFGCustomizationRecipe> CustomizationRecipe : RecipeManager->mAvailableCustomizationRecipes)
	{
		PM_LOG_ARGS(Verbose, TEXT("Remaining available customization recipes: %s"), *UKismetSystemLibrary::GetPathName(CustomizationRecipe));
	}

	for (TSubclassOf<AActor> Building : RecipeManager->mAvailableBuildings)
	{
		PM_LOG_ARGS(Verbose, TEXT("Remaining available buildings: %s"), *UKismetSystemLibrary::GetPathName(Building));
	}
}

void UPMCleanerWorldModule::RemoveRecipe(TSubclassOf<UFGRecipe> Recipe)
{
	AFGRecipeManager* RecipeManager = AFGRecipeManager::Get(GetWorld());

	if (Recipe && RecipeManager->mAllRecipes.Contains(Recipe))
	{
		FString RecipeClassName = UKismetSystemLibrary::GetPathName(Recipe);
		bool bIsAllowlisted = false;

		for (const FString& AllowlistEntry : mRecipeClassAllowlist)
		{
			if (RecipeClassName.Contains(AllowlistEntry, ESearchCase::CaseSensitive)) {
				bIsAllowlisted = true;
				break;
			}
		}

		if (!bIsAllowlisted)
		{
			PM_LOG_ARGS(Verbose, TEXT("Removing recipe: %s"), *RecipeClassName);
			RecipeManager->mAllRecipes.Remove(Recipe);

			if (RecipeManager->mAvailableRecipes.Contains(Recipe))
			{
				RecipeManager->mAvailableRecipes.Remove(Recipe);
			}

			if (UFGCustomizationRecipe* CustomizationRecipe = Cast<UFGCustomizationRecipe>(Recipe->GetDefaultObject()))
			{
				if (RecipeManager->mAvailableCustomizationRecipes.Contains(CustomizationRecipe->StaticClass()))
				{
					RecipeManager->mAvailableCustomizationRecipes.Remove(CustomizationRecipe->StaticClass());
				}

				if (RecipeManager->mAvailableCustomizationRecipesLookup.Contains(CustomizationRecipe->StaticClass()))
				{
					RecipeManager->mAvailableCustomizationRecipesLookup.Remove(CustomizationRecipe->StaticClass());
				}
			}

			TArray<TSubclassOf<UObject>> ProducedIn = UFGRecipe::GetProducedIn(Recipe);
			for (TSubclassOf<UObject> Producer : ProducedIn)
			{
				if (AFGBuildGun* BuildGun = Cast<AFGBuildGun>(Producer->GetDefaultObject()))
				{
					TArray<FItemAmount> Products = UFGRecipe::GetProducts(Recipe);
					for (const FItemAmount& Product : Products)
					{
						if (UFGBuildDescriptor* BuildDescriptor = Cast<UFGBuildDescriptor>(Product.ItemClass->GetDefaultObject()))
						{
							if (TSubclassOf<AActor> BuildClass = UFGBuildDescriptor::GetBuildClass(BuildDescriptor->StaticClass()))
							{
								if (AFGBuildable* Buildable = Cast<AFGBuildable>(BuildClass->GetDefaultObject()))
								{
									if (RecipeManager->mAvailableBuildings.Contains(Buildable->StaticClass()))
									{
										RecipeManager->mAvailableBuildings.Remove(Buildable->StaticClass());
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void UPMCleanerWorldModule::RemoveItems()
{
	AFGRecipeManager* RecipeManager = AFGRecipeManager::Get(GetWorld());
	UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

	AssetDataSubsystem->EnsureRegistryScanned();
	AssetDataSubsystem->EnsureCategoryResolved(2);

	for (TSubclassOf<UFGItemDescriptor> Item : AssetDataSubsystem->GetAllItems())
	{
		if (Item && RecipeManager->mAllItemDescriptors.Contains(Item))
		{
			FString ItemClassName = UKismetSystemLibrary::GetPathName(Item);
			bool bIsAllowlisted = false;
			for (const FString& AllowlistEntry : mItemClassAllowlist)
			{
				if (ItemClassName.Contains(AllowlistEntry, ESearchCase::CaseSensitive)) {
					bIsAllowlisted = true;
					break;
				}
			}
			if (!bIsAllowlisted)
			{
				PM_LOG_ARGS(Verbose, TEXT("Removing item: %s"), *ItemClassName);
				RecipeManager->mAllItemDescriptors.Remove(Item);

				if (RecipeManager->mAvailableItemDescriptors.Contains(Item))
				{
					RecipeManager->mAvailableItemDescriptors.Remove(Item);
				}
			}
		}
	}

	RecipeManager->RebuildAvailableItemDescriptorLookup();

	for (TSubclassOf<UFGItemDescriptor> Item : RecipeManager->mAllItemDescriptors)
	{
		PM_LOG_ARGS(Verbose, TEXT("Remaining all item descriptors: %s"), *UKismetSystemLibrary::GetPathName(Item));
	}

	for (TSubclassOf<UFGItemDescriptor> Item : RecipeManager->mAvailableItemDescriptors)
	{
		PM_LOG_ARGS(Verbose, TEXT("Remaining available item descriptors: %s"), *UKismetSystemLibrary::GetPathName(Item));
	}
}
