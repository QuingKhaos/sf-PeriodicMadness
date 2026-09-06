#include "Settings/PMCleanerSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Resources/FGItemDescriptor.h"
#include "Resources/FGResourceDescriptor.h"
#include "FGRecipe.h"
#include "FGResearchTree.h"
#include "FGSchematic.h"

const UPMCleanerSettings* UPMCleanerSettings::SingletonInstance = nullptr;

const UPMCleanerSettings* UPMCleanerSettings::Get()
{
	SingletonInstance = SingletonInstance ? SingletonInstance : GetDefault<UPMCleanerSettings>();

	return SingletonInstance;
}

bool UPMCleanerSettings::ShouldRemoveStaticMesh(const UStaticMesh* StaticMesh) const
{
	return mStaticMeshesCleanlist.Contains(StaticMesh);
}

bool UPMCleanerSettings::ShouldRemoveResourceClass(const TSubclassOf<UFGResourceDescriptor>& ResourceClass) const
{
	FString ResourceClassName = UKismetSystemLibrary::GetPathName(ResourceClass);
	bool bIsAllowlisted = false;

	for (const FString& AllowlistEntry : mResourceClassAllowlist)
	{
		if (ResourceClassName.Contains(AllowlistEntry, ESearchCase::CaseSensitive))
		{
			bIsAllowlisted = true;
			break;
		}
	}

	return !bIsAllowlisted;
}

bool UPMCleanerSettings::ShouldReplaceResourceClass(const TSubclassOf<UFGResourceDescriptor>& ResourceClass, TSubclassOf<UFGResourceDescriptor>& OutReplacement) const
{
	for (const FPMResourceReplacement& Replacement : mResourceClassReplacements)
	{
		if (ResourceClass == Replacement.Target)
		{
			OutReplacement = Replacement.Replacement;
			return true;
		}
	}

	return false;
}

bool UPMCleanerSettings::ShouldRemoveResearchTreeClass(const TSubclassOf<UFGResearchTree>& ResearchTreeClass) const
{
	FString ResearchTreeClassName = UKismetSystemLibrary::GetPathName(ResearchTreeClass);
	bool bIsAllowlisted = false;

	for (const FString& AllowlistEntry : mResearchTreeClassAllowlist)
	{
		if (ResearchTreeClassName.Contains(AllowlistEntry, ESearchCase::CaseSensitive))
		{
			bIsAllowlisted = true;
			break;
		}
	}

	return !bIsAllowlisted;
}

bool UPMCleanerSettings::ShouldRemoveSchematicClass(const TSubclassOf<UFGSchematic>& SchematicClass) const
{
	FString SchematicClassName = UKismetSystemLibrary::GetPathName(SchematicClass);
	bool bIsAllowlisted = false;

	for (const FString& AllowlistEntry : mSchematicClassAllowlist)
	{
		if (SchematicClassName.Contains(AllowlistEntry, ESearchCase::CaseSensitive))
		{
			bIsAllowlisted = true;
			break;
		}
	}

	return !bIsAllowlisted;
}

bool UPMCleanerSettings::ShouldRemoveRecipeClass(const TSubclassOf<UFGRecipe>& RecipeClass) const
{
	FString RecipeClassName = UKismetSystemLibrary::GetPathName(RecipeClass);
	bool bIsAllowlisted = false;

	for (const FString& AllowlistEntry : mRecipeClassAllowlist)
	{
		if (RecipeClassName.Contains(AllowlistEntry, ESearchCase::CaseSensitive))
		{
			bIsAllowlisted = true;
			break;
		}
	}

	return !bIsAllowlisted;
}

bool UPMCleanerSettings::ShouldRemoveItemClass(const TSubclassOf<UFGItemDescriptor>& ItemClass) const
{
	FString ItemClassName = UKismetSystemLibrary::GetPathName(ItemClass);
	bool bIsAllowlisted = false;

	for (const FString& AllowlistEntry : mItemClassAllowlist)
	{
		if (ItemClassName.Contains(AllowlistEntry, ESearchCase::CaseSensitive))
		{
			bIsAllowlisted = true;
			break;
		}
	}

	return !bIsAllowlisted;
}
