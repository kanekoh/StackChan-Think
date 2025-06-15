#include "LLMEngine.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "SDUtils.h"

const size_t maxMessages = 10;

static EmotionType labelToEnum(const String& lbl) {
  if      (lbl == "happy")   return EmotionType::Happy;
  else if (lbl == "sad")     return EmotionType::Sad;
  else if (lbl == "angry")     return EmotionType::Angry;
  else if (lbl == "sleepy")  return EmotionType::Sleepy;
  else if (lbl == "doubt")   return EmotionType::Doubt;
  else if (lbl == "neutral") return EmotionType::Neutral;
  return EmotionType::Undefined;
}

LLMEngine::LLMEngine(const String& apiKey, const String& systemPrompt)
  : _apiKey(apiKey) {
  // 改行ルールを無条件で追加
  _systemPrompt = systemPrompt;

  if (!_systemPrompt.isEmpty()) {
    _history.emplace_back("system", _systemPrompt);
  }
}

void LLMEngine::addUserMessage(const String& content) {
  _history.emplace_back("user", content);
  trimHistory();
}

void LLMEngine::addAssistantMessage(const String& content) {
  _history.emplace_back("assistant", content);
  trimHistory();
}

void LLMEngine::resetConversation() {
  _history.clear();
  if (!_systemPrompt.isEmpty()) {
    _history.emplace_back("system", _systemPrompt);
  }
}

void LLMEngine::trimHistory() {
  while (_history.size() > maxMessages) {
    _history.erase(_history.begin() + 1); // systemを除いて削除
  }
}

String LLMEngine::buildPayload() const {
  JsonDocument doc;
  doc["model"] = "gpt-4o-mini";
  JsonArray messages = doc["messages"].to<JsonArray>();

  for (const auto& entry : _history) {
    JsonObject msg = messages.add<JsonObject>();
    msg["role"] = entry.first;
    msg["content"] = entry.second;
  }

  String payload;
  serializeJson(doc, payload);
  Serial.println("Payload: " + payload); // デバッグ用
  return payload;
}

bool LLMEngine::saveHistoryToFile(const String& filename) {
  JsonDocument doc;
  JsonArray messages = doc.to<JsonArray>();
  for (const auto& entry : _history) {
    JsonObject obj = messages.add<JsonObject>();
    obj["role"] = entry.first;
    obj["content"] = entry.second;
  }

  return writeJsonToSD(filename.c_str(), doc);
}

bool LLMEngine::loadHistoryFromFile(const String& filename) {
  JsonDocument doc;
  if (!readJsonFromSD(filename.c_str(), doc)) {
    return false;
  }

  _history.clear();
  for (JsonObject obj : doc.as<JsonArray>()) {
    _history.emplace_back(obj["role"].as<String>(), obj["content"].as<String>());
  }

  return true;
}

std::vector<std::pair<String, String>> LLMEngine::getHistory() const {
  return _history;
}

static String makeTopicFilename(const String& topic) {
  return "/spiffs/history_" + topic + ".json";
}

bool LLMEngine::switchTopic(const String& newTopic) {
  // 現在のトピックを保存
  if (!_currentTopic.isEmpty()) {
    saveHistoryToFile(makeTopicFilename(_currentTopic));
  }

  // 新しいトピックに切り替え
  _currentTopic = newTopic;

  // 読み込み。ファイルがなければ初期化
  if (!loadHistoryFromFile(makeTopicFilename(_currentTopic))) {
    Serial.println("New topic started: " + _currentTopic);
    resetConversation();
  }

  return true;
}

String LLMEngine::currentTopic() const {
  return _currentTopic;
}

void LLMEngine::setSystemPrompt(const String& prompt) {
  _systemPrompt = prompt;
  resetConversation();  // 再設定時には履歴も初期化（または別設計でも可）
}


bool LLMEngine::sendAndReceive(LLMResponse& response) {
  HTTPClient https;
  WiFiClientSecure client;
  client.setInsecure(); // または適切なルート証明書を使う

  https.begin(client, "https://api.openai.com/v1/chat/completions");
  https.addHeader("Content-Type", "application/json");
  https.addHeader("Authorization", "Bearer " + _apiKey);

  int httpCode = https.POST(buildPayload());
  if (httpCode != 200) {
    response.message = "Error: HTTP " + String(httpCode);
    response.emotion = EmotionType::Sad;
    https.end();
    return false;
  }

  String responseBody = https.getString();
  Serial.println("Response: " + responseBody); // デバッグ用
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, responseBody);
  if (error) {
    response.message = "考えてたけどよくわかんなくなっちゃった。";
    response.emotion = EmotionType::Sad;
    https.end();
    return false;
  }

  String content = doc["choices"][0]["message"]["content"].as<String>();
  Serial.println("Content: " + content); // デバッグ用

  // JSON コードブロックを除去
  if (content.startsWith("```json") || content.startsWith("```")) {
    int start = content.indexOf('\n');
    int end   = content.lastIndexOf("```");
    if (start != -1 && end != -1 && end > start) {
      content = content.substring(start + 1, end);
      content.trim();  // 念のため空白削除
    }
  }
  JsonDocument inner;
  if (deserializeJson(inner, content)) {
    response.message = content;
    response.emotion = EmotionType::Neutral;
  } else {
    response.message = inner["message"].as<String>();
    response.emotion = labelToEnum(inner["emotion"].as<String>());
  }

  addAssistantMessage(response.message);
  https.end();
  return true;
}

void LLMEngine::generate(const String& prompt, Callback callback) {
  // ユーザーの発話として追加
  addUserMessage(prompt);

  // 非同期風にするが、現状は同期的に送受信して即 callback を呼ぶ
  LLMResponse response;
  bool success = sendAndReceive(response);
  callback(response);
}