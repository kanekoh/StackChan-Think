#include "IntentClassifier.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

IntentClassifier::IntentClassifier(const String& apiKey) : _apiKey(apiKey) {}

String IntentClassifier::classify(const String& userInput, const std::vector<String>& intents) {
  String intentList = "";
  for (size_t i = 0; i < intents.size(); ++i) {
    intentList += "'" + intents[i] + "'";
    if (i != intents.size() - 1) intentList += "、";
  }

  String systemPrompt = "次の発言が以下の分類のうちどれに該当するかを判定してください。"
                        "返答は分類名を1語だけ返してください。候補：" + intentList;

  DynamicJsonDocument doc(1024);
  doc["model"] = "gpt-3.5-turbo";
  JsonArray messages = doc.createNestedArray("messages");
  JsonObject sys = messages.createNestedObject();
  sys["role"] = "system";
  sys["content"] = systemPrompt;
  JsonObject usr = messages.createNestedObject();
  usr["role"] = "user";
  usr["content"] = userInput;

  String payload;
  serializeJson(doc, payload);

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  https.begin(client, "https://api.openai.com/v1/chat/completions");
  https.addHeader("Authorization", "Bearer " + _apiKey);
  https.addHeader("Content-Type", "application/json");

  int httpCode = https.POST(payload);
  if (httpCode != 200) {
    https.end();
    return "unknown";
  }

  String response = https.getString();
  https.end();

  DynamicJsonDocument respDoc(1024);
  if (deserializeJson(respDoc, response)) return "unknown";

  String content = respDoc["choices"][0]["message"]["content"];
  content.trim(); content.toLowerCase();
  return content;
}
