CXXTESTGEN = cxxtestgen
CXX        = g++
INCLUDE    = ../include
CXXFLAGS   = -I$(INCLUDE)
OPT        = -O3
DEBUG      = -O0 -g

HEADERS = $(INCLUDE)/tachy_aligned_allocator.h \
	  $(INCLUDE)/tachy_arch_traits.h \
	  $(INCLUDE)/tachy_cacheable.h \
	  $(INCLUDE)/tachy_calc_cache.h \
	  $(INCLUDE)/tachy_exception.h \
	  $(INCLUDE)/tachy_expression.h \
	  $(INCLUDE)/tachy_functor.h \
	  $(INCLUDE)/tachy_iota_engine.h \
	  $(INCLUDE)/tachy_lagged_engine.h \
	  $(INCLUDE)/tachy_lagged_vector.h \
	  $(INCLUDE)/tachy_linear_spline_incr_slope.h \
	  $(INCLUDE)/tachy_linear_spline_uniform.h \
	  $(INCLUDE)/tachy_random_engine.h \
	  $(INCLUDE)/tachy_scalar.h \
	  $(INCLUDE)/tachy_spline_util.h \
	  $(INCLUDE)/tachy_static_functor_engine.h \
	  $(INCLUDE)/tachy_time_shift.h \
	  $(INCLUDE)/tachy_util.h \
	  $(INCLUDE)/tachy_vector_engine.h \
	  $(INCLUDE)/tachy_vector.h

TESTS = $(addprefix tachy_,test_base test_sse test_sse2 test_avx)

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

test_suite.cpp: tachy.t.h
	$(CXXTESTGEN) --error-printer -o $@ $<

test: $(TESTS)
	./tachy_test_base && ./tachy_test_sse && ./tachy_test_sse2 && ./tachy_test_avx

example: example_avx example_sse2 example_base # example_avx2

#example_avx2: example.cpp $(HEADERS)
#	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -mavx2 $<

example_avx: example.cpp $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -mavx $<

example_sse2: example.cpp $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -msse2 $<

example_base: example.cpp $(HEADERS)
	$(CXX) -o $@ $(CXXFLAGS) $(OPT) -mno-sse2 $<

clean:
	rm -f $(TESTS) $(addsuffix .debug, $(TESTS)) test_suite.cpp example_{avx2,avx,sse2,base}
