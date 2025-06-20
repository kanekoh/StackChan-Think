// LLMDecisionEngine.cpp
#include "LLMDecisionEngine.h"

LLMDecisionEngine::LLMDecisionEngine(const String& apiKey)
  : _apiKey(apiKey), _functionCallPending(false) {
  _chatHistory["messages"].to<JsonArray>();
}

void LLMDecisionEngine::registerFunction(const String& name, const String& description, const JsonDocument & parameterSchema, FunctionHandler handler) {
    FunctionSpec spec(description, parameterSchema, handler);
    _functionRegistry.insert({name, spec});
}

void LLMDecisionEngine::buildFunctionSchema() {
  // まず registry をクリア
  _functionRegistry.clear();

  // activeProviders から登録
  for (auto* provider : _activeProviders) {
    provider->registerFunctions(*this);
  }

  // tools JSONをbuild
  JsonArray tools = _toolDefinition["tools"].to<JsonArray>();
  _toolDefinition["tool_choice"] = "auto";
  tools.clear();
  
  for (const auto& entry : _functionRegistry) {
    JsonObject tool = tools.add<JsonObject>();
    tool["type"] = "function";
    JsonObject fn = tool["function"].to<JsonObject>();
    fn["name"] = entry.first;
    fn["description"] = entry.second.description;
    fn["parameters"].set(entry.second.parameterSchema);
  }
}

void LLMDecisionEngine::setActiveProviders(const std::vector<IFunctionProvider*>& providers) {
  _activeProviders = providers;
}

void LLMDecisionEngine::setSystemPrompt(const String& prompt) {
  _systemPrompt = prompt;
  rebuildChatHistory();
}

void LLMDecisionEngine::addMessage(const String& role, const String& user, const String& content) {
  JsonArray messages = _chatHistory["messages"].as<JsonArray>();
  JsonObject msg = messages.add<JsonObject>();
  msg["role"] = role;
  msg["content"] = content;
  if (!user.isEmpty() && (role == "user" || role == "assistant")) {
    msg["name"] = user;
  }
}

void LLMDecisionEngine::clearHistory(bool keepSystemPrompt) {
  Serial.printf("⚠️ clearHistory called. keepSystemPrompt=%d\n", keepSystemPrompt ? 1 : 0);

  _chatHistory.clear();
  _chatHistory["messages"].to<JsonArray>();
  if (keepSystemPrompt && !_systemPrompt.isEmpty()) {
    rebuildChatHistory();
  }
}

void LLMDecisionEngine::addFunctionMessage(const String& name, const String& content) {
    JsonArray messages = _chatHistory["messages"].as<JsonArray>();
    JsonObject msg = messages.add<JsonObject>();
    msg["role"] = "function";
    msg["name"] = name;
    msg["content"] = content;

    Serial.printf("📝 Added function message: name=%s, content=%s\n", name.c_str(), content.c_str());
}


void LLMDecisionEngine::rebuildChatHistory() {
  _chatHistory["messages"].clear();
  JsonArray messages = _chatHistory["messages"].to<JsonArray>();

  if (!_systemPrompt.isEmpty()) {
    JsonObject sys = messages.add<JsonObject>();
    sys["role"] = "system";
    sys["content"] = _systemPrompt;
  }
}

bool LLMDecisionEngine::evaluate(String& rawContentOut) {
  buildFunctionSchema();
  injectDynamicSystemRoles();
  String payload = buildRequestJson();
  String response = sendRequest(payload);
  if (!parseResponse(response)) return false;

  JsonObject msg = _responseJson["choices"][0]["message"];
  if (!msg["tool_calls"].isNull()) {
    _functionCallPending = true;
    return true;
  }

  rawContentOut = msg["content"].as<String>();
  return true;
}

bool LLMDecisionEngine::isFunctionCall() {
  Serial.printf("🔍 isFunctionCall(): %s\n", _functionCallPending ? "true" : "false");

  return _functionCallPending;
}

String LLMDecisionEngine::getFunctionName() {
  return _responseJson["choices"][0]["message"]["tool_calls"][0]["function"]["name"].as<String>();
}

JsonObject LLMDecisionEngine::getFunctionArguments() {
  const char* rawArgs = _responseJson["choices"][0]["message"]["tool_calls"][0]["function"]["arguments"];
  static JsonDocument argsDoc;
  DeserializationError err = deserializeJson(argsDoc, rawArgs);

  if (err) {
      Serial.printf("❌ Failed to parse function arguments: %s\n", err.c_str());
  } else {
      String debug;
      serializeJson(argsDoc, debug);
      Serial.printf("🔍 Function arguments: %s\n", debug.c_str());
  }


  return argsDoc.as<JsonObject>();
}

bool LLMDecisionEngine::executeFunction() {
  String name = getFunctionName();
  if (_functionRegistry.count(name) == 0) return false;
  JsonObject args = getFunctionArguments();
  _functionRegistry[name].handler(args);
  _functionCallPending = false;
  return true;
}

String LLMDecisionEngine::buildRequestJson() {
  JsonDocument doc;
  doc["model"] = "gpt-4o-mini";
  doc["messages"] = _chatHistory["messages"];
  doc["tool_choice"] = _toolDefinition["tool_choice"];
  doc["tools"] = _toolDefinition["tools"];

  String out;
  serializeJson(doc, out);
  Serial.printf("🛫 Sending request to LLM: %s\n", out.c_str());
  return out;
}

String LLMDecisionEngine::sendRequest(const String& jsonPayload) {
  HTTPClient https;
  WiFiClientSecure client;
  client.setInsecure();

  https.begin(client, "https://api.openai.com/v1/chat/completions");
  https.addHeader("Content-Type", "application/json");
  https.addHeader("Authorization", "Bearer " + _apiKey);

  int httpCode = https.POST(jsonPayload);
  String response = https.getString();
  Serial.printf("🛬 Received response from LLM: %s\n", response.c_str());

  https.end();
  return response;
}

bool LLMDecisionEngine::parseResponse(const String& jsonResponse) {
  DeserializationError err = deserializeJson(_responseJson, jsonResponse);
  if (err) {
      Serial.printf("❌ Failed to parse response: %s\n", err.c_str());
  } else {
      Serial.printf("✅ Response parsed successfully.\n");
}
  return !err;
}

void LLMDecisionEngine::injectDynamicSystemRoles() {
  _temporarySystemMessages.clear();
  for (auto& provider : _dynamicSystemRoles) {
    String ctx = provider();
    if (!ctx.isEmpty()) {
      addMessage("system", "", ctx);
      _temporarySystemMessages.push_back(ctx);
    }
  }
}

void LLMDecisionEngine::removeTemporarySystemRoles() {
  if (_temporarySystemMessages.empty()) return;

  JsonArray messages = _chatHistory["messages"].as<JsonArray>();
  for (int i = messages.size() - 1; i >= 0; --i) {
    JsonObject msg = messages[i];
    if (msg["role"] == "system") {
      String content = msg["content"].as<String>();
      if (std::find(_temporarySystemMessages.begin(), _temporarySystemMessages.end(), content) != _temporarySystemMessages.end()) {
        messages.remove(i);
      }
    }
  }
  _temporarySystemMessages.clear();
}

void LLMDecisionEngine::addDynamicSystemRole(DynamicSystemRoleProvider provider) {
  _dynamicSystemRoles.push_back(provider);
}
