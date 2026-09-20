#include "PeriodicMadness.h"
#include "Hooks/PMProductionBoostHooks.h"

#define LOCTEXT_NAMESPACE "FPeriodicMadnessModule"

void FPeriodicMadnessModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddLambda([]() {
		UPMProductionBoostHooks::ConfigureHooks();
	});
}

void FPeriodicMadnessModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPeriodicMadnessModule, PeriodicMadness)
