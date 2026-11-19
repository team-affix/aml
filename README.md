# AML

Abstractive machine learning: learns models of noiseless, functional data that fit every data point exactly and
are minimal in symbolic description length.

## Build

```
git submodule update --init --recursive
make core_debug_fast && ./build/core_debug_fast
```

`make core` builds the release library `build/libaml_core.a`.
