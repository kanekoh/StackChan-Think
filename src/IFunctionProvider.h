// IFunctionProvider.h
#pragma once

class LLMDecisionEngine;

class IFunctionProvider {
public:
  virtual void registerFunctions(LLMDecisionEngine& engine) = 0;
  virtual ~IFunctionProvider() {}
};
