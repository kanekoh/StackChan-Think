#pragma once
#include <vector>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <vector>
#include "Message.h"

// ① New enum
enum class EmotionType { Happy, Neutral, Sad, Angry, Sleepy, Doubt, Undefined };

// ② Unified response object
struct LLMResponse {
    String message;    // reply for TTS
    EmotionType         emotion;     // casual engines use it; others can ignore
};

class LLMEngine {
public:
  LLMEngine(const String& apiKey, const String& systemPrompt = "あなたはスーパーかわいいAIアシスタントロボット、スタックチャンです。かわいいく話、元気づけてください。返信はPlanなJSON形式で、messageと emotion で返却してください。emotionは happy, sad, angry, sleepy, doubt, neutral のいずれかを返してください。");
  void addUserMessage(const String& content);
  void addAssistantMessage(const String& content);
  String buildPayload() const;
  bool sendAndReceive(LLMResponse& response);
  void resetConversation();
  bool saveHistoryToFile(const String& filename);
  bool loadHistoryFromFile(const String& filename);
  bool switchTopic(const String& newTopic);
  String currentTopic() const;
  void setSystemPrompt(const String& prompt);
  using Callback = std::function<void(LLMResponse)>;
  void generate(const String& prompt, Callback callback);
  std::vector<std::pair<String, String>> getHistory() const;
  /**
   * @param withEmotion  true  – ask the model to return emotion label
   *                     false – legacy, just reply text
   */
  LLMResponse generate(const std::vector<Message>& messages,
                        bool withEmotion = false);

  private:
  String _apiKey;
  String _systemPrompt;
  std::vector<std::pair<String, String>> _history; // role, content
  String _currentTopic;
  std::vector<String> splitByNewline(const String& text);

  void trimHistory(); // 履歴が長くなりすぎないように調整
};
