#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <SDL2/SDL.h>

#define PROGMEM
#define IRAM_ATTR

#define OUTPUT 1
#define INPUT 0
#define INPUT_PULLUP 2
#define HIGH 1
#define LOW 0
#define CHANGE 1
#define FALLING 2
#define RISING 3

#define constrain(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))
#define analogWrite(pin, val) ((void)0)

// Serial stub - maps Arduino Serial to stdout
struct SerialStub {
  void begin(int) {}
  void print(const char *s) { if (s) fputs(s, stdout); }
  void print(int v) { fprintf(stdout, "%d", v); }
  void print(float v) { fprintf(stdout, "%g", v); }
  void print(double v) { fprintf(stdout, "%g", v); }
  void println() { fputc('\n', stdout); }
  void println(const char *s) { if (s) fprintf(stdout, "%s\n", s); else fputc('\n', stdout); }
  void println(int v) { fprintf(stdout, "%d\n", v); }
  void println(float v) { fprintf(stdout, "%g\n", v); }
  void println(double v) { fprintf(stdout, "%g\n", v); }
  void printf(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vfprintf(stdout, fmt, ap); va_end(ap); }
};
static SerialStub Serial;

inline unsigned long millis() { return SDL_GetTicks(); }
inline unsigned long micros() { return SDL_GetTicks() * 1000; }
inline void delay(unsigned long ms) { SDL_Delay(ms); }
inline void delayMicroseconds(unsigned long us) { SDL_Delay(us / 1000 + 1); }
inline void pinMode(int, int) {}
inline void digitalWrite(int pin, int val) {}
inline int digitalRead(int pin) { return 0; }
inline void attachInterrupt(int, void (*)(), int) {}

inline bool getLocalTime(struct tm *info) {
  time_t now = time(nullptr);
  if (localtime_s(info, &now) != 0) return false;
  return true;
}

static int random(int max) { return rand() % max; }
static long random(long min, long max) { return min + rand() % (max - min); }
inline void randomSeed(unsigned long seed) { srand(seed); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define abs(x) ((x) > 0 ? (x) : -(x))
