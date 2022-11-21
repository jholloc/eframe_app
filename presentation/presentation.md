---
theme: white
highlightTheme: monokai
slideNumber: true
transition: convex
width: 1024
height: 800
bg: lightcyan
enableOverview: true
---

<!-- slide bg="[[webassembly-wasm.jpeg]]" data-background-opacity="0.2" -->


## Running compiled code anywhere with Web Assembly and WASI

### CDG meeting 21<sup>st</sup> November

Jonathan Hollocombe

jonathan.hollocombe@ukaea.uk


---

## What is Web Assembly?

WebAssembly (aka WASM) is a binary instruction format similar to regular assembly (i.e. x86) but targetting the web.

WebAssembly is an open standard, created with the following goals in mind:

- Be executed at near-native speeds.
- Be readable and debuggable.
- Be secure.
- Interact with the rest of the web ecosystem.

---

## What can it do?

- Compile high-level languages (C, C++, Rust, etc.) to run in the browser
- Run high-performance and high-resource applications on the web (games, image editing, visualisation, etc.)
- Render applications with WebGL without interacting with the DOM
- Run compiled server-side applications with node.js
- Enable sandboxed portable applications using WASI

---

## What does it look like?

WebAssembly comes in 2 formats:

- .wasm: the binary assembly which is compiled from the high-level language such as C, C++, Rust, etc. and sent to the browser
- .wat: the human readable representation of WASM that is diplayed by the browser when debugging

---

## Example

add.c
```c
int add(int first, int second)
{
    return first + second;
}
```

add.wat
```text
(module
  ;; some global exports not shown here
  (func $add (;1;) (export "add") (param $var0 i32) (param $var1 i32) (result i32)
    local.get $var0
    local.get $var1
    i32.add
  )
)
```


---

##  How to generate it?

- WebAssembly can be used with any LLVM based compiler by specifying the `wasm32` target
- For simple code (such as the previous demo) this can be done simply:
  ```bash
  clang --target=wasm32 file.c -o file.wasm
  ```
- For most codes you'll need access to the standard libraries (`libc`, etc.) so will need to use `emscripten`
  ```bash
  emcc file.c
  ```

---

## How it runs?

- WebAssembly requires a stack-based virtual machine to run
- This VM runs the compiled WASM module, providing an ArrayBuffer or SharedArrayBuffer that provide the memory space used, and interfaces to I/O, threads, exceptions, etc.

---

## How to run it?

- WebAssembly can be run using:
    - A browser with JavaScript glue code to instatiate the WASM module
    - node.js with JavaScript glue code to instatiate the WASM module
    - A WebAssembly runtime such as `wasmtime` or `wasmer`

---

## Demo 1: Simple Function

```c
int add(int first, int second)
{
    return first + second;
}
```

```bash
clang --target=wasm32 --no-standard-libraries -Wl,--export-all -Wl,--no-entry -o add.wasm add.c -Oz
```

```html
<!DOCTYPE html>
<html>
  <head></head>
  <body>
    <script type="module">
      (async() => {
        const response = await fetch('add.wasm');
        const bytes = await response.arrayBuffer();
        const { instance } = await WebAssembly.instantiate(bytes);
         window.alert('The answer is: ' + instance.exports.add(1, 2));
      })();
    </script>
  </body>
</html>
```

note:
- in simple_wasm folder of eframe_app
- compile with `/usr/local/Cellar/llvm/15.0.2/bin/clang --target=wasm32 --no-standard-libraries -Wl,--export-all -Wl,--no-entry -o add.wasm add.c -Oz`
- run `python3 -m http.server`
- open `http://localhost:8000 in browser`
- show debugger - sources - reload - show wasm

---

## Demo 2: emscripten

- Example of porting codes to WASM:
  https://github.com/emscripten-core/emscripten/wiki/Porting-Examples-and-Demos
- Pyodide: emscripten based Python
- Run demo using:
  ```bash
  python3 app.py
  ```

---

## Rust to WASM

- Rust is a popular choice for high-level WebAssembly language
    - No runtime, no exceptions, small standard library
- The Rust ecosystem has widely adopted WebAssembly - many libraries support wasm32 target
- This tends to make compiling from Rust easier than C or C++

---

## Demo 3: Signal viewer

- A very rough signal viewer application
    - Talks to a REST endpoint
    - Displays available signals
    - Plots 2D data where available
- Compiled as a desktop application:
  ```bash
  cargo run
  ```
- Compiled as a browser application:
  ```bash
  trunk serve
  ```

notes:
- make sure eframe_app_server is running:
  `FLASK_DEBUG=ON flask --app app run`

---

## What is WASI?

- The WebAssembly System Interface (aka WASI) is an API designed to provide OS-like features such as filesystem, sockets, clocks, etc.
- Browser independent and not dependent on Web APIs or JavaScript
- Runable in any WASI enabled WASM runtime such as `wasmtime` and `wasmer`
- Portable to any OS - WASI abstracts the OS from the WASM compiled code

---

## Demo 4: Image convolutions

This demo code is a simple image convolution tool that opens a file, runs a convolution over the data, and saves the file.

Native app:
```bash
cmake -Bbuild -H. -GNinja
ninja -C build
./build/convolution
```

--

## Demo 4: Image convolutions (part 2)

WASM:
```bash
em++ main.cpp -fwasm-exceptions --preload-file photo.bmp -sALLOW_MEMORY_GROWTH --post-js post.js
node a.out.js
```

WASM+WASI:
```bash
${WASI_SDK}/bin/clang++ --sysroot=${WASI_SDK}/share/wasi-sysroot main.cpp -o main.wasm -fno-exceptions
```

---

## WASI-SDK limitations

- No exceptions
- No threads - this is work in progress
- Had issues processing very large file
- I've haven't tried Rust WASI yet so need to try this to see if more is currently possible

---

## Conclusions

- WebAssembly is way for leveraging the power of compiled high-level languages to write tools that can run on the Web
- A lot of the tooling around WebAssembly in the browser is fairly mature, less so with running outside of the browser
- Rust compilation for WASM is easier than for other compiled languages

--

## Conclusions (cont)

- WASI is a fairly recent addition and still in development - the tooling and features need to be expanded before it is truly viable
- WASM+WASI does have the potential for compiling sandboxed, portable code that can be run anywhere

---

## Links

Tools:
- https://webassembly.org/
- https://wasi.dev/
- https://wasmer.io/
- https://emscripten.org/


--

## Links (cont)

Useful articles:
- https://thenewstack.io/what-is-webassembly-and-why-do-you-need-it
- https://scientificprogrammer.net/2019/08/18/what-the-heck-is-webassembly

Demonstration code:
- https://github.com/jholloc/wasm_wasi_talk

---

![[quote.png]]
