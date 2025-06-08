// LLMDecisionEngine.h
#pragma once

#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <map>
#include <functional>
#include "IFunctionProvider.h"

class LLMDecisionEngine {
public:
  using FunctionHandler = std::function<void(JsonObject)>;

  struct FunctionSpec {
    String description;
    JsonDocument parameterSchema; // Store full document to retain scope
    FunctionHandler handler;

    FunctionSpec() : handler(nullptr) {}

    FunctionSpec(const String& desc, const JsonDocument& schema, FunctionHandler h)
      : description(desc), handler(h) {
      parameterSchema.set(schema);  // コピーする
    }
  };

  LLMDecisionEngine(const String& apiKey);

  void setSystemPrompt(const String& prompt);
  void addMessage(const String& role, const String& content);
  void clearHistory(bool keepSystemPrompt = true);

  // Automatically build tool schema from registered functions
  void buildFunctionSchema();

  // Evaluate and extract structured response
  bool evaluate(String& rawContentOut);

  // For function_call-based flows
  bool isFunctionCall();
  String getFunctionName();
  JsonObject getFunctionArguments();

  // Register and execute function calls
  void registerFunction(const String& name, const String& description, const JsonDocument& parameterSchema, FunctionHandler handler);
  bool executeFunction();

  void setActiveProviders(const std::vector<IFunctionProvider*>& providers);

private:
  String _apiKey;
  JsonDocument _chatHistory;
  JsonDocument _responseJson;
  JsonDocument _toolDefinition;
  bool _functionCallPending;

  std::map<String, FunctionSpec> _functionRegistry;
  String _systemPrompt;
  std::vector<IFunctionProvider*> _activeProviders;

  String buildRequestJson();
  String sendRequest(const String& jsonPayload);
  bool parseResponse(const String& jsonResponse);

  void rebuildChatHistory();

};
