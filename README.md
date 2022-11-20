# WASM + WASI talk & demonstration codes

[![dependency status](https://deps.rs/repo/github/emilk/eframe_template/status.svg)](https://deps.rs/repo/github/emilk/eframe_template)
[![Build Status](https://github.com/emilk/eframe_template/workflows/CI/badge.svg)](https://github.com/emilk/eframe_template/actions?workflow=CI)

This is a repo containing the different demonstration codes used in the included talk on
WebAssembly (WASM) and the WebAssembly System Interface (WASI).

Instructions building and running each demonstration is given below.

## Demo 1: Simple Function

In the simple_wasm folder.

Build using:

```bash
clang --target=wasm32 --no-standard-libraries -Wl,--export-all -Wl,--no-entry -o add.wasm add.c -Oz
```
Run the code by serving the included index.html via:

```bash
python3 -m http.server
```

## Demo 2: emscripten

Run the pyodide app, in the pyodide folder, using:

```bash
python3 app.py
```

## Demo 3: Signal viewer

In the root directory of this repo.

Build and run the native app using:

```bash
cargo run --release
```

Build and run the WASM-based web app using:

```bash
trunk serve
```

## Demo 4: Image convolutions

In the convolution directory.

Build and run the native app using:

```bash
cmake -Bbuild -H. -GNinja
ninja -C build
./build/convolution
```

Build and run the single-threaded WASM app using:
```bash
em++ main.cpp -fwasm-exceptions --preload-file photo.bmp -sALLOW_MEMORY_GROWTH --post-js post.js
node a.out.js
```

Build and run the multithreaded WASM app using:

```bash
em++ main.cpp -fwasm-exceptions --preload-file photo.bmp -pthread -sPROXY_TO_PTHREAD -sALLOW_MEMORY_GROWTH -DTHREADS=1 --post-js post-threads.js
node a.out.js
```

Build and run the wasi-sdk app using:

```bash
${WASI_SDK}/bin/clang++ --sysroot=${WASI_SDK}/share/wasi-sysroot main.cpp -o main.wasm -fno-exceptions
wasmer main.wasm
```