// StackchanHandlers.h
#pragma once

#include "LLMDecisionEngine.h"
#include <M5Unified.h>
#include <Avatar.h>

// Example: Avatar object reference (you may need to manage this elsewhere)
extern m5avatar::Avatar avatar;

void registerStackchanFunctions(LLMDecisionEngine& engine);
