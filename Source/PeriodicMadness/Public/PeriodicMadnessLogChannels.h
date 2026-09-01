#pragma once

#include "Logging/LogMacros.h"

PERIODICMADNESS_API DECLARE_LOG_CATEGORY_EXTERN(LogPeriodicMadness, Verbose, All);

/**
 * Creates appropriate messages for the LogPeriodicMadness category.
 *
 * @param Verbosity Verbosity level of this message. See ELogVerbosity.
 * @param Message Message string literal.
 */
#define PM_LOG(Verbosity, Message) \
	UE_LOG(LogPeriodicMadness, Verbosity, TEXT("[%s]: %s"), *FString::Printf(TEXT("%s::%s"), *GetClass()->GetName(), ANSI_TO_TCHAR(__FUNCTION__)), Message);

 /**
  * Creates appropriate messages for the LogPeriodicMadness category, with format arguments.
  *
  * @param Verbosity Verbosity level of this message. See ELogVerbosity.
  * @param Format Format string literal in the style of printf.
  * @param Args Comma-separated arguments used to format the message.
  */
#define PM_LOG_ARGS(Verbosity, Format, ...) \
	UE_LOG(LogPeriodicMadness, Verbosity, TEXT("[%s]: %s"), *FString::Printf(TEXT("%s::%s"), *GetClass()->GetName(), ANSI_TO_TCHAR(__FUNCTION__)), *FString::Printf(Format, ##__VA_ARGS__));
