#include "../include/miniaudio.h"
#include "../include/raylib.h"
#include <stdio.h>
#define Rectangle CombaseApiRectangle
#define CloseWindow CombaseApiCloseWindow
#define ShowCursor CombaseApiShowCursor
#include <combaseapi.h>
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#include <complex.h>
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

void config_window() {
  SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_UNDECORATED);
}

void open_window() { InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Visualizer"); }
void close_window() { CloseWindow(); }

void tray_close(struct tray_menu *menu) { tray_exit(); }

void tray_open_window(struct tray_menu *menu) { open_window(); }

struct tray tray = {
    .menu = (struct tray_menu[]){{"Open", 0, 0, tray_open_window, NULL},
                                 {"Quit", 0, 0, tray_close, NULL},
                                 {NULL, 0, 0, NULL, NULL}}};

int tray_start() { return tray_init(&tray); }

_Fcomplex fft_input[FFT_SIZE];
_Fcomplex fft_output[FFT_SIZE];
float frequencies[FFT_SIZE / 2];

unsigned int bit_reverse(unsigned int x, int bits) {
  unsigned int y = 0;
  for (int i = 0; i < bits; i++) {
    y = (y << 1) | (x & 1);
    x >>= 1;
  }
  return y;
}

void fft(_Fcomplex *in, _Fcomplex *out, int n, int step) {
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
ma_device device;

int fft_sample_count = 0;

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  if (pInput == NULL || frameCount == 0)
    return;

  const float *pInputFloat = (const float *)pInput;
  ma_uint32 channels = pDevice->capture.channels;

  for (ma_uint32 i = 0; i < frameCount; i++) {
    float raw_sample = pInputFloat[i * channels];

    float window =
        0.5f * (1.0f - cosf((2.0f * M_PI * fft_sample_count) / (FFT_SIZE - 1)));

    fft_input[fft_sample_count]._Val[0] = raw_sample * window;
    fft_input[fft_sample_count]._Val[1] = 0.0f;

    fft_sample_count++;

    if (fft_sample_count >= FFT_SIZE) {
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

float smoothed_frequencies[FFT_SIZE / 2] = {0};

FILE *cfg_file;

void open_cfg_file() {}

Vector2 drag_offset = {0};
bool is_dragging = false;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  CoInitialize(NULL);
  if (tray_start() < 0 || init_audio() < 0)
    return 1;
  config_window();
  open_window();

  SetTargetFPS(60);

  while (tray_loop(0) == 0) {
    if (WindowShouldClose())
      close_window();

    fft(fft_input, fft_output, FFT_SIZE, 1);

    for (int i = 0; i < VISUAL_BARS; i++) {
      float t = (float)i / (float)VISUAL_BARS;

      float log_index =
          (powf(2.0f, t * 11.0f) - 1.0f) * ((FFT_SIZE / 2) / 2047.0f);

      int idx_low = (int)floorf(log_index);
      int idx_high = (int)ceilf(log_index);
      if (idx_low < 0)
        idx_low = 0;
      if (idx_high >= FFT_SIZE / 2)
        idx_high = (FFT_SIZE / 2) - 1;

      float frac = log_index - (float)idx_low;
      float amp_low = complex_abs(fft_output[idx_low]);
      float amp_high = complex_abs(fft_output[idx_high]);
      float raw_amplitude = amp_low + (amp_high - amp_low) * frac;

      float bass_boost = 1.0f + (1.0f - t) * 10.0f;

      float amplitude = log10f(1.0f + raw_amplitude * 15.0f * bass_boost);

      smoothed_frequencies[i] =
          smoothed_frequencies[i] * 0.80f + amplitude * 0.20f;
    }

    for (int i = 0; i < VISUAL_BARS; i++) {
      float raw_amplitude = complex_abs(fft_output[i]);
      float amplitude = log10f(1.0f + raw_amplitude * 15.0f);
      smoothed_frequencies[i] =
          smoothed_frequencies[i] * 0.82f + amplitude * 0.18f;
    }

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

    BeginDrawing();
    ClearBackground(BLANK);

    for (int i = 0; i < VISUAL_BARS; i++) {
      int bar_height = (int)(smoothed_frequencies[i] * WINDOW_HEIGHT * 0.25f);

      if (bar_height > WINDOW_HEIGHT)
        bar_height = WINDOW_HEIGHT;

      int x_pos = i * (BAR_SIZE + BAR_GAP);
      int y_pos = WINDOW_HEIGHT - bar_height;

      DrawRectangle(x_pos, y_pos, BAR_SIZE, bar_height, RED);
    }
    EndDrawing();
  }
  ma_device_uninit(&device);
  return 0;
}
