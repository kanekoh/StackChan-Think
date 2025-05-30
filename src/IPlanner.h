#pragma once
#include <Arduino.h>

enum class IntentType {
  Chat,
  Task,
  Unknown
};

struct PlannedTopic {
  String text;
  IntentType intent;
};

class IPlanner {
public:
  typedef void (*TopicCallback)(const PlannedTopic&);
  virtual void tick() = 0;
  virtual bool hasTopic() const = 0;
  virtual PlannedTopic getTopic() = 0;
  virtual void resetTiming() = 0;
};
