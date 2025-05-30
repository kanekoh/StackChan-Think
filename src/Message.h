#pragma once
#include <Arduino.h>

/**
 * A single chat turn sent to the LLM.
 *   role    – "system", "user", or "assistant"
 *   content – plain-text payload
 */
struct Message {
  String role;
  String content;

  Message(const String& r = "", const String& c = "")
      : role(r), content(c) {}
};
