#Welcome!
This is **pngenc**: A small and *fast* png encoder. 

It is optimized for 64bit multicore machines such as x86_64 and ARM64 but compiles and runs on 32bit platforms as well.

###Status
Branch | Build Status
------------ | -------------
master | [![Build Status](https://travis-ci.org/GregorBudweiser/pngenc.svg?branch=master)](https://travis-ci.org/GregorBudweiser/pngenc)

###Typical Results
pngenc implements both uncompressed and compressed (Z_HUFF_ONLY) modes.
Runtimes for a FullHD image (1920x1080, 24bit) encoded on an Intel Core i7 860, 4C/8T (save to /dev/null):
* Uncompressed: ~5ms (libpng: ~12ms)
* Compressed: ~9ms (libpng: ~85ms)

###How to Build
To build pngenc just use cmake:

```
cd /to/pngenc
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

You can then run the example executable in /build/bin (see /source/example.c for details)
