# Welcome!

### TL;DR
This is **pngenc**: A *fast png encoder* written in C

### Technical stuff
**pngenc** is optimized for 64bit multi-core processors such as x86_64 and depends on libc and OpenMP. It is tested with gcc/mingw, clang and msvc.
The following compression modes are supported: Uncompressed, "Huffman Only" and Run-length encoding. The achieved compression rates are the same as libpng/zlib but up to an order of magnitude faster (with the option to add multi-threading on top of that).

### Typical Results
Runtimes for encoding a 1920x1080x24bit image on an AMD Ryzen 7 3700X:

| Mode         | libpng(1.6) [ms] | pngenc (1 thread) [ms] | pngenc (multi-threaded) [ms] |
|--------------|-----------------:|-----------------------:|-----------------------------:|
| Uncompressed | 9                | 0.8                    | 0.2                          |
| Huffman Only | 82               | 7.3                    | 1.2                          |
| RLE          | 84               | 30                     | 4                            |
| Full LZ77    | ~150             | N/A                    | N/A                          | 

What is happening? 
- zlib's RLE and HUFF_ONLY modes share most of their implementation with the "full compression" mode which is suboptimal. (I still think this is a valid tradeoff!)
- As far as I can tell zlib does not use SIMD extensions on x86 (but on arm!)
- Both zlib and libpng don't provide multi-threading. Sure, each thread could compress one image but this requires more memory than most L3 caches in desktops/laptops can provide.

### About
This is pretty much a hobby-project for learning purposes. Most notably I learned to appreciate how fast computers are these days. Or perhaps how fast they could be :)
