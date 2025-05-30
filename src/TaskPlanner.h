#pragma once
#include "IPlanner.h"

class TaskPlanner : public IPlanner {
public:
  void tick() override {
    if (_taskDueSoon && !_alreadyAsked) {
      _nextTopic.text = "そろそろこのタスクやりましょうか？";
      _nextTopic.intent = IntentType::Task;
      _alreadyAsked = true;
    }
  }

  bool hasTopic() const override {
    return !_nextTopic.text.isEmpty();
  }

  PlannedTopic getTopic() override {
    PlannedTopic t = _nextTopic;
    _nextTopic.text = "";
    return t;
  }

private:
  bool _taskDueSoon = true;  // デモ用
  bool _alreadyAsked = false;
  PlannedTopic _nextTopic;
};
