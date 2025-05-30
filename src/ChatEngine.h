#pragma once
#include "IEngine.h"
#include "LLMEngine.h"
#include <vector>

enum class EmotionType { Happy, Neutral, Sad, Sleepy, Angry, Doubt};

struct ChatResult {
  String       text;
  EmotionType  emotion;
};


class ChatEngine : public IEngine {
public:
  ChatEngine(const String& apiKey);
  std::vector<String> generateReply(const String& input) override;

  void switchTopic(const String& topic);
  String currentTopic() const;

  LLMEngine* getLLMEngine() {
    return &llm;
  }

private:
  LLMEngine llm;
};
