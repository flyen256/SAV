#include "../include/miniaudio.h"
#include "../include/raylib.h"
#include <stdio.h>
#include <stdlib.h>
#define Rectangle CombaseApiRectangle
#define CloseWindow CombaseApiCloseWindow
#define ShowCursor CombaseApiShowCursor
#include <combaseapi.h>
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#include <complex.h>
#include <limits.h>
#include <math.h>
#include <objbase.h>

#ifdef _WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define ARRAY_LEN(xs) (sizeof(xs) / sizeof((xs)[0]))
#define MAX_FFT_SIZE 2048
#define FFT_SIZE 1024

#define TRAY_WINAPI 1

#define Rectangle WinapiRectangle
#define CloseWindow WinapiCloseWindow
#define ShowCursor WinapiShowCursor
#define DrawText WinuserDrawText

#include "../include/tray.h"

#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#undef DrawText

#define BAR_SIZE 1
#define BAR_GAP 0
#define VISUAL_BARS (FFT_SIZE / 2)

#define WINDOW_WIDTH (VISUAL_BARS * (BAR_SIZE + BAR_GAP))
#define WINDOW_HEIGHT 300

#define MAX_LINE_LENGTH 256

int current_fft_size;

FILE *cfg_file;
bool window_opened = false;
ma_device device;

void trim(char *str) {
  int l = strlen(str);
  while (l > 0 && isspace((unsigned char)str[l - 1])) {
    str[--l] = '\0';
  }
  int start = 0;
  while (str[start] && isspace((unsigned char)str[start])) {
    start++;
  }
  if (start > 0) {
    memmove(str, str + start, l - start + 1);
  }
}

char *read_config_value(const char *key) {
  if (cfg_file == NULL)
    return NULL;
  rewind(cfg_file);

  char line[MAX_LINE_LENGTH];

  while (fgets(line, sizeof(line), cfg_file) != NULL) {
    trim(line);

    if (line[0] == '\0' || line[0] == '#' || line[0] == ';') {
      continue;
    }

    char *delimiter = strchr(line, '=');
    if (delimiter != NULL) {
      *delimiter = '\0';

      char *found_key = line;
      char *value = delimiter + 1;

      trim(found_key);
      trim(value);

      if (strcmp(found_key, key) == 0) {
        return strdup(value);
      }
    }
  }
  return NULL;
}

int read_config_int(const char *key) {
  char *string_value = read_config_value(key);
  if (string_value != NULL) {
    char *endptr;
    long val = strtol(string_value, &endptr, 10);

    if (string_value == endptr) {
      return -1;
    } else {
      return (int)val;
    }
  }
  return -1;
}

float read_config_float(const char *key) {
  char *string_value = read_config_value(key);
  if (string_value != NULL) {
    char *endptr;

    double val = strtod(string_value, &endptr);

    if (string_value == endptr) {
      return -1.0f;
    } else {
      return (float)val;
    }
  }
}

int get_visual_bars() {
  int value = VISUAL_BARS;
  int fft_size = read_config_int("fft_size");
  if (fft_size > 0)
    value = fft_size / 2;
  return value;
}

int get_fft_size() {
  int value = FFT_SIZE;
  int fft_size = read_config_int("fft_size");
  if (fft_size > 0)
    value = fft_size;
  return value;
}

int get_bar_size() {
  int value = BAR_SIZE;
  int bar_size = read_config_int("bar_size");
  if (bar_size > 0)
    value = bar_size;
  return value;
}

int get_bar_gap() {
  int value = BAR_GAP;
  int bar_gap = read_config_int("bar_gap");
  if (bar_gap > 0)
    value = bar_gap;
  return value;
}

int get_window_width() {
  int visual_bars = get_visual_bars();
  int bar_size = get_bar_size();
  int bar_gap = get_bar_gap();
  if (visual_bars < 0 || bar_size < 0 || bar_gap < 0)
    return WINDOW_WIDTH;
  return visual_bars * (bar_size + bar_gap);
}

int get_window_height() {
  int value = WINDOW_HEIGHT;
  int window_height = read_config_int("window_height");
  if (window_height > 0)
    value = window_height;
  return value;
}

float get_bass_boost_multiplier() {
  float value = 4.0f;
  float bass_boost_multiplier = read_config_float("bass_boost_multiplier");
  if (bass_boost_multiplier >= 0)
    value = bass_boost_multiplier;
  return value;
}

float get_volume_multiplier() {
  float value = 1.0f;
  float volume_multiplier = read_config_float("volume_multiplier");
  if (volume_multiplier >= 0)
    value = volume_multiplier;
  return value;
}

void config_window() {
  SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_UNDECORATED);
}

void open_window(int width, int height) {
  if (window_opened)
    return;
  InitWindow(width, height, "Visualizer");
  window_opened = true;
}
void close_window() {
  if (!window_opened)
    return;
  CloseWindow();
  window_opened = false;
}

void tray_close(struct tray_menu *menu) {
  tray_exit();
  close_window();
  ma_device_uninit(&device);

  ExitProcess(0);
}

bool is_click_through_enabled = false;

void set_click_through(bool enable) {
  HWND hwnd = (HWND)GetWindowHandle();

  LONG_PTR style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

  if (enable) {
    SetWindowLongPtr(hwnd, GWL_EXSTYLE,
                     style | WS_EX_TRANSPARENT | WS_EX_LAYERED);

    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
  } else {
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
  }
}

extern struct tray tray;

void tray_open_window(struct tray_menu *menu) {
  if (!window_opened)
    open_window(get_window_width(), get_window_height());
  else
    close_window();
  menu->checked = window_opened;
  tray_update(&tray);
}

void tray_toggle_click_through(struct tray_menu *menu) {
  is_click_through_enabled = !is_click_through_enabled;

  menu->checked = is_click_through_enabled;
  tray_update(&tray);

  set_click_through(is_click_through_enabled);
}

void set_always_on_top(bool enable) {
  HWND hwnd = (HWND)GetWindowHandle();

  if (enable)
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
  else
    SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

bool is_always_on_top_enabled = false;

void tray_toggle_always_on_top(struct tray_menu *menu);

void tray_toggle_always_on_top(struct tray_menu *menu) {
  is_always_on_top_enabled = !is_always_on_top_enabled;
  menu->checked = is_always_on_top_enabled;

  tray_update(&tray);
  set_always_on_top(is_always_on_top_enabled);
}

struct tray tray = {
    .menu = (struct tray_menu[]){
        {"Open", 0, 1, tray_open_window, NULL},
        {"Always on Top", 0, 0, tray_toggle_always_on_top, NULL},
        {"Click-Through (Pass Click)", 0, 0, tray_toggle_click_through, NULL},
        {"Quit", 0, 0, tray_close, NULL},
        {NULL, 0, 0, NULL, NULL}}};

int tray_start() { return tray_init(&tray); }

_Fcomplex fft_input[MAX_FFT_SIZE];
_Fcomplex fft_output[MAX_FFT_SIZE];
float frequencies[MAX_FFT_SIZE];

unsigned int bit_reverse(unsigned int x, int bits) {
  unsigned int y = 0;
  for (int i = 0; i < bits; i++) {
    y = (y << 1) | (x & 1);
    x >>= 1;
  }
  return y;
}

void fft(_Fcomplex *in, _Fcomplex *out, int n) {
  int bits = (int)log2f((float)n);

  for (int i = 0; i < n; i++) {
    unsigned int rev_idx = bit_reverse(i, bits);
    out[rev_idx] = in[i];
  }

  for (int len = 2; len <= n; len <<= 1) {
    float angle = -2.0f * M_PI / len;
    _Fcomplex wlen;
    wlen._Val[0] = cosf(angle);
    wlen._Val[1] = sinf(angle);

    for (int i = 0; i < n; i += len) {
      _Fcomplex w;
      w._Val[0] = 1.0f;
      w._Val[1] = 0.0f;

      for (int j = 0; j < len / 2; j++) {
        _Fcomplex u = out[i + j];
        _Fcomplex v = out[i + j + len / 2];

        _Fcomplex t;
        t._Val[0] = v._Val[0] * w._Val[0] - v._Val[1] * w._Val[1];
        t._Val[1] = v._Val[0] * w._Val[1] + v._Val[1] * w._Val[0];

        out[i + j]._Val[0] = u._Val[0] + t._Val[0];
        out[i + j]._Val[1] = u._Val[1] + t._Val[1];

        out[i + j + len / 2]._Val[0] = u._Val[0] - t._Val[0];
        out[i + j + len / 2]._Val[1] = u._Val[1] - t._Val[1];

        _Fcomplex next_w;
        next_w._Val[0] = w._Val[0] * wlen._Val[0] - w._Val[1] * wlen._Val[1];
        next_w._Val[1] = w._Val[0] * wlen._Val[1] + w._Val[1] * wlen._Val[0];
        w = next_w;
      }
    }
  }
}

ma_device_config device_config;

int fft_sample_count = 0;

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  if (pInput == NULL || frameCount == 0)
    return;

  const float *pInputFloat = (const float *)pInput;
  ma_uint32 channels = pDevice->capture.channels;
  int fft_size = current_fft_size;

  for (ma_uint32 i = 0; i < frameCount; i++) {
    float raw_sample = pInputFloat[i * channels];

    float window =
        0.5f * (1.0f - cosf((2.0f * M_PI * fft_sample_count) / (fft_size - 1)));

    fft_input[fft_sample_count]._Val[0] = raw_sample * window;
    fft_input[fft_sample_count]._Val[1] = 0.0f;

    fft_sample_count++;

    if (fft_sample_count >= fft_size) {
      fft_sample_count = 0;
    }
  }

  (void)pOutput;
}

int init_audio() {
  ma_result result;
  device_config = ma_device_config_init(ma_device_type_loopback);
  device_config.capture.format = ma_format_f32;
  device_config.capture.channels = 2;
  device_config.sampleRate = 48000;
  device_config.dataCallback = data_callback;
  result = ma_device_init(NULL, &device_config, &device);
  if (result != MA_SUCCESS)
    return -1;
  ma_device_start(&device);
  return 0;
}

float complex_abs(_Fcomplex c) {
  return sqrtf(c._Val[0] * c._Val[0] + c._Val[1] * c._Val[1]);
}

float smoothed_frequencies[MAX_FFT_SIZE] = {0};

void open_cfg_file() {
  char exePath[MAX_PATH];

  if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) {
    perror("Could not found path to executable");
    return;
  }

  char *lastSlash = strrchr(exePath, '\\');
  if (lastSlash != NULL)
    *(lastSlash + 1) = '\0';

  strcat(exePath, "config.cfg");

  cfg_file = fopen(exePath, "r");
}

void create_default_cfg_file() {
  if (cfg_file != NULL)
    return;
  char exePath[MAX_PATH];

  if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) {
    perror("Could not found path to executable");
    return;
  }

  char *lastSlash = strrchr(exePath, '\\');
  if (lastSlash != NULL)
    *(lastSlash + 1) = '\0';

  strcat(exePath, "config.cfg");

  FILE *file = fopen(exePath, "w");

  if (file == NULL)
    return;

  fprintf(file, "# Default config\n");
  fprintf(file, "fft_size=1024\n");
  fprintf(file, "bar_gap=0\n");
  fprintf(file, "bar_size=1\n");
  fprintf(file, "window_height=300\n");
  fprintf(file, "volume_multiplier=1.0\n");
  fprintf(file, "bass_boost_multiplier=4.0\n");
  fclose(file);
}

Vector2 drag_offset = {0};
bool is_dragging = false;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  CoInitialize(NULL);

  open_cfg_file();
  create_default_cfg_file();

  if (tray_start() < 0 || init_audio() < 0)
    return 1;
  config_window();

  int window_width = get_window_width();
  int window_height = get_window_height();

  int bar_size = get_bar_size();
  int bar_gap = get_bar_gap();

  open_window(window_width, window_height);

  int visual_bars = get_visual_bars();
  int fft_size = current_fft_size = get_fft_size();

  float volume_multiplier = get_volume_multiplier();
  float bass_boost_multiplier = get_bass_boost_multiplier();

  SetTargetFPS(60);

  while (tray_loop(0) == 0) {
    if (WindowShouldClose())
      close_window();

    fft(fft_input, fft_output, fft_size);

    for (int i = 0; i < visual_bars; i++) {
      float t = (float)i / (float)visual_bars;

      float max_fft_index = (float)(fft_size / 2 - 1);
      float log_index =
          (powf(2.0f, t * 11.0f) - 1.0f) * (max_fft_index / 2047.0f);

      int idx_low = (int)floorf(log_index);
      int idx_high = (int)ceilf(log_index);
      if (idx_low < 0)
        idx_low = 0;
      if (idx_high >= fft_size / 2)
        idx_high = (fft_size / 2) - 1;

      float frac = log_index - (float)idx_low;
      float amp_low = complex_abs(fft_output[idx_low]);
      float amp_high = complex_abs(fft_output[idx_high]);
      float raw_amplitude = amp_low + (amp_high - amp_low) * frac;

      float bass_boost = 1.0f + (1.0f - t) * bass_boost_multiplier;

      float amplitude = log10f(1.0f + raw_amplitude * 15.0f * bass_boost);

      smoothed_frequencies[i] =
          smoothed_frequencies[i] * 0.80f + amplitude * 0.20f;
    }

    if (!is_click_through_enabled) {
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        drag_offset = GetMousePosition();
        is_dragging = true;
      }

      if (is_dragging) {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          is_dragging = false;
        } else {
          Vector2 window_pos = GetWindowPosition();
          Vector2 current_mouse = GetMousePosition();

          int next_x = (int)(window_pos.x + current_mouse.x - drag_offset.x);
          int next_y = (int)(window_pos.y + current_mouse.y - drag_offset.y);

          SetWindowPosition(next_x, next_y);
        }
      }
    }
    if (!window_opened)
      continue;

    BeginDrawing();
    ClearBackground(BLANK);

    for (int i = 0; i < visual_bars; i++) {
      int bar_height =
          (int)(smoothed_frequencies[i] * window_height * volume_multiplier);

      if (bar_height > window_height)
        bar_height = window_height;

      int x_pos = i * (bar_size + bar_gap);
      int y_pos = window_height - bar_height;

      DrawRectangle(x_pos, y_pos, bar_size, bar_height, RED);
    }
    EndDrawing();
  }
  ma_device_uninit(&device);
  return 0;
}
