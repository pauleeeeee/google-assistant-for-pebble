#include "./google-assistant.h"

static Window *s_window;
static DictationSession *s_dictation_session;
//holder char for dictation session initialization
static char s_sent_message[512];
static int s_selected_action = 0;
//number of total messages from the user and assistant
static int s_num_messages;
//array of message bubbles
static MessageBubble s_message_bubbles[MAX_MESSAGES];
//array of text layers that hold the bounds of each message bubble
static TextLayer *s_text_layers[MAX_MESSAGES];
//all text layers are added as children to the scroll layer
static ScrollLayer *s_scroll_layer;
//create a layer to draw the animated google assitant "dots"
static Layer *s_canvas_layer;
//create an array of layers for each dot, so that they can each be animated individually
static Layer *s_dot_layer_1;
static Layer *s_dot_layer_2;
static Layer *s_dot_layer_3;
static Layer *s_dot_layer_4;
//create a timer to use to call the animate() function. This timer is set active after a user message and set inactive after an assistant response
static AppTimer *s_timer;
//create a boolean that determines whether or not the bouncing dots should be animated
static bool animated = false;
//toggle state for HYPER mode
static bool s_hyper_mode = false;
// HYPER text layer
static TextLayer *s_hyper_text_layer;



int get_num_messages() {
  return s_num_messages;
}

MessageBubble *get_message_by_id(int id) {
  return &s_message_bubbles[id];
}

//creates a text layer that is appropriately sized AND placed for the message.
TextLayer *render_new_bubble(int index, GRect bounds) {
  //get text from message array
  char *text = s_message_bubbles[index].text;
  //set font
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  //set bold if message is from user
  if (s_message_bubbles[index].is_user){
    font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  } 
  //create a holder GRect to use to measure the size of the final textlayer for the message all integers like '5' and '10' are for padding
  GRect shrinking_rect = GRect(5, 0, bounds.size.w - 10, 2000);
  GSize text_size = graphics_text_layout_get_content_size(text, font, shrinking_rect, GTextOverflowModeWordWrap, GTextAlignmentLeft);
  GRect text_bounds = bounds;

  //set the starting y bounds to the height of the total bounds which was passed into this function by the other functions
  text_bounds.origin.y = bounds.size.h;
  text_bounds.size.h = text_size.h + 5;

  //creates the textlayer with the bounds caluclated above. Size AND position are set this way. This is how we get all the messages stacked on top of each other like a chat
  TextLayer *text_layer = text_layer_create(text_bounds);
  text_layer_set_overflow_mode(text_layer, GTextOverflowModeWordWrap);
  text_layer_set_background_color(text_layer, GColorClear);

  //set alignment depending on whether the message was made by the user or the assistant
  if (s_message_bubbles[index].is_user){
    text_layer_set_text_alignment(text_layer, GTextAlignmentRight);
  } else {
    text_layer_set_text_alignment(text_layer, GTextAlignmentLeft);
  }

  //do not get creative here
  text_layer_set_font(text_layer, font);
  text_layer_set_text(text_layer, text);

  return text_layer;
}

//this function loops through all messages and passes their data to the render_new_bubble function which actually draws each textlayer
static void draw_message_bubbles(){
  //get bounds of the pebble
  Layer *window_layer = window_get_root_layer(s_window);

  //this bounds GRect is very important. It gets modified on each pass through the loop. As new bubbles are created, the bounds are adjusted. The bounds supply information both for the actual size of the scroll layer but also for positioning of each textlayer
  //again, adjusted for padding with 5 and -10
  GRect bounds = GRect(5, 0, (layer_get_bounds(window_layer).size.w - 10), 5);

    //for each message, render a new bubble (stored in the text layers array), adjust the important bounds object, and update the scroll layer's size
    for(int index = 0; index < s_num_messages; index++) {
      //render the bubble for the message
      s_text_layers[index] = render_new_bubble(index, bounds);
      //adjust bounds based on the height of the bubble rendered
      bounds.size.h = bounds.size.h + text_layer_get_content_size(s_text_layers[index]).h + 10;
      //set scroll layer content size so everything is shown and is scrollable
      scroll_layer_set_content_size(s_scroll_layer, bounds.size);
      //add the newly rendered bubble to the scroll layer. Again, its position was set by passing the bounds to the render_new_bubble function
      scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layers[index]));
    }
  //after the bubbles have been drawn and added to the scroll layer, this function scrolls the scroll layer to the given offset. In this case: the bottom of the screen (to the y coordinate of the bounds object)
  scroll_layer_set_content_offset(s_scroll_layer, GPoint(0,-bounds.size.h), true);
}

//adds a new message to the messages array but does not render anything
static void add_new_message(char *text, bool is_user){
  //safeguard to prevent too many bubbles being added
  //I have no experience managing memory so review here would be great. let's get as many messages as possible!
  if(s_num_messages < MAX_MESSAGES && strlen(text) > 0) {
    //set the message text and the is_user bool
    strncpy(s_message_bubbles[s_num_messages].text, text, MAX_MESSAGE_LENGTH - 1);
    s_message_bubbles[s_num_messages].is_user = is_user;
    //increment the number of messages int. This int is used throughout the other functions
    s_num_messages++;
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Failed to add message to bubble list; exceeded maximum number of bubbles");
  }
}


//this function actually animates the dots. Again, maybe this could be reduced with clever use of loops and arrays
static void animate(){
  //duration for all dots
  uint16_t duration = 250;

  //dot 1
  GRect dot_bounds_1 = layer_get_frame(s_dot_layer_1);
  //up
  PropertyAnimation *dot_prop_animation_up_1 = property_animation_create_layer_frame(s_dot_layer_1, &dot_bounds_1, &GRect(dot_bounds_1.origin.x, 2, dot_bounds_1.size.w, 12));
  Animation *dot_animation_up_1 = property_animation_get_animation(dot_prop_animation_up_1);
  animation_set_duration(dot_animation_up_1, duration);
  animation_set_curve(dot_animation_up_1, AnimationCurveEaseOut);
  //down
  PropertyAnimation *dot_prop_animation_down_1 = property_animation_create_layer_frame(s_dot_layer_1, &GRect(dot_bounds_1.origin.x, 2, dot_bounds_1.size.w, 12), &dot_bounds_1);
  Animation *dot_animation_down_1 = property_animation_get_animation(dot_prop_animation_down_1);
  animation_set_duration(dot_animation_down_1, duration);
  animation_set_curve(dot_animation_down_1, AnimationCurveEaseIn);
  // sequence for dot 1
  Animation *dot_sequence_1 = animation_sequence_create(dot_animation_up_1, dot_animation_down_1, NULL);
  // play the dot 1 sequence
  animation_schedule(dot_sequence_1);

  //dot 2
  GRect dot_bounds_2 = layer_get_frame(s_dot_layer_2);
  //up
  PropertyAnimation *dot_prop_animation_up_2 = property_animation_create_layer_frame(s_dot_layer_2, &dot_bounds_2, &GRect(dot_bounds_2.origin.x, 2, dot_bounds_2.size.w, 12));
  Animation *dot_animation_up_2 = property_animation_get_animation(dot_prop_animation_up_2);
  //small delay to give wave effect
  animation_set_delay(dot_animation_up_2, 75);
  animation_set_duration(dot_animation_up_2, duration);
  animation_set_curve(dot_animation_up_2, AnimationCurveEaseOut);
  //down
  PropertyAnimation *dot_prop_animation_down_2 = property_animation_create_layer_frame(s_dot_layer_2, &GRect(dot_bounds_2.origin.x, 2, dot_bounds_2.size.w, 12), &dot_bounds_2);
  Animation *dot_animation_down_2 = property_animation_get_animation(dot_prop_animation_down_2);
  animation_set_duration(dot_animation_down_2, duration);
  animation_set_curve(dot_animation_down_2, AnimationCurveEaseIn);
  // sequence for dot 2
  Animation *dot_sequence_2 = animation_sequence_create(dot_animation_up_2, dot_animation_down_2, NULL);
  // play the dot 2 sequence
  animation_schedule(dot_sequence_2);

  //dot 3
  GRect dot_bounds_3 = layer_get_frame(s_dot_layer_3);
  //up
  PropertyAnimation *dot_prop_animation_up_3 = property_animation_create_layer_frame(s_dot_layer_3, &dot_bounds_3, &GRect(dot_bounds_3.origin.x, 2, dot_bounds_3.size.w, 12));
  Animation *dot_animation_up_3 = property_animation_get_animation(dot_prop_animation_up_3);
  //small delay to give wave effect
  animation_set_delay(dot_animation_up_3, 150);
  animation_set_duration(dot_animation_up_3, duration);
  animation_set_curve(dot_animation_up_3, AnimationCurveEaseOut);
  //down
  PropertyAnimation *dot_prop_animation_down_3 = property_animation_create_layer_frame(s_dot_layer_3, &GRect(dot_bounds_3.origin.x, 2, dot_bounds_3.size.w, 12), &dot_bounds_3);
  Animation *dot_animation_down_3 = property_animation_get_animation(dot_prop_animation_down_3);
  animation_set_duration(dot_animation_down_3, duration);
  animation_set_curve(dot_animation_down_3, AnimationCurveEaseIn);
  // sequence for dot 3
  Animation *dot_sequence_3 = animation_sequence_create(dot_animation_up_3, dot_animation_down_3, NULL);
  // play the dot 3 sequence
  animation_schedule(dot_sequence_3);

  //dot 3
  GRect dot_bounds_4 = layer_get_frame(s_dot_layer_4);
  //up
  PropertyAnimation *dot_prop_animation_up_4 = property_animation_create_layer_frame(s_dot_layer_4, &dot_bounds_4, &GRect(dot_bounds_4.origin.x, 2, dot_bounds_4.size.w, 12));
  Animation *dot_animation_up_4 = property_animation_get_animation(dot_prop_animation_up_4);
  //small delay to give wave effect
  animation_set_delay(dot_animation_up_4, 225);
  animation_set_duration(dot_animation_up_4, duration);
  animation_set_curve(dot_animation_up_4, AnimationCurveEaseOut);
  //down
  PropertyAnimation *dot_prop_animation_down_4 = property_animation_create_layer_frame(s_dot_layer_4, &GRect(dot_bounds_4.origin.x, 2, dot_bounds_4.size.w, 12), &dot_bounds_4);
  Animation *dot_animation_down_4 = property_animation_get_animation(dot_prop_animation_down_4);
  animation_set_duration(dot_animation_down_4, duration);
  animation_set_curve(dot_animation_down_4, AnimationCurveEaseIn);
  // sequence for dot 4
  Animation *dot_sequence_4 = animation_sequence_create(dot_animation_up_4, dot_animation_down_4, NULL);
  // play the dot 4 sequence
  animation_schedule(dot_sequence_4);

}

//callback for the timer to check to see whether or not we should play the animation.
static void check_animate(){
  if(animated){
    animate();
    app_timer_register(1250, check_animate, NULL);
  }
}

//standard dictation callback
static void handle_transcription(char *transcription_text) {
  //makes a dictionary which is used to send the phone app the results of the transcription
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  //writes the transcription and the HYPER mode state to the dictionary
  int hyper_mode = (s_hyper_mode) ? 1 : 0;
  dict_write_int(iter, 0, &hyper_mode, sizeof(int), false);
  dict_write_cstring(iter, 1, transcription_text);
  ;

  //sends the dictionary through an appmessage to the pebble phone app
  app_message_outbox_send();

  //adds the transcription to the messages array so it is displayed as a bubble. true indicates that this message is from a user
  add_new_message(transcription_text, true);

  //update the view so new message is drawn
  draw_message_bubbles();

  //start the dot bounce animation
  animated = true;
  app_timer_register(250, check_animate, NULL);

}

static void dictation_session_callback(DictationSession *session, DictationSessionStatus status, char *transcription, void *context) {
  if(status == DictationSessionStatusSuccess) {
    handle_transcription(transcription);
  } else {
    //handle error
  }
}

static void toggle_hyper_mode(){
  if(s_hyper_mode) {
    //show text layer
    text_layer_set_text_color(s_hyper_text_layer, GColorBlack);
  } else {
    //hide text layer
    text_layer_set_text_color(s_hyper_text_layer, GColorLightGray);
  }
}

//starts dictation if the user short presses select.
static void select_callback(ClickRecognizerRef recognizer, void *context) {
  dictation_session_start(s_dictation_session);
}

//open pre-filled action menu
static void long_select_callback(ClickRecognizerRef recognizer, void *context) {
  // handle_transcription("How many times have we gone to the moon?");
    dictation_session_start(s_dictation_session);

}

//toggle HYPER mode
static void long_down_callback(ClickRecognizerRef recognizer, void *context) {
  s_hyper_mode = !s_hyper_mode;
  toggle_hyper_mode();

}

//this click config provider is not really used
static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_callback);
}

//click config provider for the scroll window, setting both short and long select pushes.
static void prv_scroll_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_callback);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, long_select_callback, NULL);
  window_long_click_subscribe(BUTTON_ID_DOWN, 500, long_down_callback, NULL);

}

//when the app.js/phone app sends the pebble app an appmessage, this function is called.
static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *response_tuple = dict_find( iter, GAssistantResponse );
  if (response_tuple) {
    //if an appmessage has a GAssistantResponse key, read the value and add a new message. Since this is the assistant, the is_user flag is set to false
    add_new_message(response_tuple->value->cstring, false);
    //stop animation since the assistant responded
    animated = false;
    //gotta update the view
    draw_message_bubbles();
  } else {
    //basically this should never happen
    add_new_message("tuple error", false);
    animated = false;
    draw_message_bubbles();
  }

  // Tuple *ready_tuple = dict_find(iter, MESSAGE_KEY_APP_READY);
  // if (ready_tuple) {
  //   prv_lockitron_toggle_state();
  //   return;
  // }
}

static void in_dropped_handler(AppMessageResult reason, void *context){
  //handle failed message
  add_new_message("dropped message", false);
  draw_message_bubbles();
  animated = false;
}


//draw each dot with a 4 pixel radius located at 4, 4 in the child layer's coordinate plane; assign appropriate color
static void dot_layer_1_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBlueMoon, GColorLightGray));
  graphics_fill_circle(ctx, GPoint(4, 4), 4);
}

static void dot_layer_2_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorSunsetOrange, GColorLightGray));
  graphics_fill_circle(ctx, GPoint(4, 4), 4);
}

static void dot_layer_3_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorLightGray));
  graphics_fill_circle(ctx, GPoint(4, 4), 4);
}

static void dot_layer_4_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorMayGreen, GColorLightGray));
  graphics_fill_circle(ctx, GPoint(4, 4), 4);
}

//   // Fill a circle
//   // blue = GColorBlueMoon
//   // red = GColorSunsetOrange
//   // yellow = GColorChromeYellow
//   // green = GColorMayGreen
//   // graphics_fill_circle(ctx, center, radius);
// }

static void prv_window_load(Window *window) {
  //root layer
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);


  s_hyper_text_layer  = text_layer_create(GRect(0,bounds.size.h - 20, bounds.size.w, bounds.size.h));
    text_layer_set_font(s_hyper_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text(s_hyper_text_layer, "HYPER");
    text_layer_set_text_alignment(s_hyper_text_layer, GTextAlignmentRight);

  if(s_hyper_mode){
    text_layer_set_text_color(s_hyper_text_layer, GColorBlack);
  } else {
    text_layer_set_text_color(s_hyper_text_layer, GColorLightGray);
  }

  layer_add_child(window_layer, text_layer_get_layer(s_hyper_text_layer));

  //create dictation and set confirmation mode to false
  s_dictation_session = dictation_session_create(sizeof(s_sent_message), dictation_session_callback, NULL);
  dictation_session_enable_confirmation(s_dictation_session, false);

  //create scroll layer with shortened root layer bounds ( to accommodate bouncing dots )
  GRect scroll_bounds = GRect(0, 0, bounds.size.w, bounds.size.h - 20);
  s_scroll_layer = scroll_layer_create(scroll_bounds);
  // Set the scrolling content size
  scroll_layer_set_content_size(s_scroll_layer, GSize(scroll_bounds.size.w, scroll_bounds.size.h-2));
  //we could make the scroll layer page instead of single click scroll but sometimes that leads to UI problems
  scroll_layer_set_paging(s_scroll_layer, true);
  // set the click config provider of the scroll layer. This adds the short and long select press callbacks. it silently preserves scrolling functionality
  scroll_layer_set_callbacks(s_scroll_layer, (ScrollLayerCallbacks){
    .click_config_provider = prv_scroll_click_config_provider
  });
  //take the scroll layer's click config and place it onto the main window
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);

  //add scroll layer to the root window
  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));


  // Create the holding layer for the google assistant dots which will be drawn as individual layers next
  GRect dots_bounds = GRect(0, bounds.size.h - 20, bounds.size.w, bounds.size.h);
  s_canvas_layer = layer_create(dots_bounds);

  //calculate dot offset so it is centered for every pebble
  uint16_t offset = (bounds.size.w - 48) / 2;

  // create individual layers for each dot; remember that each child layer's coordinate system is relative to its parent
  // we need each dot to have its own layer so that it can be animated seperately
  // maybe this could be done with a loop and an array but then the update_procs would also need to be programatically set and defined
  GRect dot_bounds = GRect(offset, 10, offset + 8, 18);
  s_dot_layer_1 = layer_create(dot_bounds);
  layer_set_update_proc(s_dot_layer_1, dot_layer_1_update_proc);
  layer_add_child(s_canvas_layer, s_dot_layer_1);
  //increment offset
  dot_bounds.origin.x += 12;
  dot_bounds.size.w += 12;
  //next dot
  s_dot_layer_2 = layer_create(dot_bounds);
  layer_set_update_proc(s_dot_layer_2, dot_layer_2_update_proc);
  layer_add_child(s_canvas_layer, s_dot_layer_2);
  //increment offset
  dot_bounds.origin.x += 12;
  dot_bounds.size.w += 12;
  //next dot
  s_dot_layer_3 = layer_create(dot_bounds);
  layer_set_update_proc(s_dot_layer_3, dot_layer_3_update_proc);
  layer_add_child(s_canvas_layer, s_dot_layer_3);
  //increment offset
  dot_bounds.origin.x += 12;
  dot_bounds.size.w += 12;
  //next dot
  s_dot_layer_4 = layer_create(dot_bounds);
  layer_set_update_proc(s_dot_layer_4, dot_layer_4_update_proc);
  layer_add_child(s_canvas_layer, s_dot_layer_4);

  
  //add canvas to window
  layer_add_child(window_layer, s_canvas_layer);


  //start dictation on first launch
  dictation_session_start(s_dictation_session);

  //testing
  // add_new_message("test test test test test test test test test test test test test test test test test test ", true);
  // add_new_message("test test test test test test test test test test test test test test test test test test test test test test test test test test test test test test ", false);
  // draw_message_bubbles();

}

static void prv_window_unload(Window *window) {
  dictation_session_destroy(s_dictation_session);
  layer_destroy(s_dot_layer_1);
  layer_destroy(s_dot_layer_2);
  layer_destroy(s_dot_layer_3);
  layer_destroy(s_dot_layer_4);
  layer_destroy(s_canvas_layer);
  scroll_layer_destroy(s_scroll_layer);
  //probably need to destroy other stuff here like the s_message_bubbles and the s_text_layers arrays
}

static void prv_init(void) {
  s_window = window_create();
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  //instantiate appmessages
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
