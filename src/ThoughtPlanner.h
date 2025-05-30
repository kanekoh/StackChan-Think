#pragma once
#include "IPlanner.h"
#include "LLMEngine.h"  // ← これを追加
#include <vector>

class ThoughtPlanner : public IPlanner {
public:
  ThoughtPlanner(LLMEngine* engine);  // ← ポインタ渡し

  void tick() override;
  bool hasTopic() const override;
  PlannedTopic getTopic() override;

private:
  enum State {
    Idle,
    Prompting,
    Waiting,
    Ready
  };

  volatile State state = State::Idle;
  unsigned long lastTrigger = 0;
  unsigned long intervalMs = 600000;
  PlannedTopic currentTopic;
  LLMEngine* llmEngine; // LLMエンジンインスタンス

  void requestLLM(); // LLMにプロンプト送信
  void onLLMResponse(const String& response); // コールバック

  std::vector<String> promptTemplates;

  String buildPrompt();
  String getRecentPhrases();
  String classifyTopic(const String& content);
  void resetTiming() override { lastTrigger = millis(); }
};
