# tachy
Expression templates library with multi-level caching of common sub-expressions (useful when the situation requires nested inner loops and there is significant portion of calculations that are constant in the innermost loop - e.g. in econometric prepayment models). It's strictly vectors, no matrix algebra here (at least not yet. If you're doing matrices - check out blaze at https://bitbucket.org/blaze-lib/blaze).

Tachy uses intel/amd sse/sse2/avx/avx2/fma simd instructions (but no dynamic dispatch though - it must be compiled for the target set).

test/example.cpp shows the intended use (implementation of a mock prepayment model)

tested:
g++ 4.8, 4.9, 9.3 on linux

(clang 10.0 crashes with an internal error)

requires:
cxxtest (http://cxxtest.com) for testing


