#pragma once
#include <Arduino.h>
#include <vector>
#include "LLMEngine.h"

class IEngine {
public:
  virtual LLMResponse generateReply(const String& userInput) = 0;
  virtual ~IEngine() {}
};
