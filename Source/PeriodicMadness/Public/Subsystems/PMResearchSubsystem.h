#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "PMResearchSubsystem.generated.h"

class UFGItemDescriptor;
class UFGRecipe;
class UFGSchematic;

USTRUCT(BlueprintType)
struct FPMResearchRecipe
{
	GENERATED_BODY()

public:
	/** The research recipe class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Periodic Madness|Research")
	TSubclassOf<UFGRecipe> Recipe;

	/** The allowed science packs for this research recipe. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Periodic Madness|Research")
	TArray<TSubclassOf<UFGItemDescriptor>> AllowedSciencePacks;
};

/**
 * Controls the remaining research time depending on producing laboratories.
 */
UCLASS(Abstract, Blueprintable)
class PERIODICMADNESS_API APMResearchSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	APMResearchSubsystem();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Begin AActor interface
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End AActor interface

	// Begin IFGSaveInterface
	virtual void PreSaveGame_Implementation(int32 SaveVersion, int32 GameVersion) override;
	virtual void PostLoadGame_Implementation(int32 SaveVersion, int32 GameVersion) override;
	virtual bool NeedTransform_Implementation() override;
	virtual bool ShouldSave_Implementation() const override;
	// End IFGSaveInterface

	UFUNCTION()
	void HandleOnResearchStarted(TSubclassOf<UFGSchematic> Schematic);

	UFUNCTION()
	void HandleOnResearchCompleted(TSubclassOf<UFGSchematic> Schematic);

protected:
	/** Calculates the research speed multiplier based on the number of producing laboratories. */
	float CalculateResearchSpeedMultiplier() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Periodic Madness|Research", DisplayName = "OnResearchTimeAdjusted")
	void K2_ResearchTimeAdjusted(const float AdjustedDuration);

	/** The science packs to consider in the research costs to allow improved research speed. */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Research")
	TArray<TSubclassOf<UFGItemDescriptor>> mSciencePacksToConsider;

	/** Specifies the research recipes and their associated allowed science packs. */
	UPROPERTY(EditDefaultsOnly, Category = "Periodic Madness|Research")
	TArray<FPMResearchRecipe> mResearchRecipes;

private:
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_ResearchTimeAdjusted(TSubclassOf<UFGSchematic> Schematic, float AdjustedDuration);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_ResetResearchDuration(TSubclassOf<UFGSchematic> Schematic, float OriginalDuration);

	/** Updates the research recipes based on the allowed science packs on them and the current research schematic costs. */
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_UpdateResearchRecipes();

	/** Resets the research recipes to their original state. */
	void ResetResearchRecipes();

	/** The current research schematic being processed. */
	UPROPERTY(SaveGame, Replicated)
	TSubclassOf<UFGSchematic> mCurrentResearchSchematic;

	UPROPERTY(Transient)
	TObjectPtr<UFGSchematic> mCurrentResearchSchematicCDO;

	/** The original research duration before any modifications. */
	UPROPERTY(SaveGame, Replicated)
	float mOriginalResearchDuration;

	/** The original research complete timestamp before any modifications. */
	UPROPERTY(Transient)
	float mOriginalResearchCompleteTimestamp;

	/** The original remaining research time before any modifications. Will be calculated on pre save, and extracted on post load. */
	UPROPERTY(SaveGame)
	float mOriginalRemainingResearchTime;

	UPROPERTY(Transient)
	TMap<TSubclassOf<UFGRecipe>, TObjectPtr<UFGRecipe>> mResearchRecipeCDOs;
};
