# Examples

This directory contains minimal runnable examples for `fisiCs`.

## Files

- `hello_world.c`: basic stdout example
- `sdl_window_loop.c`: SDL window with event loop and animated clear color

## Build Both Examples

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
make examples
```

## Build and Run: Hello World

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
./fisics examples/hello_world.c -o build/examples/hello_world
./build/examples/hello_world
```

## Build and Run: SDL Window Loop

Requires SDL2 development libraries.

```bash
cd /Users/calebsv/Desktop/CodeWork/fisiCs
mkdir -p build/examples
./fisics -c examples/sdl_window_loop.c -o build/examples/sdl_window_loop.o
clang build/examples/sdl_window_loop.o -o build/examples/sdl_window_loop $(sdl2-config --cflags --libs)
./build/examples/sdl_window_loop
```

Press `Esc` or close the window to exit.

## Notes

- The SDL example uses clang for final linking in this README to keep environment setup straightforward.
- If your local `fisics` link flow is configured for SDL2, you can also link directly with `fisics`.
