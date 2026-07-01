#ifndef CUSTOM_WIDGET_H
#define CUSTOM_WIDGET_H

#include <lvgl.h>
#include <stdint.h>

// ============================================================================
// Custom Widget Pipeline — on-device dynamic LVGL renderer
//
// A "custom widget" is a user-designed cluster face composed on the phone and
// pushed to the device. The layout is a small JSON document (see the Flutter
// designer + docs spec) that this module parses into a flat element list and
// renders with generic LVGL primitives — one interpreter, no per-design code.
//
// The same JSON contract is consumed by the PC/LVGL simulator so a design
// renders identically on desktop and hardware.
// ============================================================================

// Screen geometry of the round 240x240 panel (elements use this coordinate
// space; the designer canvas mirrors it 1:1).
#define CW_SCREEN 240
#define CW_MAX_ELEMENTS 24
#define CW_LAYOUT_FILE "/custom/layout.json"

// Element kinds supported in v1.
enum CwType : uint8_t {
  CW_GAUGE = 0,   // circular arc gauge bound to a value (lv_arc)
  CW_READOUT,     // live numeric value of a binding (lv_label)
  CW_LABEL,       // static text (lv_label)
  CW_BAR,         // horizontal/vertical progress bound to a value (lv_bar)
  CW_ICON,        // a built-in LVGL symbol (lv_label with symbol font)
  CW_IMAGE,       // placeholder framed box (bitmaps deferred; see spec)
  CW_SHAPE,       // rectangle / circle / line (lv_obj / lv_line)
  CW_TYPE_COUNT
};

// Live data sources an element can bind to. Resolved against sensors_get_*().
enum CwBind : uint8_t {
  CW_BIND_NONE = 0,
  CW_BIND_HEADING,
  CW_BIND_PITCH,
  CW_BIND_ROLL,
  CW_BIND_TEMPERATURE,
  CW_BIND_ALTITUDE,
  CW_BIND_PRESSURE,
  CW_BIND_COUNT
};

// Shape sub-kinds for CW_SHAPE.
enum CwShape : uint8_t { CW_SHAPE_RECT = 0, CW_SHAPE_CIRCLE, CW_SHAPE_LINE };

struct CwElement {
  CwType type;
  int16_t x, y, w, h;       // top-left + size in screen space
  uint32_t color;           // 0xRRGGBB
  CwBind bind;              // live binding (or CW_BIND_NONE)
  float vmin, vmax;         // value range for gauge/bar
  uint8_t font;             // 0=small 1=med 2=large (readout/label/icon size)
  uint8_t shape;            // CwShape when type==CW_SHAPE
  uint8_t filled;           // shape/image fill flag
  char text[24];            // static text (label) / unit suffix (readout)
  char icon[16];            // LVGL symbol token for CW_ICON (e.g. "GPS")

  // Live LVGL handle created during render (not serialized).
  lv_obj_t *obj;
};

struct CwLayout {
  char name[24];
  uint32_t bg;              // background colour 0xRRGGBB
  uint8_t count;
  CwElement elements[CW_MAX_ELEMENTS];
};

// Parse a JSON layout document into `out`. Returns false on malformed input;
// on failure `out` is left in a safe (empty) state.
bool cw_parse_json(const char *json, size_t len, CwLayout *out);

// Render `layout` into `parent` (creates LVGL objects, fills element.obj).
// Any previously rendered layout on `parent` should be cleared by the caller.
void cw_render(lv_obj_t *parent, CwLayout *layout);

// Refresh all live-bound elements of the currently rendered layout from the
// latest sensor values. Cheap; safe to call at UI cadence (~10-20Hz).
void cw_update(CwLayout *layout);

// Persist / load the active layout to SPIFFS (survives reboot).
bool cw_save(const CwLayout *layout);
bool cw_load(CwLayout *out);

// Map a binding token string (as sent by the app) to a CwBind enum.
CwBind cw_bind_from_str(const char *s);

#endif // CUSTOM_WIDGET_H
