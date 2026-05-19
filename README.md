# SAV

SAV - ScreenAudioVisualizer is open-source customizable device audio visualizer build with C, raylib, tray and miniaudio.

# Preview

<img width="508" height="178" alt="preview" src="https://github.com/user-attachments/assets/fa57c212-ae74-4db1-8d9d-1e91080a8c09" />

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
bass_boost_multiplier=1.0
example_color=red,green,blue,alpha
# Bottom color of gradient
bottom_color=255,255,255,255
# Top color of gradient
top_color=173,106,255,255
# Low frequencies width controller
bass_expansion=7.5
```

# License

SAV provided with [MIT License](https://choosealicense.com/licenses/mit/).

### Dependencies

- [raylib](https://github.com/raysan5/raylib) - [Zlib license](https://zlib.net/zlib_license.html)
- [miniaudio](https://github.com/mackron/miniaudio) - Public Domain
- [tray](https://github.com/zserge/tray) - [MIT License](https://choosealicense.com/licenses/mit/)
