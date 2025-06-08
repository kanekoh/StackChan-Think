#pragma once
#include <map>
#include <vector>
#include "IEngine.h"
#include "IntentClassifier.h"
#include "LLMEngine.h"

enum class InteractionState {
  Idle,
  Listening,
  Speaking,
  Thinking 
};

class EngineManager {
public:
  EngineManager(const String& apiKey);

  void registerEngine(const String& intentName, IEngine* engine);
  LLMResponse handle(const String& userInput);

  // 新しく追加するメソッド
  void setState(InteractionState newState) {
    state = newState;
  }

  InteractionState getState() const {
    return state;
  }

  bool canTalk() const {
    return state == InteractionState::Idle;
  }

private:
  IntentClassifier classifier;
  std::map<String, IEngine*> engineMap;

  InteractionState state = InteractionState::Idle;
};
