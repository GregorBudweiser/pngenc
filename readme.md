# Welcome!

This is **pngenc**: A small and *fast* png encoder written in C. 

It is optimized for 64bit multicore machines such as x86_64 and depends on libc and libgomp.

### Status

Branch | Build Status
------------ | -------------
master | [![Build Status](https://travis-ci.org/GregorBudweiser/pngenc.svg?branch=master)](https://travis-ci.org/GregorBudweiser/pngenc)

### Typical Results

pngenc implements both uncompressed and compressed (Z_HUFFMAN_ONLY) modes.

Runtimes for a FullHD image (1920x1080, 24bit) encoded on an **Intel Core i7 860**, 4C/8T (save to /dev/null):
* Uncompressed: ~5ms (libpng: ~12ms)
* Compressed: ~9ms (libpng: ~85ms)

Runtimes for a FullHD image (1920x1080, 24bit) encoded on an **AMD Ryzen 7 3700X**, 8C/16T (save to /dev/null):
* Uncompressed: ~1.7ms (libpng: ~6.3ms)
* Compressed: ~2.7ms (libpng: ~70ms)

### How to Build

Building pngenc requires a C compiler and cmake:

```
cd /to/pngenc
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

You can then run the example executable in /build/bin (see /source/example.c for details)
