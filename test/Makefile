CXXTESTGEN = cxxtestgen
CXX        = g++
INCLUDE    = ../include
CXXFLAGS   = -I$(INCLUDE) #-std=c++11
OPT        = -O3
DEBUG      = -O0 -g

HEADERS = $(wildcard $(INCLUDE)/tachy_*.h) $(INCLUDE)/tachy.h

TESTS = $(addprefix tachy_test_,base sse sse2 avx avx2 fma fmavx2)

all: $(TESTS)

test_base: tachy_test_base tachy_test_base.debug

tachy_test_base: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -mno-sse2 $<

tachy_test_base.debug: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(DEBUG) -mno-sse2 $<

test_sse: tachy_test_sse tachy_test_sse.debug

tachy_test_sse: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -msse -mno-sse2 $<

tachy_test_sse.debug: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(DEBUG) -msse -mno-sse2 $<

test_sse2: tachy_test_sse2 tachy_test_sse2.debug

tachy_test_sse2: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -msse2 $<

tachy_test_sse2.debug: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(DEBUG) -msse2 $<

test_avx: tachy_test_avx tachy_test_avx.debug

tachy_test_avx: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -mavx $<

tachy_test_avx.debug: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(DEBUG) -mavx $<

test_avx2: tachy_test_avx2 tachy_test_avx2.debug

tachy_test_avx2: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -mavx2 $<

tachy_test_avx2.debug: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(DEBUG) -mavx2 $<

test_fma: tachy_test_fma tachy_test_fma.debug

tachy_test_fma: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -mfma $<

tachy_test_fma.debug: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(DEBUG) -mfma $<

test_fmavx2: tachy_test_fmavx2 tachy_test_fmavx2.debug

tachy_test_fmavx2: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -mavx2 -mfma $<

tachy_test_fmavx2.debug: test_suite.cpp tachy.t.h $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(DEBUG) -mavx2 -mfma $<

test_suite.cpp: tachy.t.h
	$(CXXTESTGEN) --error-printer -o $@ $<

test: $(TESTS)
	-@echo base   && tachy_test_base
	-@echo sse    && tachy_test_sse
	-@echo sse2   && tachy_test_sse2
	-@echo avx    && tachy_test_avx
	-@echo avx2   && tachy_test_avx2
	-@echo fma    && tachy_test_fma
	-@echo fmavx2 && tachy_test_fmavx2

EXAMPLE=$(addprefix example_,base sse2 avx avx2 fma fmavx2)

example: $(EXAMPLE)

example_fmavx2: example.cpp $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -mfma -mavx2 $<

example_fma: example.cpp $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -mfma $<

example_avx2: example.cpp $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -mavx2 $<

example_avx: example.cpp $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -mavx $<

example_sse2: example.cpp $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -msse2 $<

example_base: example.cpp $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -mno-sse2 -mno-avx -mno-avx2 -mno-fma $<

.PHONY: example

clean:
	rm -f $(TESTS) $(addsuffix .debug, $(TESTS)) test_suite.cpp $(EXAMPLE)
