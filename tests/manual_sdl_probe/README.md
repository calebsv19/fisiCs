# Manual SDL Probe (Isolated)

These probes are intentionally minimal and isolated from the rest of your app.

## 1) Headless color check

Compile:

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
./fisics tests/manual_sdl_probe/sdl_headless_color_check.c $(pkg-config --cflags --libs sdl2) -o /tmp/sdl_headless_probe.out
```

Run:

```bash
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy /tmp/sdl_headless_probe.out
```

Expected output:

`HEADLESS_OK color=(12,34,56,255)`

## 2) Visible window color check

Compile:

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
./fisics tests/manual_sdl_probe/sdl_visible_color_window.c $(pkg-config --cflags --libs sdl2) -o /tmp/sdl_visible_probe.out
```

Run:

```bash
/tmp/sdl_visible_probe.out
```

Expected:

- A visible 640x360 window appears (default 5 seconds).
- Program prints a `VISIBLE_INFO ...` line with driver/size/flags.
- Program prints `VISIBLE_OK color=(20,80,220,255)`.
- You can change display time: `FISICS_SDL_VISIBLE_MS=10000 /tmp/sdl_visible_probe.out`.

## 3) Visible color-cycle window check

Compile:

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
./fisics tests/manual_sdl_probe/sdl_visible_color_cycle_window.c $(pkg-config --cflags --libs sdl2) -o /tmp/sdl_visible_cycle_probe.out
```

Run:

```bash
/tmp/sdl_visible_cycle_probe.out
```

Expected:

- A visible window cycles through multiple full-screen colors for ~7 seconds.
- Program prints `VISIBLE_CYCLE_INFO ...` and then `VISIBLE_CYCLE_OK ...`.
- You can change cycle duration: `FISICS_SDL_VISIBLE_CYCLE_MS=12000 /tmp/sdl_visible_cycle_probe.out`.

## 4) Headless event-routing + state-machine probe (deterministic)

Compile:

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
./fisics tests/manual_sdl_probe/sdl_headless_event_route_probe.c $(pkg-config --cflags --libs sdl2) -o /tmp/sdl_headless_route_probe.out
```

Run:

```bash
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy /tmp/sdl_headless_route_probe.out
```

Expected:

- Program injects synthetic key/mouse/window/user events into SDL queue.
- Routing state is validated against exact expected values.
- Program prints `ROUTE_OK ... color=(...)`.

## 5) Visible interactive input-routing probe

Compile:

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
./fisics tests/manual_sdl_probe/sdl_visible_input_route_probe.c $(pkg-config --cflags --libs sdl2) -o /tmp/sdl_visible_input_probe.out
```

Run:

```bash
/tmp/sdl_visible_input_probe.out
```

Expected:

- Window appears and runs for up to ~12 seconds.
- Move mouse, click, type, and/or press keys (ESC quits early).
- Program prints `INPUT_OK ...` summary with event counts and sampled center color.
- If no interaction occurs, it fails closed (unless `FISICS_SDL_REQUIRE_INPUT=0`).

Useful knobs:

- `FISICS_SDL_INPUT_MAX_MS=20000 /tmp/sdl_visible_input_probe.out`
- `FISICS_SDL_REQUIRE_INPUT=0 /tmp/sdl_visible_input_probe.out`

## 6) Visible visual-FX playground probe (new functionality)

This probe is intentionally different from the existing color/input checks. It exercises:

- dense multi-lane animated visual field (many draw calls/frame)
- moving box swarm with theme/speed mode changes
- live minimap overlay with scaled world projection
- interactive mouse-influenced distortion

Compile:

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
./fisics tests/manual_sdl_probe/sdl_visible_visual_fx_playground.c $(pkg-config --cflags --libs sdl2) -o /tmp/sdl_visual_fx_probe.out
```

Run:

```bash
/tmp/sdl_visual_fx_probe.out
```

Controls:

- `SPACE`: pause/resume simulation
- `TAB`: cycle color theme
- `S`: cycle speed mode
- hold left mouse button: enable mouse influence
- `ESC`: quit early

Expected:

- A 900x540 animated window with moving boxes and a top-right minimap.
- Terminal prints `VISUAL_FX_INFO ...` then `VISUAL_FX_OK ...`.

Knob:

- `FISICS_SDL_VISUAL_FX_MS=20000 /tmp/sdl_visual_fx_probe.out`

If macOS loader cannot find SDL:

```bash
DYLD_LIBRARY_PATH=/opt/homebrew/lib /tmp/sdl_visible_probe.out
```
