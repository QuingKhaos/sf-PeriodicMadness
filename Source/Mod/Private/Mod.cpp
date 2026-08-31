#include "###NAME###.h"

#define LOCTEXT_NAMESPACE "F###NAME###Module"

void F###NAME###Module::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void F###NAME###Module::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(F###NAME###Module, ###NAME###)
