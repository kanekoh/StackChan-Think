#pragma once
#include <Arduino.h>
#include <vector>

class IntentClassifier {
public:
  IntentClassifier(const String& apiKey);
  String classify(const String& userInput, const std::vector<String>& intents);
  
private:
  String _apiKey;
};
