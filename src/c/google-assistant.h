#pragma once

#include <pebble.h>
#include <string.h>


#define DOTSHEIGHT 28
#define PADDING 10
#define MAX_MESSAGES 32
#define MAX_MESSAGE_LENGTH 512

#define GAssistantResponse 9

typedef struct MessageBubble {
  // the text displayed in the bubble
  char text[512];

  // user or assistant?
  bool is_user;

} MessageBubble;