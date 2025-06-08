#pragma once
#include "IEngine.h"
#include "LLMEngine.h"
#include <vector>


class ChatEngine : public IEngine {
public:
  ChatEngine(const String& apiKey);
  LLMResponse generateReply(const String& input) override;

  void switchTopic(const String& topic);
  String currentTopic() const;

  LLMEngine* getLLMEngine() {
    return &llm;
  }

private:
  LLMEngine llm;
};
