#include "custom_widget.h"

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <math.h>
#include <string.h>

#include "sensors.h"

// ---------------------------------------------------------------------------
// Binding resolution
// ---------------------------------------------------------------------------
CwBind cw_bind_from_str(const char *s) {
  if (!s) return CW_BIND_NONE;
  if (!strcmp(s, "heading")) return CW_BIND_HEADING;
  if (!strcmp(s, "pitch")) return CW_BIND_PITCH;
  if (!strcmp(s, "roll")) return CW_BIND_ROLL;
  if (!strcmp(s, "temp") || !strcmp(s, "temperature")) return CW_BIND_TEMPERATURE;
  if (!strcmp(s, "altitude") || !strcmp(s, "alt")) return CW_BIND_ALTITUDE;
  if (!strcmp(s, "pressure")) return CW_BIND_PRESSURE;
  return CW_BIND_NONE;
}

static float cw_bind_value(CwBind b) {
  switch (b) {
    case CW_BIND_HEADING: return sensors_get_heading();
    case CW_BIND_PITCH: return sensors_get_pitch();
    case CW_BIND_ROLL: return sensors_get_roll();
    case CW_BIND_TEMPERATURE: return sensors_get_temperature();
    case CW_BIND_ALTITUDE: return sensors_get_altitude();
    case CW_BIND_PRESSURE: return sensors_get_pressure();
    default: return 0.0f;
  }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static lv_color_t cw_color(uint32_t rgb) {
  return lv_color_make((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

static const lv_font_t *cw_font(uint8_t f) {
  switch (f) {
    case 0: return &lv_font_montserrat_14;
    case 1: return &lv_font_montserrat_22;
    default: return &lv_font_montserrat_34;
  }
}

// LVGL exposes named symbols; map a short token to the glyph string.
static const char *cw_symbol(const char *tok) {
  if (!strcmp(tok, "GPS")) return LV_SYMBOL_GPS;
  if (!strcmp(tok, "WIFI")) return LV_SYMBOL_WIFI;
  if (!strcmp(tok, "BLUETOOTH")) return LV_SYMBOL_BLUETOOTH;
  if (!strcmp(tok, "BATTERY")) return LV_SYMBOL_BATTERY_FULL;
  if (!strcmp(tok, "CHARGE")) return LV_SYMBOL_CHARGE;
  if (!strcmp(tok, "BELL")) return LV_SYMBOL_BELL;
  if (!strcmp(tok, "SETTINGS")) return LV_SYMBOL_SETTINGS;
  if (!strcmp(tok, "POWER")) return LV_SYMBOL_POWER;
  if (!strcmp(tok, "PLAY")) return LV_SYMBOL_PLAY;
  if (!strcmp(tok, "WARNING")) return LV_SYMBOL_WARNING;
  return LV_SYMBOL_GPS;
}

// Format a live value for a readout, appending the element's unit suffix.
static void cw_format_value(const CwElement *e, float v, char *out, size_t n) {
  // heading/altitude read cleaner as integers; temps/pressure keep 1 decimal.
  if (e->bind == CW_BIND_HEADING || e->bind == CW_BIND_ALTITUDE) {
    snprintf(out, n, "%d%s", (int)lroundf(v), e->text);
  } else {
    snprintf(out, n, "%.1f%s", v, e->text);
  }
}

// ---------------------------------------------------------------------------
// JSON parsing
// ---------------------------------------------------------------------------
static uint32_t cw_parse_hex(const char *s, uint32_t fallback) {
  if (!s || s[0] != '#') return fallback;
  return (uint32_t)strtoul(s + 1, nullptr, 16) & 0xFFFFFF;
}

bool cw_parse_json(const char *json, size_t len, CwLayout *out) {
  memset(out, 0, sizeof(CwLayout));
  out->bg = 0x0A0A0C;
  strncpy(out->name, "Custom", sizeof(out->name) - 1);

  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, json, len);
  if (err) return false;

  const char *nm = doc["name"] | "Custom";
  strncpy(out->name, nm, sizeof(out->name) - 1);
  out->bg = cw_parse_hex(doc["bg"] | "#0A0A0C", 0x0A0A0C);

  JsonArray els = doc["elements"].as<JsonArray>();
  if (els.isNull()) return false;

  uint8_t i = 0;
  for (JsonObject el : els) {
    if (i >= CW_MAX_ELEMENTS) break;
    CwElement *e = &out->elements[i];

    const char *ts = el["type"] | "label";
    if (!strcmp(ts, "gauge")) e->type = CW_GAUGE;
    else if (!strcmp(ts, "readout")) e->type = CW_READOUT;
    else if (!strcmp(ts, "bar")) e->type = CW_BAR;
    else if (!strcmp(ts, "icon")) e->type = CW_ICON;
    else if (!strcmp(ts, "image")) e->type = CW_IMAGE;
    else if (!strcmp(ts, "shape")) e->type = CW_SHAPE;
    else e->type = CW_LABEL;

    e->x = el["x"] | 0;
    e->y = el["y"] | 0;
    e->w = el["w"] | 60;
    e->h = el["h"] | 60;
    e->color = cw_parse_hex(el["color"] | "#C8C9CC", 0xC8C9CC);
    e->bind = cw_bind_from_str(el["bind"] | "");
    e->vmin = el["min"] | 0.0f;
    e->vmax = el["max"] | 100.0f;
    e->font = el["font"] | 1;
    e->filled = (el["filled"] | true) ? 1 : 0;

    const char *sh = el["shape"] | "rect";
    if (!strcmp(sh, "circle")) e->shape = CW_SHAPE_CIRCLE;
    else if (!strcmp(sh, "line")) e->shape = CW_SHAPE_LINE;
    else e->shape = CW_SHAPE_RECT;

    strncpy(e->text, el["text"] | "", sizeof(e->text) - 1);
    strncpy(e->icon, el["icon"] | "GPS", sizeof(e->icon) - 1);
    e->obj = nullptr;
    i++;
  }
  out->count = i;
  return true;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------
static void cw_render_gauge(lv_obj_t *parent, CwElement *e) {
  lv_obj_t *arc = lv_arc_create(parent);
  lv_obj_set_pos(arc, e->x, e->y);
  lv_obj_set_size(arc, e->w, e->h);
  lv_arc_set_rotation(arc, 135);
  lv_arc_set_bg_angles(arc, 0, 270);
  lv_arc_set_range(arc, (int16_t)e->vmin, (int16_t)e->vmax);
  lv_obj_remove_style(arc, nullptr, LV_PART_KNOB);
  lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_color(arc, cw_color(e->color), LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(arc, 8, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc, cw_color(0x2A2A30), LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc, 8, LV_PART_MAIN);
  lv_arc_set_value(arc, (int16_t)e->vmin);
  e->obj = arc;
}

static void cw_render_bar(lv_obj_t *parent, CwElement *e) {
  lv_obj_t *bar = lv_bar_create(parent);
  lv_obj_set_pos(bar, e->x, e->y);
  lv_obj_set_size(bar, e->w, e->h);
  lv_bar_set_range(bar, (int16_t)e->vmin, (int16_t)e->vmax);
  lv_obj_set_style_bg_color(bar, cw_color(0x2A2A30), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar, cw_color(e->color), LV_PART_INDICATOR);
  lv_obj_set_style_radius(bar, 4, LV_PART_MAIN);
  lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR);
  lv_bar_set_value(bar, (int16_t)e->vmin, LV_ANIM_OFF);
  e->obj = bar;
}

static void cw_render_label(lv_obj_t *parent, CwElement *e, const char *txt) {
  lv_obj_t *lbl = lv_label_create(parent);
  lv_obj_set_pos(lbl, e->x, e->y);
  lv_obj_set_style_text_color(lbl, cw_color(e->color), 0);
  lv_obj_set_style_text_font(lbl, cw_font(e->font), 0);
  lv_label_set_text(lbl, txt);
  e->obj = lbl;
}

static void cw_render_shape(lv_obj_t *parent, CwElement *e) {
  lv_obj_t *o = lv_obj_create(parent);
  lv_obj_set_pos(o, e->x, e->y);
  lv_obj_set_size(o, e->w, e->h);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_width(o, e->filled ? 0 : 2, 0);
  lv_obj_set_style_border_color(o, cw_color(e->color), 0);
  lv_obj_set_style_bg_opa(o, e->filled ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
  lv_obj_set_style_bg_color(o, cw_color(e->color), 0);
  if (e->shape == CW_SHAPE_CIRCLE) {
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
  } else if (e->shape == CW_SHAPE_LINE) {
    lv_obj_set_height(o, 2);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  } else {
    lv_obj_set_style_radius(o, 6, 0);
  }
  e->obj = o;
}

static void cw_render_image(lv_obj_t *parent, CwElement *e) {
  // v1: bitmaps are deferred; render a framed placeholder so layouts still
  // compose. (See spec: image bitmap upload is future work.)
  lv_obj_t *o = lv_obj_create(parent);
  lv_obj_set_pos(o, e->x, e->y);
  lv_obj_set_size(o, e->w, e->h);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(o, 2, 0);
  lv_obj_set_style_border_color(o, cw_color(e->color), 0);
  lv_obj_set_style_radius(o, 6, 0);
  e->obj = o;
}

void cw_render(lv_obj_t *parent, CwLayout *layout) {
  lv_obj_set_style_bg_color(parent, cw_color(layout->bg), 0);
  lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

  for (uint8_t i = 0; i < layout->count; i++) {
    CwElement *e = &layout->elements[i];
    switch (e->type) {
      case CW_GAUGE: cw_render_gauge(parent, e); break;
      case CW_BAR: cw_render_bar(parent, e); break;
      case CW_READOUT: {
        char buf[32];
        cw_format_value(e, cw_bind_value(e->bind), buf, sizeof(buf));
        cw_render_label(parent, e, buf);
        break;
      }
      case CW_LABEL: cw_render_label(parent, e, e->text); break;
      case CW_ICON: cw_render_label(parent, e, cw_symbol(e->icon)); break;
      case CW_SHAPE: cw_render_shape(parent, e); break;
      case CW_IMAGE: cw_render_image(parent, e); break;
      default: break;
    }
  }
}

void cw_update(CwLayout *layout) {
  for (uint8_t i = 0; i < layout->count; i++) {
    CwElement *e = &layout->elements[i];
    if (!e->obj || e->bind == CW_BIND_NONE) continue;
    float v = cw_bind_value(e->bind);
    switch (e->type) {
      case CW_GAUGE: lv_arc_set_value(e->obj, (int16_t)lroundf(v)); break;
      case CW_BAR: lv_bar_set_value(e->obj, (int16_t)lroundf(v), LV_ANIM_OFF); break;
      case CW_READOUT: {
        char buf[32];
        cw_format_value(e, v, buf, sizeof(buf));
        lv_label_set_text(e->obj, buf);
        break;
      }
      default: break;
    }
  }
}

// ---------------------------------------------------------------------------
// Persistence (SPIFFS)
// ---------------------------------------------------------------------------
bool cw_save(const CwLayout *layout) {
  if (!SPIFFS.exists("/custom")) SPIFFS.mkdir("/custom");
  File f = SPIFFS.open(CW_LAYOUT_FILE, "w");
  if (!f) return false;

  DynamicJsonDocument doc(8192);
  doc["name"] = layout->name;
  char bg[8];
  snprintf(bg, sizeof(bg), "#%06X", layout->bg);
  doc["bg"] = bg;
  JsonArray els = doc.createNestedArray("elements");
  static const char *typeNames[] = {"gauge",  "readout", "label", "bar",
                                     "icon",   "image",   "shape"};
  static const char *bindNames[] = {"",         "heading",  "pitch",
                                     "temp",     "altitude", "pressure"};
  for (uint8_t i = 0; i < layout->count; i++) {
    const CwElement *e = &layout->elements[i];
    JsonObject o = els.createNestedObject();
    o["type"] = typeNames[e->type];
    o["x"] = e->x;
    o["y"] = e->y;
    o["w"] = e->w;
    o["h"] = e->h;
    char col[8];
    snprintf(col, sizeof(col), "#%06X", e->color);
    o["color"] = col;
    // bindNames index is a compact map; guard against enum drift.
    o["bind"] = (e->bind < (sizeof(bindNames) / sizeof(bindNames[0])))
                    ? bindNames[e->bind]
                    : "";
    o["min"] = e->vmin;
    o["max"] = e->vmax;
    o["font"] = e->font;
    if (e->text[0]) o["text"] = e->text;
  }
  serializeJson(doc, f);
  f.close();
  return true;
}

bool cw_load(CwLayout *out) {
  File f = SPIFFS.open(CW_LAYOUT_FILE, "r");
  if (!f) return false;
  String s = f.readString();
  f.close();
  return cw_parse_json(s.c_str(), s.length(), out);
}
