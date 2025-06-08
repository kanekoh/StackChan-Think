#include "EngineManager.h"
#include "LLMEngine.h"
#include <vector>

EngineManager::EngineManager(const String& apiKey)
  : classifier(apiKey), state(InteractionState::Idle) {}

void EngineManager::registerEngine(const String& intentName, IEngine* engine) {
  engineMap[intentName] = engine;
}

LLMResponse EngineManager::handle(const String& userInput) {
  setState(InteractionState::Listening);
  std::vector<String> availableIntents;
  for (const auto& pair : engineMap) {
    availableIntents.push_back(pair.first);
  }

  String intent = classifier.classify(userInput, availableIntents);
  Serial.print("[EngineManager] Intent classified as: ");
  LLMResponse responses;

  if (engineMap.count(intent)) {
    responses = engineMap[intent]->generateReply(userInput);
  } else {
    responses = { "ごめんね、よくわからなかったよ。" };
  }

  setState(InteractionState::Speaking); // 発話をキューに入れる場合に合わせて調整可
  return responses;
}
