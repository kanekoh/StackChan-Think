#pragma once
#include <Arduino.h>
#include <vector>

class IEngine {
public:
  virtual std::vector<String> generateReply(const String& userInput) = 0;
  virtual ~IEngine() {}
};
