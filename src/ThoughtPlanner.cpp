#include "ThoughtPlanner.h"
#include "LLMEngine.h" // LLM問い合わせのため
#include <Arduino.h>
#include <vector>
#include <map>
#include <algorithm>


ThoughtPlanner::ThoughtPlanner(LLMEngine* engine) : llmEngine(engine) {
  lastTrigger = millis();
  promptTemplates = {
    "ふと思ったんだけど、",
    "なんとなく気になってるのは、",
    "今日ってさ、"
  };
  Serial.println("[ThoughtPlanner] Initialized.");
}

void ThoughtPlanner::tick() {
  unsigned long now = millis();

  switch (state) {
    case State::Idle:
      if (now - lastTrigger > intervalMs) {
        Serial.println("[ThoughtPlanner] Triggering LLM request...");
        lastTrigger = now;
        state = State::Waiting;
        requestLLM();
      }
      break;

    case State::Waiting:
      // LLM応答待ち（非同期）
      Serial.println("[ThoughtPlanner] Waiting for LLM response...");
      break;

    case State::Ready:
      // 発言が準備完了
      Serial.println("[ThoughtPlanner] Topic is ready to be retrieved.");
      break;

    case State::Prompting:
      // 使用していないが保険
      Serial.println("[ThoughtPlanner] Prompting state, should not happen.");
      break;
  }
}

bool ThoughtPlanner::hasTopic() const {
  if (state == State::Ready) {
    Serial.println("[ThoughtPlanner] Topic is ready.");
  }
  return state == State::Ready;
}

PlannedTopic ThoughtPlanner::getTopic() {
  Serial.println("[ThoughtPlanner] Returning topic: " + currentTopic.text);
  state = State::Idle;
  return currentTopic;
}

void ThoughtPlanner::requestLLM() {
  String prompt = buildPrompt();
  Serial.println("[ThoughtPlanner] Sending prompt to LLM:");
  Serial.println(prompt);

  LLMEngine* engine = llmEngine;
  engine->generate(prompt, [this](const String& response) {
    if (this) {  // 明示的にチェック（ただし無意味なケースもある）
      this->onLLMResponse(response);
    }
  });
}

void ThoughtPlanner::onLLMResponse(const String& response) {
  Serial.println("[ThoughtPlanner] Received response from LLM:");
  Serial.println(response);
  
  currentTopic.text = response;
  currentTopic.intent = IntentType::Chat;
  state = State::Ready;
}

String ThoughtPlanner::buildPrompt() {
  int mode = random(0, 4); // 0〜3の4パターンからランダム
  String suffixPrompt = "\n前後の説明は不要で、スタックチャンが自然に独り言を言うようにしてください。ポエムっぽいものはいらないです。どちらかというと豆知識的な。1度に話すのは１つのトピックでいいですよ。";
  String prompt = "スタックチャンが自然につぶやくとしたら、どんな独り言を言うでしょう？";

  switch (mode) {
    case 0: // 最近の話題に触れる
    {
      String recent = getRecentPhrases();
      prompt = "最近話したこと: " + recent + "\nそのことについてスタックチャンがつぶやくとしたら？";
    }
    case 1: // 関係ない話をふと思いつく
      prompt = "スタックチャンが、突然思いついたことを独り言のようにつぶやいてください。";
    case 2: // 面白い雑談ネタ
      prompt = "スタックチャンが、話題としてふさわしい雑談ネタを自然に言ってください。";
    case 3: // ちょっと謎めいたことをつぶやく
      prompt = "スタックチャンが、意味があるようでないような、ちょっと不思議なことを自然に言ってください。";
  }

  return prompt + suffixPrompt;
}

String ThoughtPlanner::getRecentPhrases() {
  auto history = llmEngine->getHistory();

  std::map<String, std::vector<String>> topicMap;
  for (const auto& entry : history) {
    if (entry.first == "user" || entry.first == "assistant") {
      String topic = classifyTopic(entry.second); // キーワード分類
      topicMap[topic].push_back(entry.second);
    }
  }

  if (topicMap.empty()) return "";

  std::vector<String> topics;
  for (const auto& pair : topicMap) {
    topics.push_back(pair.first);
  }

  String selectedTopic = topics[random(topics.size())];
  const auto& phrases = topicMap[selectedTopic];

  String result;
  for (int i = 0; i < min(2, (int)phrases.size()); ++i) {
    String phrase = phrases[random(phrases.size())];
    result += "「" + phrase + "」";
  }

  Serial.printf("[ThoughtPlanner] Selected topic: %s\n", selectedTopic.c_str());
  Serial.println("[ThoughtPlanner] Sampled phrases: " + result);

  return result;
}

String ThoughtPlanner::classifyTopic(const String& content) {
  if (content.indexOf("晴") >= 0 || content.indexOf("雨") >= 0) return "天気";
  if (content.indexOf("宿題") >= 0 || content.indexOf("学校") >= 0) return "学校";
  if (content.indexOf("ゲーム") >= 0 || content.indexOf("遊") >= 0) return "遊び";
  return "その他";
}