#include <pebble.h>
#include "twatch.h"

#if 0
#define BLACK
#endif

#if 0
#define SECONDS
#endif

#ifdef BLACK
#define FG GColorWhite
#define BG GColorBlack
#else
#define FG GColorBlack
#define BG GColorWhite
#endif

static Window *window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_hands_layer;
static TextLayer *s_day_label, *s_mon_label;

static GPath *s_tick_paths[NUM_CLOCK_TICKS];
static GPath *s_minute_arrow, *s_hour_arrow;
static char s_mon_buffer[4], s_day_buffer[6];

static TextLayer *s_hour_label[4];
static char s_hour[4][4];

static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, BG);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, FG);
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_draw_filled(ctx, s_tick_paths[i]);
  }

  // Draw numbers
  for (int i = 0; i < 4; i += 1) {
    int hour = 3 * (i+1);

    if (1 && clock_is_24h_style()) {
      hour += 12;
    }

    snprintf(s_hour[i], 3, "%d", hour);
    text_layer_set_text(s_hour_label[i], s_hour[i]);
  }
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
#ifdef SECONDS
  GPoint center = grect_center_point(&bounds);
  int16_t second_hand_length = bounds.size.w / 2;

  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  GPoint second_hand = {
    .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
  };

  // second hand
  graphics_context_set_stroke_color(ctx, FG);
  graphics_draw_line(ctx, second_hand, center);
#endif

  // minute/hour hand
  graphics_context_set_fill_color(ctx, FG);
  graphics_context_set_stroke_color(ctx, BG);

  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);

  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);

  // dot in the middle
  graphics_context_set_fill_color(ctx, FG);
  graphics_fill_circle(ctx, GPoint(bounds.size.w / 2, bounds.size.h / 2), 2);
}

static void date_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char *b = s_day_buffer;

  strftime(s_mon_buffer, sizeof(s_mon_buffer), "%b", t);
  text_layer_set_text(s_mon_label, s_mon_buffer);

  strftime(s_day_buffer, sizeof(s_day_buffer), "%d", t);
  if (b[0] == '0') {
    b += 1;
  }
  text_layer_set_text(s_day_label, b);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(window));
}

#define NUM_HEIGHT 42
#define NUM_WIDTH 50

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  for (int i = 0; i < 4; i += 1) {
    int x, y;
    GTextAlignment align;
    int hour = 3 * (i+1);

    if (1 && clock_is_24h_style()) {
      hour += 12;
    }

    switch (i) {
    case 0:
      x = 140 - (NUM_WIDTH); y = 80 - (NUM_HEIGHT/2);
      align = GTextAlignmentRight;
      break;
    case 1:
      x = 72 - (NUM_WIDTH/2); y = 122;
      align = GTextAlignmentCenter;
      break;
    case 2:
      x = 4; y = 80 - (NUM_HEIGHT/2);
      align = GTextAlignmentLeft;
      break;
    case 3:
      x = 72 - (NUM_WIDTH/2); y = 4;
      align = GTextAlignmentCenter;
      break;
    }

    s_hour_label[i] = text_layer_create(GRect(x, y, NUM_WIDTH, NUM_HEIGHT));
    text_layer_set_text_alignment(s_hour_label[i], align);
    text_layer_set_background_color(s_hour_label[i], BG);
    text_layer_set_text_color(s_hour_label[i], FG);
    text_layer_set_font(s_hour_label[i], fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));

    layer_add_child(s_simple_bg_layer, text_layer_get_layer(s_hour_label[i]));
  }

  s_date_layer = layer_create(bounds);
  layer_set_update_proc(s_date_layer, date_update_proc);
  layer_add_child(window_layer, s_date_layer);

  s_mon_label = text_layer_create(GRect(114, 140, 27, 24));
  text_layer_set_text_alignment(s_mon_label, GTextAlignmentRight);
  text_layer_set_text(s_mon_label, s_day_buffer);
  text_layer_set_background_color(s_mon_label, BG);
  text_layer_set_text_color(s_mon_label, FG);
  text_layer_set_font(s_mon_label, fonts_get_system_font(FONT_KEY_GOTHIC_24));

  layer_add_child(s_date_layer, text_layer_get_layer(s_mon_label));

  s_day_label = text_layer_create(GRect(121, 123, 20, 24));
  text_layer_set_text_alignment(s_day_label, GTextAlignmentRight);
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_background_color(s_day_label, BG);
  text_layer_set_text_color(s_day_label, FG);
  text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));

  layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_date_layer);

  text_layer_destroy(s_day_label);
  text_layer_destroy(s_mon_label);

  layer_destroy(s_hands_layer);
}

static void init() {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  s_day_buffer[0] = '\0';
  s_mon_buffer[0] = '\0';

  // init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    s_tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
  }

#ifdef SECONDS
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
#else
  tick_timer_service_subscribe(MINUTE_UNIT, handle_second_tick);
#endif
}

static void deinit() {
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_destroy(s_tick_paths[i]);
  }

  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
