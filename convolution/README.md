## wasi-sdk

```bash
/Users/jhollocombe/wasi-sdk/wasi-sdk-16.5ga0a342ac182c/bin/clang++ --sysroot=/Users/jhollocombe/wasi-sdk/wasi-sdk-16.5ga0a342ac182c/share/wasi-sysroot main.cpp -o main.wasm -fno-exceptions
wasmer run --dir . main.wasm
```

`em++ main.cpp -fwasm-exceptions --preload-file photo.bmp -pthread -sPROXY_TO_PTHREAD -sALLOW_MEMORY_GROWTH -DTHREADS=1 --post-js post-threads.js`

`em++ main.cpp -fwasm-exceptions --preload-file photo.bmp -sALLOW_MEMORY_GROWTH --post-js post.js`

`clang --target=wasm32 --no-standard-libraries -Wl,--export-all -Wl,--no-entry -o convolution.wasm main.cpp -Oz`