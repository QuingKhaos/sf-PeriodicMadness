#include "World/PMCleanerWorldModule.h"
#include "Buildables/FGBuildable.h"
#include "Equipment/FGBuildGun.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Resources/FGBuildDescriptor.h"
#include "Resources/FGResourceNodeBase.h"
#include "Settings/PMCleanerSettings.h"
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
		ReplaceResources();
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
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();

	TArray<AActor*> ResourceNodeActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFGResourceNodeBase::StaticClass(), ResourceNodeActors);

	for (AActor* Actor : ResourceNodeActors)
	{
		AFGResourceNodeBase* ResourceNode = Cast<AFGResourceNodeBase>(Actor);
		if (ResourceNode)
		{
			if (CleanerSettings->ShouldRemoveResourceClass(ResourceNode->GetResourceClass()))
			{
				PM_LOG_ARGS(Verbose, TEXT("Removing resource: %s, Node: %s"), *UKismetSystemLibrary::GetPathName(ResourceNode->GetResourceClass()), *UKismetSystemLibrary::GetPathName(ResourceNode));

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

void UPMCleanerWorldModule::ReplaceResources()
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();

	TArray<AActor*> ResourceNodeActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFGResourceNodeBase::StaticClass(), ResourceNodeActors);

	for (AActor* Actor : ResourceNodeActors)
	{
		AFGResourceNodeBase* ResourceNode = Cast<AFGResourceNodeBase>(Actor);
		if (ResourceNode)
		{
			TSubclassOf<UFGResourceDescriptor> ReplacementResourceClass;
			if (CleanerSettings->ShouldReplaceResourceClass(ResourceNode->GetResourceClass(), ReplacementResourceClass))
			{
				PM_LOG_ARGS(Verbose, TEXT("Replacing resource: %s with %s, Node: %s"), *UKismetSystemLibrary::GetPathName(ResourceNode->GetResourceClass()), *UKismetSystemLibrary::GetPathName(ReplacementResourceClass), *UKismetSystemLibrary::GetPathName(ResourceNode));
				ResourceNode->SetResourceClass(ReplacementResourceClass);
			}
		}
	}

	K2_ResourceNodesRemoved();
}

void UPMCleanerWorldModule::RemoveResearchTrees()
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();
	AFGResearchManager* ResearchManager = AFGResearchManager::Get(GetWorld());
	UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

	AssetDataSubsystem->EnsureRegistryScanned();
	AssetDataSubsystem->EnsureCategoryResolved(10);

	for (TSubclassOf<UFGResearchTree> ResearchTree : AssetDataSubsystem->GetAllResearchTrees())
	{
		if (ResearchTree && ResearchManager->mAvailableResearchTrees.Contains(ResearchTree))
		{
			if (CleanerSettings->ShouldRemoveResearchTreeClass(ResearchTree))
			{
				PM_LOG_ARGS(Verbose, TEXT("Removing research tree: %s"), *UKismetSystemLibrary::GetPathName(ResearchTree));
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
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();
	AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld());
	UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

	AssetDataSubsystem->EnsureRegistryScanned();
	AssetDataSubsystem->EnsureCategoryResolved(0);

	for (TSubclassOf<UFGSchematic> Schematic : AssetDataSubsystem->GetAllSchematics())
	{
		if (Schematic && SchematicManager->mAllSchematics.Contains(Schematic))
		{
			if (CleanerSettings->ShouldRemoveSchematicClass(Schematic))
			{
				PM_LOG_ARGS(Verbose, TEXT("Removing schematic: %s"), *UKismetSystemLibrary::GetPathName(Schematic));
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
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();
	AFGRecipeManager* RecipeManager = AFGRecipeManager::Get(GetWorld());

	if (Recipe && RecipeManager->mAllRecipes.Contains(Recipe))
	{
		if (CleanerSettings->ShouldRemoveRecipeClass(Recipe))
		{
			PM_LOG_ARGS(Verbose, TEXT("Removing recipe: %s"), *UKismetSystemLibrary::GetPathName(Recipe));
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
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();
	AFGRecipeManager* RecipeManager = AFGRecipeManager::Get(GetWorld());
	UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

	AssetDataSubsystem->EnsureRegistryScanned();
	AssetDataSubsystem->EnsureCategoryResolved(2);

	for (TSubclassOf<UFGItemDescriptor> Item : AssetDataSubsystem->GetAllItems())
	{
		if (Item && RecipeManager->mAllItemDescriptors.Contains(Item))
		{
			if (CleanerSettings->ShouldRemoveItemClass(Item))
			{
				PM_LOG_ARGS(Verbose, TEXT("Removing item: %s"), *UKismetSystemLibrary::GetPathName(Item));
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
