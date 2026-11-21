# ==============================================================================
# Toolchain
# ==============================================================================

MAKEFLAGS += -j$(shell nproc)

CXX      = g++
DEPFLAGS = -MMD -MP
CXXFLAGS = -std=c++20 -I. -Icore/hpp -Imcts/core/hpp $(DEPFLAGS)
AR       = ar
ARFLAGS  = rcs

DEBUG_CXXFLAGS      = -DDEBUG -g
DEBUG_FAST_CXXFLAGS = -DDEBUG -g -O3
RELEASE_CXXFLAGS    = -O3

# ==============================================================================
# Paths
# ==============================================================================

GTEST_INC     = googletest/googletest/include
GMOCK_INC     = googletest/googlemock/include
GTEST_ALL_CC  = googletest/googletest/src/gtest-all.cc
GMOCK_ALL_CC  = googletest/googlemock/src/gmock-all.cc
GTEST_CPPFLAGS = \
    -I$(GTEST_INC) -I$(GMOCK_INC) \
    -Igoogletest/googletest -Igoogletest/googlemock

# ==============================================================================
# Output names  (all under build/)
# ==============================================================================

CORE_LIB            = build/libaml_core.a
CORE_DEBUG_LIB      = build/libaml_core_debug.a
CORE_DEBUG_FAST_LIB = build/libaml_core_debug_fast.a

CORE_DEBUG_BIN      = build/core_debug
CORE_DEBUG_FAST_BIN = build/core_debug_fast

# The probe (probe/): a developer's entry point outside the core library, built against the release library.
PROBE_BIN = build/probe

# Research proofs (research/<name>/): each the exact recorded run, with its settings fixed in its source, built as
# make research/<name> into build/research/<name>.
KESTREL_PARITY_BIN = build/research/kestrel_parity
SWIFT_PARITY_BIN   = build/research/swift_parity
SWIFT_SCALED_SQUARE_OF_SUM_BIN = build/research/swift_scaled_square_of_sum

# ==============================================================================
# Object lists  (object files live in build/obj/<variant>/)
# ==============================================================================

# Core production: only the non-templated types have definitions to compile;
# everything templated stays header-only in core/hpp. The mcts submodule's
# non-templated sources are compiled into the core library.
CORE_SRC = $(shell find core/cpp -name '*.cpp' | sort)
MCTS_SRC = $(shell find mcts/core/cpp -name '*.cpp' | sort)

CORE_OBJ            = $(patsubst core/cpp/%.cpp, build/obj/core/%.o,                 $(CORE_SRC)) \
                      $(patsubst mcts/core/cpp/%.cpp, build/obj/core/mcts/%.o,            $(MCTS_SRC))
CORE_DEBUG_OBJ      = $(patsubst core/cpp/%.cpp, build/obj/core_debug/%.o,           $(CORE_SRC)) \
                      $(patsubst mcts/core/cpp/%.cpp, build/obj/core_debug/mcts/%.o,      $(MCTS_SRC))
CORE_DEBUG_FAST_OBJ = $(patsubst core/cpp/%.cpp, build/obj/core_debug_fast/%.o,      $(CORE_SRC)) \
                      $(patsubst mcts/core/cpp/%.cpp, build/obj/core_debug_fast/mcts/%.o, $(MCTS_SRC))

# Core tests: same layout under build/obj/core_{debug,debug_fast}_test/.
CORE_TEST_SRC = $(shell find core/test -name '*.cpp' | sort)

CORE_DEBUG_TEST_OBJ = \
    $(patsubst core/test/%.cpp, build/obj/core_debug_test/%.o, $(CORE_TEST_SRC))
CORE_DEBUG_FAST_TEST_OBJ = \
    $(patsubst core/test/%.cpp, build/obj/core_debug_fast_test/%.o, $(CORE_TEST_SRC))

CORE_DEBUG_GTEST_OBJ = \
    build/obj/core_debug_test/gtest-all.o \
    build/obj/core_debug_test/gmock-all.o
CORE_DEBUG_FAST_GTEST_OBJ = \
    build/obj/core_debug_fast_test/gtest-all.o \
    build/obj/core_debug_fast_test/gmock-all.o

CORE_DEBUG_BIN_OBJ      = $(CORE_DEBUG_TEST_OBJ) $(CORE_DEBUG_GTEST_OBJ)
CORE_DEBUG_FAST_BIN_OBJ = $(CORE_DEBUG_FAST_TEST_OBJ) $(CORE_DEBUG_FAST_GTEST_OBJ)

CORE_DEBUG_TEST_CXXFLAGS      = $(DEBUG_CXXFLAGS) $(GTEST_CPPFLAGS) -Icore/test
CORE_DEBUG_FAST_TEST_CXXFLAGS = $(DEBUG_FAST_CXXFLAGS) $(GTEST_CPPFLAGS) -Icore/test

# Compiler-written header deps (one .d per .o); empty until the first compile.
CORE_DEP                  = $(CORE_OBJ:.o=.d)
CORE_DEBUG_DEP            = $(CORE_DEBUG_OBJ:.o=.d)
CORE_DEBUG_FAST_DEP       = $(CORE_DEBUG_FAST_OBJ:.o=.d)
CORE_DEBUG_TEST_DEP       = $(CORE_DEBUG_TEST_OBJ:.o=.d)
CORE_DEBUG_FAST_TEST_DEP  = $(CORE_DEBUG_FAST_TEST_OBJ:.o=.d)
CORE_DEBUG_GTEST_DEP      = $(CORE_DEBUG_GTEST_OBJ:.o=.d)
CORE_DEBUG_FAST_GTEST_DEP = $(CORE_DEBUG_FAST_GTEST_OBJ:.o=.d)

# ==============================================================================
# User-facing targets
# ==============================================================================

.PHONY: all core core_debug core_debug_fast test test_fast probe clean \
        research/kestrel_parity research/swift_parity research/swift_scaled_square_of_sum

all: core core_debug core_debug_fast

core: $(CORE_LIB)

core_debug: $(CORE_DEBUG_BIN)

core_debug_fast: $(CORE_DEBUG_FAST_BIN)

test: core_debug
	$(CORE_DEBUG_BIN)

test_fast: core_debug_fast
	$(CORE_DEBUG_FAST_BIN)

probe: $(PROBE_BIN)

research/kestrel_parity: $(KESTREL_PARITY_BIN)

research/swift_parity: $(SWIFT_PARITY_BIN)

research/swift_scaled_square_of_sum: $(SWIFT_SCALED_SQUARE_OF_SUM_BIN)

clean:
	rm -rf build

# ==============================================================================
# Library archive rules
# ==============================================================================

$(CORE_LIB): $(CORE_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(CORE_DEBUG_LIB): $(CORE_DEBUG_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(CORE_DEBUG_FAST_LIB): $(CORE_DEBUG_FAST_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

# ==============================================================================
# Binary link rules
# ==============================================================================

$(CORE_DEBUG_BIN): $(CORE_DEBUG_LIB) $(CORE_DEBUG_BIN_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(CORE_DEBUG_BIN_OBJ) \
	    -Lbuild -laml_core_debug -lpthread

$(CORE_DEBUG_FAST_BIN): $(CORE_DEBUG_FAST_LIB) $(CORE_DEBUG_FAST_BIN_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(CORE_DEBUG_FAST_BIN_OBJ) \
	    -Lbuild -laml_core_debug_fast -lpthread

PROBE_SRC = $(shell find probe core/test/support -name '*.cpp' | sort)

$(PROBE_BIN): $(PROBE_SRC) $(CORE_LIB) | build
	$(CXX) $(CXXFLAGS) $(RELEASE_CXXFLAGS) -Icore/test -o $@ $(PROBE_SRC) \
	    -Lbuild -laml_core

KESTREL_PARITY_SRC = research/kestrel_parity/kestrel_parity.cpp $(shell find core/test/support -name '*.cpp' | sort)

$(KESTREL_PARITY_BIN): $(KESTREL_PARITY_SRC) $(CORE_LIB) | build
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(RELEASE_CXXFLAGS) -Icore/test -o $@ $(KESTREL_PARITY_SRC) \
	    -Lbuild -laml_core

SWIFT_PARITY_SRC = research/swift_parity/swift_parity.cpp $(shell find core/test/support -name '*.cpp' | sort)

$(SWIFT_PARITY_BIN): $(SWIFT_PARITY_SRC) $(CORE_LIB) | build
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(RELEASE_CXXFLAGS) -Icore/test -o $@ $(SWIFT_PARITY_SRC) \
	    -Lbuild -laml_core

SWIFT_SCALED_SQUARE_OF_SUM_SRC = research/swift_scaled_square_of_sum/swift_scaled_square_of_sum.cpp

$(SWIFT_SCALED_SQUARE_OF_SUM_BIN): $(SWIFT_SCALED_SQUARE_OF_SUM_SRC) $(CORE_LIB) | build
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(RELEASE_CXXFLAGS) -o $@ $(SWIFT_SCALED_SQUARE_OF_SUM_SRC) \
	    -Lbuild -laml_core

# ==============================================================================
# Compilation pattern rules
# ==============================================================================

# --- core (release | debug | debug_fast) ---

build/obj/core/%.o: core/cpp/%.cpp | build/obj/core
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(RELEASE_CXXFLAGS) -c $< -o $@

build/obj/core_debug/%.o: core/cpp/%.cpp | build/obj/core_debug
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_CXXFLAGS) -c $< -o $@

build/obj/core_debug_fast/%.o: core/cpp/%.cpp | build/obj/core_debug_fast
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_FAST_CXXFLAGS) -c $< -o $@

# --- mcts submodule sources, compiled into core ---

build/obj/core/mcts/%.o: mcts/core/cpp/%.cpp | build/obj/core
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(RELEASE_CXXFLAGS) -c $< -o $@

build/obj/core_debug/mcts/%.o: mcts/core/cpp/%.cpp | build/obj/core_debug
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_CXXFLAGS) -c $< -o $@

build/obj/core_debug_fast/mcts/%.o: mcts/core/cpp/%.cpp | build/obj/core_debug_fast
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_FAST_CXXFLAGS) -c $< -o $@

# --- core tests (debug | debug_fast) ---

build/obj/core_debug_test/%.o: core/test/%.cpp | build/obj/core_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CORE_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/core_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CORE_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/core_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CORE_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_fast_test/%.o: core/test/%.cpp | build/obj/core_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CORE_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_fast_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/core_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CORE_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_fast_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/core_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CORE_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

# ==============================================================================
# Header dependencies (compiler-generated .d files)
# ==============================================================================

-include $(CORE_DEP)
-include $(CORE_DEBUG_DEP)
-include $(CORE_DEBUG_FAST_DEP)
-include $(CORE_DEBUG_TEST_DEP)
-include $(CORE_DEBUG_FAST_TEST_DEP)
-include $(CORE_DEBUG_GTEST_DEP)
-include $(CORE_DEBUG_FAST_GTEST_DEP)
-include $(PROBE_BIN).d

# ==============================================================================
# Build directory creation
# ==============================================================================

build \
build/obj/core build/obj/core_debug build/obj/core_debug_fast \
build/obj/core_debug_test build/obj/core_debug_fast_test:
	mkdir -p $@
