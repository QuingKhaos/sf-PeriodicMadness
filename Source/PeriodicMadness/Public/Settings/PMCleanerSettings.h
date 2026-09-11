#pragma once

#include "CoreMinimal.h"
#include "FGSettings.h"
#include "PMCleanerSettings.generated.h"

class UFGItemDescriptor;
class UFGRecipe;
class UFGResearchTree;
class UFGResourceDescriptor;
class UFGSchematic;

/**
 * Item replacement mapping.
 */
USTRUCT(BlueprintType)
struct FPMItemReplacement
{
	GENERATED_BODY()

public:
	/** The item class to be replaced. */
	UPROPERTY(BlueprintReadOnly, Category = "Periodic Madness|Resource Cleanup")
	TSubclassOf<UFGItemDescriptor> Target;

	/** The item class to use as a replacement. */
	UPROPERTY(BlueprintReadOnly, Category = "Periodic Madness|Resource Cleanup")
	TSubclassOf<UFGItemDescriptor> Replacement;
};

/**
 * Resource replacement mapping.
 */
USTRUCT(BlueprintType)
struct FPMResourceReplacement
{
	GENERATED_BODY()

public:
	/** The resource class to be replaced. */
	UPROPERTY(BlueprintReadOnly, Category = "Periodic Madness|Resource Cleanup")
	TSubclassOf<UFGResourceDescriptor> Target;

	/** The resource class to use as a replacement. */
	UPROPERTY(BlueprintReadOnly, Category = "Periodic Madness|Resource Cleanup")
	TSubclassOf<UFGResourceDescriptor> Replacement;
};

/**
 * Periodic Madness settings for the resource cleanup system.
 */
UCLASS()
class PERIODICMADNESS_API UPMCleanerSettings : public UFGSettings
{
	GENERATED_BODY()

public:
	static const UPMCleanerSettings* Get();

	/** Checks if a static mesh is allowed to be removed. */
	bool ShouldRemoveStaticMesh(const UStaticMesh* StaticMesh) const;
	/** Checks if a resource class is allowed to be removed. */
	bool ShouldRemoveResourceClass(const TSubclassOf<UFGResourceDescriptor>& ResourceClass) const;
	/** Checks if a resource class should be replaced and provides the replacement class. */
	bool ShouldReplaceResourceClass(const TSubclassOf<UFGResourceDescriptor>& ResourceClass, TSubclassOf<UFGResourceDescriptor>& OutReplacement) const;
	/** Checks if a foliage item drop class is allowed to be removed. */
	bool ShouldRemoveFoliageItemClass(const TSubclassOf<UFGItemDescriptor>& ItemClass) const;
	/** Checks if a foliage item class should be replaced and provides the replacement class. */
	bool ShouldReplaceFoliageItemClass(const TSubclassOf<UFGItemDescriptor>& ItemClass, TSubclassOf<UFGItemDescriptor>& OutReplacement) const;
	/** Checks if a research tree class is allowed to be removed. */
	bool ShouldRemoveResearchTreeClass(const TSubclassOf<UFGResearchTree>& ResearchTreeClass) const;
	/** Checks if a schematic class is allowed to be removed. */
	bool ShouldRemoveSchematicClass(const TSubclassOf<UFGSchematic>& SchematicClass) const;
	/** Checks if a recipe class is allowed to be removed. */
	bool ShouldRemoveRecipeClass(const TSubclassOf<UFGRecipe>& RecipeClass) const;
	/** Checks if an item class is allowed to be removed. */
	bool ShouldRemoveItemClass(const TSubclassOf<UFGItemDescriptor>& ItemClass) const;
	/** Checks if a resource class is allowed to be removed from the resource scanner. */
	bool ShouldRemoveScannableResourceClass(const TSubclassOf<UFGResourceDescriptor>& ResourceClass) const;

protected:
	/** List of static meshes substrings to be cleaned up */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Resource Cleanup")
	TArray<FString> mStaticMeshesCleanlist;

	/** List of resource class substrings to block from removal. */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Resource Cleanup")
	TArray<FString> mResourceClassAllowlist;

	/** List of resource class replacements. */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Resource Cleanup")
	TArray<FPMResourceReplacement> mResourceClassReplacements;

	/** List of item class substrings on foliage drops to block from removal. */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Resource Cleanup")
	TArray<FString> mFoliageItemClassAllowlist;

	/** List of item class replacements on foliage drops. */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Resource Cleanup")
	TArray<FPMItemReplacement> mFoliageItemClassReplacements;

	/** List of research tree class substrings to block from removal. */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Resource Cleanup")
	TArray<FString> mResearchTreeClassAllowlist;

	/** List of schematic class substrings to block from removal. */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Resource Cleanup")
	TArray<FString> mSchematicClassAllowlist;

	/** List of recipe class substrings to block from removal. */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Resource Cleanup")
	TArray<FString> mRecipeClassAllowlist;

	/** List of item class substrings to block from removal. */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Resource Cleanup")
	TArray<FString> mItemClassAllowlist;

	/** List of resource class substrings to block from removal from the resource scanner. */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Resource Cleanup")
	TArray<FString> mResourceScannerAllowlist;

private:
	static const UPMCleanerSettings* SingletonInstance;
};
