#include "LLMEngine.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "SDUtils.h"

const size_t maxMessages = 10;

static EmotionType labelToEnum(const String& lbl) {
  if      (lbl == "happy")   return EmotionType::Happy;
  else if (lbl == "sad")     return EmotionType::Sad;
  else if (lbl == "mad")     return EmotionType::Angry;
  else if (lbl == "sleepy")  return EmotionType::Sleeply;
  else if (lbl == "doubt")   return EmotionType::Doubt;
  else if (lbl == "neutral") return EmotionType::Neutral;
  return EmotionType::Undefined;
}

LLMEngine::LLMEngine(const String& apiKey, const String& systemPrompt)
  : _apiKey(apiKey) {
  // 改行ルールを無条件で追加
  _systemPrompt = systemPrompt + 
    "返答は自然な単位で、30〜40文字ごとに改行してください。";

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
  DynamicJsonDocument doc(4096);
  doc["model"] = "gpt-4o-mini";
  JsonArray messages = doc.createNestedArray("messages");

  for (const auto& entry : _history) {
    JsonObject msg = messages.createNestedObject();
    msg["role"] = entry.first;
    msg["content"] = entry.second;
  }

  String payload;
  serializeJson(doc, payload);
  return payload;
}

bool LLMEngine::saveHistoryToFile(const String& filename) {
  DynamicJsonDocument doc(4096);
  JsonArray messages = doc.to<JsonArray>();
  for (const auto& entry : _history) {
    JsonObject obj = messages.createNestedObject();
    obj["role"] = entry.first;
    obj["content"] = entry.second;
  }

  return writeJsonToSD(filename.c_str(), doc);
}

bool LLMEngine::loadHistoryFromFile(const String& filename) {
  DynamicJsonDocument doc(4096);
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


bool LLMEngine::sendAndReceive(String& response) {
  HTTPClient https;
  WiFiClientSecure client;
  client.setInsecure(); // または適切なルート証明書を使う

  https.begin(client, "https://api.openai.com/v1/chat/completions");
  https.addHeader("Content-Type", "application/json");
  https.addHeader("Authorization", "Bearer " + _apiKey);

  int httpCode = https.POST(buildPayload());
  if (httpCode != 200) {
    response = "Error: HTTP " + String(httpCode);
    https.end();
    return false;
  }

  String responseBody = https.getString();
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, responseBody);
  if (error) {
    response = "Error: JSON parse failed";
    https.end();
    return false;
  }

  response = doc["choices"][0]["message"]["content"].as<String>();
  addAssistantMessage(response);
  https.end();
  return true;
}

bool LLMEngine::sendAndReceive(std::vector<String>& responses) {
  String fullResponse;
  bool ok = sendAndReceive(fullResponse);  // 既存の関数を使う

  if (!ok) {
    responses = { "応答に失敗しました。" };
    return false;
  }

  responses = splitByNewline(fullResponse);
  return true;
}

std::vector<String> LLMEngine::splitByNewline(const String& text) {
  std::vector<String> result;
  int start = 0;

  while (true) {
    int newlinePos = text.indexOf('\n', start);
    if (newlinePos == -1) break;

    String part = text.substring(start, newlinePos);
    part.trim();
    if (!part.isEmpty()) {
      result.push_back(part);
      Serial.print("[分割] "); Serial.println(part);
    }
    start = newlinePos + 1;
  }

  // 最後の文（改行なし）
  if (start < text.length()) {
    String part = text.substring(start);
    part.trim();
    if (!part.isEmpty()) {
      result.push_back(part);
      Serial.print("[分割] "); Serial.println(part);
    }
  }

  return result;
}

void LLMEngine::generate(const String& prompt, Callback callback) {
  // ユーザーの発話として追加
  addUserMessage(prompt);

  // 非同期風にするが、現状は同期的に送受信して即 callback を呼ぶ
  String response;
  bool success = sendAndReceive(response);
  if (success) {
    callback(response);
  } else {
    callback("エラーが発生しました：" + response);
  }
}