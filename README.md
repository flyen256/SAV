# SAV

SAV - ScreenAudioVisualizer is open-source customizable device audio visualizer build with C, raylib, tray and miniaudio.

# Preview
![preview](https://github.com/user-attachments/assets/d5d54eda-6040-4a6c-91a5-1c1290ba38ca)

# Configuration

```cfg
# When higher than 1536 bar_size should be 1, fft_size should preferably be divisible by 2 (max value is 2048)
fft_size=256
# Should be zero when fft_size is very high (like 1536 or 2048)
bar_gap=0
bar_size=5
window_height=300
# All frequencies multiplier
volume_multiplier=0.35
# Multiplier of low frequencies
bass_boost_multiplier=3.0
example_color=red,green,blue,alpha
# Bottom color of gradient
bottom_color=255,255,255,255
# Top color of gradient
top_color=173,106,255,255
```

# License

SAV provided with [MIT License](https://choosealicense.com/licenses/mit/).

### Dependencies

- [raylib](https://github.com/raysan5/raylib) - [Zlib license](https://zlib.net/zlib_license.html)
- [miniaudio](https://github.com/mackron/miniaudio) - Public Domain
- [tray](https://github.com/zserge/tray) - [MIT License](https://choosealicense.com/licenses/mit/)
