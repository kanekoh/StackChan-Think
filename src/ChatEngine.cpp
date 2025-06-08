#include "ChatEngine.h"

ChatEngine::ChatEngine(const String& apiKey)
  : llm(apiKey) {
  llm.switchTopic("chat");  // デフォルトトピックをセット
}

LLMResponse ChatEngine::generateReply(const String& input) {
  llm.addUserMessage(input);
  LLMResponse result;

  if (llm.sendAndReceive(result)) {
    llm.saveHistoryToFile("/spiffs/history_" + llm.currentTopic() + ".json");
  }

  return result;
}

void ChatEngine::switchTopic(const String& topic) {
  llm.switchTopic(topic);
}

String ChatEngine::currentTopic() const {
  return llm.currentTopic();
}
