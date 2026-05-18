#include "../include/raylib.h"
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

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define ARRAY_LEN(xs) (sizeof(xs) / sizeof((xs)[0]))
#define FFT_SIZE 1024

#define TRAY_WINAPI 1

#define Rectangle WinapiRectangle
#define CloseWindow WinapiCloseWindow
#define ShowCursor WinapiShowCursor

#include "../include/tray.h"

#undef Rectangle
#undef CloseWindow
#undef ShowCursor

void config_window() {
  SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_UNDECORATED);
}

void open_window() { InitWindow(400, 200, "Visualizer"); }
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

void fft(_Fcomplex *in, _Fcomplex *out, int n, int step) {
  if (n == 1) {
    out[0] = in[0];
    return;
  }
  fft(in, out, n / 2, step * 2);
  fft(in + step, out + n / 2, n / 2, step * 2);

  for (int i = 0; i < n / 2; i++) {
    float angle = -2.0f * M_PI * i / n;

    float cos_a = cosf(angle);
    float sin_a = sinf(angle);

    _Fcomplex target = out[i + n / 2];

    _Fcomplex t;
    t._Val[0] = cos_a * target._Val[0] - sin_a * target._Val[1];
    t._Val[1] = cos_a * target._Val[1] + sin_a * target._Val[0];

    _Fcomplex e = out[i];

    out[i]._Val[0] = e._Val[0] + t._Val[0];
    out[i]._Val[1] = e._Val[1] + t._Val[1];

    out[i + n / 2]._Val[0] = e._Val[0] - t._Val[0];
    out[i + n / 2]._Val[1] = e._Val[1] - t._Val[1];
  }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  CoInitialize(NULL);
  if (tray_start() < 0)
    return 1;
  config_window();
  open_window();
  while (tray_loop(1) == 0) {
    if (WindowShouldClose())
      close_window();
    BeginDrawing();
    ClearBackground(BLANK);

    EndDrawing();
  }
  close_window();
  return 0;
}
