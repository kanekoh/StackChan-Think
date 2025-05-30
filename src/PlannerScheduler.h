// PlannerScheduler.h
#pragma once
#include <vector>
#include "IPlanner.h"
#include "EngineManager.h"
#include "SpeechEngine.h"

class PlannerScheduler {
public:
  PlannerScheduler(EngineManager* engineMgr)
    : engineManager(engineMgr) {}

  void addPlanner(IPlanner* planner) {
    planners.push_back(planner);
  }

  void tick() {
    if (!engineManager->canTalk() || SpeechEngine::isSpeaking()) return;

    if (engineManager->getState() == InteractionState::Speaking) {
      engineManager->setState(InteractionState::Idle);
    }

    for (auto planner : planners) {
      planner->tick();
      if (planner->hasTopic()) {
        PlannedTopic topic = planner->getTopic();
        SpeechEngine::enqueueText(topic.text);
        engineManager->setState(InteractionState::Speaking);

        // ThoughtPlannerのタイミングを初期化する
        planner->resetTiming();

        break;
      }
    }
  }

private:
  std::vector<IPlanner*> planners;
  EngineManager* engineManager;
};
