# tachy
Expression templates library with multi-level caching of common sub-expressions (useful when the situation requires nested inner loops and there is significant portion of calculations that are constant in the innermost loop - e.g. in econometric prepayment models). Strictly vectors, no matrix algebra here (at least yet). If you're doing matrices - check out blaze (https://bitbucket.org/blaze-lib/blaze). Uses intel/amd sse/sse2/avx simd instructions (no dynamic dispatch though - it must be compiled for target set). avx2/fma is in the works

test/example.cpp shows the intended use (implementation of a mock prepayment model)

tested:
g++ 4.8

requires:
cxxtest (http://cxxtest.com) for testing


