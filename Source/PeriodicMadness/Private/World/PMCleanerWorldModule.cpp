#include "World/PMCleanerWorldModule.h"
#include "Buildables/FGBuildable.h"
#include "Equipment/FGBuildGun.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Resources/FGBuildDescriptor.h"
#include "Resources/FGResourceDeposit.h"
#include "Resources/FGResourceNode.h"
#include "Resources/FGResourceNodeBase.h"
#include "Settings/PMCleanerSettings.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/PMCleanerSubsystem.h"
#include "FGCustomizationRecipe.h"
#include "FGRecipe.h"
#include "FGRecipeManager.h"
#include "FGResearchManager.h"
#include "FGResearchTree.h"
#include "FGSchematic.h"
#include "FGSchematicManager.h"
#include "FGUnlockSubsystem.h"
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
		RemoveUnlockedScannableResources();
	}

    // Blueprint event logic should be dispatched after our code.
    Super::DispatchLifecycleEvent(Phase);
}

void UPMCleanerWorldModule::RemoveResourceNodes()
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();

	TArray<AActor*> ResourceNodeActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFGResourceNode::StaticClass(), ResourceNodeActors);

	for (AActor* Actor : ResourceNodeActors)
	{
		AFGResourceNode* ResourceNode = Cast<AFGResourceNode>(Actor);
		if (ResourceNode && ResourceNode->GetResourceClass())
		{
			if (CleanerSettings->ShouldRemoveResourceClass(ResourceNode->GetResourceClass()))
			{
				// Log removed nodes in CSV format.
				//if (!ResourceNode->IsA(AFGResourceDeposit::StaticClass()))
				//{
				//	PM_LOG_ARGS(Verbose, TEXT("%s;%s;%3.3f;%3.3f;%3.3f;%f;%f;%f"), *UKismetSystemLibrary::GetPathName(ResourceNode->GetResourceClass()), *UEnum::GetValueAsString(ResourceNode->GetResourcePurity()), ResourceNode->GetActorLocation().X, ResourceNode->GetActorLocation().Y, ResourceNode->GetActorLocation().Z, ResourceNode->GetActorRotation().Roll, ResourceNode->GetActorRotation().Pitch, ResourceNode->GetActorRotation().Yaw);
				//}

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
		if (ResourceNode && ResourceNode->GetResourceClass())
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
	if (APMCleanerSubsystem* CleanerSubsystem = APMCleanerSubsystem::Get(GetWorld()))
	{
		if (AFGResearchManager* ResearchManager = AFGResearchManager::Get(GetWorld()))
		{
			UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

			AssetDataSubsystem->EnsureRegistryScanned();
			AssetDataSubsystem->EnsureCategoryResolved(10);

			for (const TSubclassOf<UFGResearchTree>& ResearchTree : AssetDataSubsystem->GetAllResearchTrees())
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

						CleanerSubsystem->ResearchTreeRemovalModifyCDO(ResearchTree);
					}
				}
			}
		}
	}
}

void UPMCleanerWorldModule::RemoveSchematics()
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();
	if (APMCleanerSubsystem* CleanerSubsystem = APMCleanerSubsystem::Get(GetWorld()))
	{
		if (AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld()))
		{
			UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

			AssetDataSubsystem->EnsureRegistryScanned();
			AssetDataSubsystem->EnsureCategoryResolved(0);

			for (const TSubclassOf<UFGSchematic>& Schematic : AssetDataSubsystem->GetAllSchematics())
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

						CleanerSubsystem->SchematicRemovalModifyCDO(Schematic);
					}

					CleanerSubsystem->SchematicCleanup(Schematic);
				}
			}
		}
	}
}

void UPMCleanerWorldModule::RemoveRecipes()
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();
	if (AFGRecipeManager* RecipeManager = AFGRecipeManager::Get(GetWorld()))
	{
		UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

		AssetDataSubsystem->EnsureRegistryScanned();
		AssetDataSubsystem->EnsureCategoryResolved(1);

		for (const TSubclassOf<UFGRecipe>& Recipe : AssetDataSubsystem->GetAllRecipes())
		{
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

					if (const UFGCustomizationRecipe* CustomizationRecipe = Cast<UFGCustomizationRecipe>(Recipe->GetDefaultObject()))
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
					for (const TSubclassOf<UObject>& Producer : ProducedIn)
					{
						if (const AFGBuildGun* BuildGun = Cast<AFGBuildGun>(Producer->GetDefaultObject()))
						{
							TArray<FItemAmount> Products = UFGRecipe::GetProducts(Recipe);
							for (const FItemAmount& Product : Products)
							{
								if (const UFGBuildDescriptor* BuildDescriptor = Cast<UFGBuildDescriptor>(Product.ItemClass->GetDefaultObject()))
								{
									if (const TSubclassOf<AActor> BuildClass = UFGBuildDescriptor::GetBuildClass(BuildDescriptor->StaticClass()))
									{
										if (const AFGBuildable* Buildable = Cast<AFGBuildable>(BuildClass->GetDefaultObject()))
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

		RecipeManager->RebuildDerivedAvailableRecipesData();

		for (const TSubclassOf<UFGRecipe>& Recipe : RecipeManager->mAllRecipes)
		{
			PM_LOG_ARGS(Verbose, TEXT("Remaining all recipes: %s"), *UKismetSystemLibrary::GetPathName(Recipe));
		}

		for (const TSubclassOf<UFGRecipe>& Recipe : RecipeManager->mAvailableRecipes)
		{
			PM_LOG_ARGS(Verbose, TEXT("Remaining available recipes: %s"), *UKismetSystemLibrary::GetPathName(Recipe));
		}

		for (const TSubclassOf<UFGCustomizationRecipe>& CustomizationRecipe : RecipeManager->mAvailableCustomizationRecipes)
		{
			PM_LOG_ARGS(Verbose, TEXT("Remaining available customization recipes: %s"), *UKismetSystemLibrary::GetPathName(CustomizationRecipe));
		}

		for (const TSubclassOf<AActor>& Building : RecipeManager->mAvailableBuildings)
		{
			PM_LOG_ARGS(Verbose, TEXT("Remaining available buildings: %s"), *UKismetSystemLibrary::GetPathName(Building));
		}
	}
}

void UPMCleanerWorldModule::RemoveItems()
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();
	if (AFGRecipeManager* RecipeManager = AFGRecipeManager::Get(GetWorld()))
	{
		UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

		AssetDataSubsystem->EnsureRegistryScanned();
		AssetDataSubsystem->EnsureCategoryResolved(2);

		for (const TSubclassOf<UFGItemDescriptor>& Item : AssetDataSubsystem->GetAllItems())
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

		for (const TSubclassOf<UFGItemDescriptor>& Item : RecipeManager->mAllItemDescriptors)
		{
			PM_LOG_ARGS(Verbose, TEXT("Remaining all item descriptors: %s"), *UKismetSystemLibrary::GetPathName(Item));
		}

		for (const TSubclassOf<UFGItemDescriptor>& Item : RecipeManager->mAvailableItemDescriptors)
		{
			PM_LOG_ARGS(Verbose, TEXT("Remaining available item descriptors: %s"), *UKismetSystemLibrary::GetPathName(Item));
		}
	}
}

void UPMCleanerWorldModule::RemoveUnlockedScannableResources()
{
	const UPMCleanerSettings* CleanerSettings = UPMCleanerSettings::Get();
	if (AFGUnlockSubsystem* UnlockSubsystem = AFGUnlockSubsystem::Get(GetWorld()))
	{
		UnlockSubsystem->mScannableResources.Empty();

		TArray<FScannableResourcePair> UnlockedScannableResourcePairs;
		UnlockedScannableResourcePairs.Append(UnlockSubsystem->mScannableResourcesPairs);

		for (const FScannableResourcePair& ResourcePair : UnlockedScannableResourcePairs)
		{
			if (CleanerSettings->ShouldRemoveScannableResourceClass(ResourcePair.ResourceDescriptor))
			{
				PM_LOG_ARGS(Verbose, TEXT("Removing unlocked scannable resource: %s"), *UKismetSystemLibrary::GetPathName(ResourcePair.ResourceDescriptor));
				UnlockSubsystem->mScannableResourcesPairs.Remove(ResourcePair);
			}
		}
	}
}
