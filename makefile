# ==============================================================================
# Toolchain
# ==============================================================================

MAKEFLAGS += -j$(shell nproc)

CXX      = g++
DEPFLAGS = -MMD -MP
# -I.                  : aml root (debug_assert.hpp)
# -Icore/hpp           : aml core headers
# -Icli/hpp            : aml cli headers
# -Iatlas              : atlas root (atlas/debug_assert.hpp)
# -Iatlas/core/hpp     : atlas core headers
# -Iatlas/mcts/include : mcts headers pulled in by atlas manifests
CXXFLAGS = -std=c++20 -I. -Icore/hpp -Icli/hpp -Iatlas -Iatlas/core/hpp -Iatlas/mcts/include $(DEPFLAGS)
AR       = ar
ARFLAGS  = rcs

DEBUG_CXXFLAGS      = -DDEBUG -g
DEBUG_FAST_CXXFLAGS = -DDEBUG -g -O3
RELEASE_CXXFLAGS    = -O3

# ==============================================================================
# Paths
# ==============================================================================

ANTLR4_INC = /usr/include/antlr4-runtime
ANTLR4_LIB = /usr/lib/x86_64-linux-gnu
ANTLR4_JAR = atlas/tools/antlr4-4.10.1-complete.jar

GTEST_INC    = googletest/googletest/include
GMOCK_INC    = googletest/googlemock/include
GTEST_ALL_CC = googletest/googletest/src/gtest-all.cc
GMOCK_ALL_CC = googletest/googlemock/src/gmock-all.cc
GTEST_CPPFLAGS = \
    -I$(GTEST_INC) -I$(GMOCK_INC) \
    -Igoogletest/googletest -Igoogletest/googlemock

CLI11_INC = CLI11/include

GIT_TAG := $(shell git describe --tags --always --dirty 2>/dev/null || echo dev)

# Atlas submodule libraries (built via atlas/makefile)
ATLAS_CORE_LIB            = atlas/build/libatlas_core.a
ATLAS_CORE_DEBUG_LIB      = atlas/build/libatlas_core_debug.a
ATLAS_CORE_DEBUG_FAST_LIB = atlas/build/libatlas_core_debug_fast.a
ATLAS_PARSER_DEBUG_LIB      = atlas/build/libatlas_parser_debug.a
ATLAS_PARSER_DEBUG_FAST_LIB = atlas/build/libatlas_parser_debug_fast.a

# ==============================================================================
# Output names  (all under build/)
# ==============================================================================

CORE_LIB            = build/libaml_core.a
CORE_DEBUG_LIB      = build/libaml_core_debug.a
CORE_DEBUG_FAST_LIB = build/libaml_core_debug_fast.a

PARSER_LIB            = build/libaml_parser.a
PARSER_DEBUG_LIB      = build/libaml_parser_debug.a
PARSER_DEBUG_FAST_LIB = build/libaml_parser_debug_fast.a

CLI_LIB            = build/libaml_cli.a
CLI_DEBUG_LIB      = build/libaml_cli_debug.a
CLI_DEBUG_FAST_LIB = build/libaml_cli_debug_fast.a

CORE_DEBUG_BIN      = build/core_debug
CORE_DEBUG_FAST_BIN = build/core_debug_fast

PARSER_DEBUG_BIN      = build/parser_debug
PARSER_DEBUG_FAST_BIN = build/parser_debug_fast

CLI_DEBUG_BIN      = build/cli_debug
CLI_DEBUG_FAST_BIN = build/cli_debug_fast

AML_BIN = build/aml

# ==============================================================================
# Object lists  (object files live in build/obj/<variant>/)
# ==============================================================================

# Core production: discovered at parse time (sources are always present).
CORE_SRC = $(shell find core/cpp -name '*.cpp' | sort)

CORE_OBJ            = $(patsubst core/cpp/%.cpp, build/obj/core/%.o,            $(CORE_SRC))
CORE_DEBUG_OBJ      = $(patsubst core/cpp/%.cpp, build/obj/core_debug/%.o,      $(CORE_SRC))
CORE_DEBUG_FAST_OBJ = $(patsubst core/cpp/%.cpp, build/obj/core_debug_fast/%.o, $(CORE_SRC))

# Core tests
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

CORE_DEBUG_TEST_CXXFLAGS      = $(DEBUG_CXXFLAGS) $(GTEST_CPPFLAGS) -Icore/test -Iatlas/parser/hpp -I$(ANTLR4_INC)
CORE_DEBUG_FAST_TEST_CXXFLAGS = $(DEBUG_FAST_CXXFLAGS) $(GTEST_CPPFLAGS) -Icore/test -Iatlas/parser/hpp -I$(ANTLR4_INC)

# Parser tests
PARSER_TEST_SRC = $(shell find parser/test -name '*.cpp' | sort)

PARSER_DEBUG_TEST_OBJ = \
    $(patsubst parser/test/%.cpp, build/obj/parser_debug_test/%.o, $(PARSER_TEST_SRC))
PARSER_DEBUG_FAST_TEST_OBJ = \
    $(patsubst parser/test/%.cpp, build/obj/parser_debug_fast_test/%.o, $(PARSER_TEST_SRC))

PARSER_DEBUG_GTEST_OBJ = \
    build/obj/parser_debug_test/gtest-all.o \
    build/obj/parser_debug_test/gmock-all.o
PARSER_DEBUG_FAST_GTEST_OBJ = \
    build/obj/parser_debug_fast_test/gtest-all.o \
    build/obj/parser_debug_fast_test/gmock-all.o

PARSER_DEBUG_BIN_OBJ      = $(PARSER_DEBUG_TEST_OBJ) $(PARSER_DEBUG_GTEST_OBJ)
PARSER_DEBUG_FAST_BIN_OBJ = $(PARSER_DEBUG_FAST_TEST_OBJ) $(PARSER_DEBUG_FAST_GTEST_OBJ)

PARSER_DEBUG_TEST_CXXFLAGS      = $(DEBUG_CXXFLAGS) $(GTEST_CPPFLAGS) -Iparser/test -I$(ANTLR4_INC)
PARSER_DEBUG_FAST_TEST_CXXFLAGS = $(DEBUG_FAST_CXXFLAGS) $(GTEST_CPPFLAGS) -Iparser/test -I$(ANTLR4_INC)

# Parser generated: hardcoded because parser/generated/ may not exist at parse
# time, so $(wildcard ...) would expand to nothing.
PARSER_GENERATED_STEMS = AMLLexer AMLParser AMLBaseVisitor AMLVisitor

# Parser hand-written: safe to wildcard — parser/cpp/ always exists.
PARSER_SRC = $(wildcard parser/cpp/*.cpp)

PARSER_OBJ = \
    $(patsubst %,                build/obj/parser/%.o,            $(PARSER_GENERATED_STEMS)) \
    $(patsubst parser/cpp/%.cpp, build/obj/parser/%.o,            $(PARSER_SRC))
PARSER_DEBUG_OBJ = \
    $(patsubst %,                build/obj/parser_debug/%.o,      $(PARSER_GENERATED_STEMS)) \
    $(patsubst parser/cpp/%.cpp, build/obj/parser_debug/%.o,      $(PARSER_SRC))
PARSER_DEBUG_FAST_OBJ = \
    $(patsubst %,                build/obj/parser_debug_fast/%.o, $(PARSER_GENERATED_STEMS)) \
    $(patsubst parser/cpp/%.cpp, build/obj/parser_debug_fast/%.o, $(PARSER_SRC))

# CLI tests
CLI_TEST_SRC = $(shell find cli/test -name '*.cpp' | sort)

CLI_DEBUG_TEST_OBJ = \
    $(patsubst cli/test/%.cpp, build/obj/cli_debug_test/%.o, $(CLI_TEST_SRC))
CLI_DEBUG_FAST_TEST_OBJ = \
    $(patsubst cli/test/%.cpp, build/obj/cli_debug_fast_test/%.o, $(CLI_TEST_SRC))

CLI_DEBUG_GTEST_OBJ = \
    build/obj/cli_debug_test/gtest-all.o \
    build/obj/cli_debug_test/gmock-all.o
CLI_DEBUG_FAST_GTEST_OBJ = \
    build/obj/cli_debug_fast_test/gtest-all.o \
    build/obj/cli_debug_fast_test/gmock-all.o

CLI_DEBUG_BIN_OBJ      = $(CLI_DEBUG_TEST_OBJ) $(CLI_DEBUG_GTEST_OBJ)
CLI_DEBUG_FAST_BIN_OBJ = $(CLI_DEBUG_FAST_TEST_OBJ) $(CLI_DEBUG_FAST_GTEST_OBJ)

CLI_DEBUG_TEST_CXXFLAGS      = $(DEBUG_CXXFLAGS) $(GTEST_CPPFLAGS) -Icli/test -Iatlas/parser/hpp -I$(ANTLR4_INC)
CLI_DEBUG_FAST_TEST_CXXFLAGS = $(DEBUG_FAST_CXXFLAGS) $(GTEST_CPPFLAGS) -Icli/test -Iatlas/parser/hpp -I$(ANTLR4_INC)

# CLI: source files are always present; no codegen needed for compilation.
CLI_SRC = $(shell find cli/cpp -name '*.cpp' | sort)

CLI_OBJ            = $(patsubst cli/cpp/%.cpp, build/obj/cli/%.o,            $(CLI_SRC))
CLI_DEBUG_OBJ      = $(patsubst cli/cpp/%.cpp, build/obj/cli_debug/%.o,      $(CLI_SRC))
CLI_DEBUG_FAST_OBJ = $(patsubst cli/cpp/%.cpp, build/obj/cli_debug_fast/%.o, $(CLI_SRC))

# Compiler-written header deps (one .d per .o); empty until the first compile.
CORE_DEP            = $(CORE_OBJ:.o=.d)
CORE_DEBUG_DEP      = $(CORE_DEBUG_OBJ:.o=.d)
CORE_DEBUG_FAST_DEP = $(CORE_DEBUG_FAST_OBJ:.o=.d)
CORE_DEBUG_TEST_DEP      = $(CORE_DEBUG_TEST_OBJ:.o=.d)
CORE_DEBUG_FAST_TEST_DEP = $(CORE_DEBUG_FAST_TEST_OBJ:.o=.d)
CORE_DEBUG_GTEST_DEP      = $(CORE_DEBUG_GTEST_OBJ:.o=.d)
CORE_DEBUG_FAST_GTEST_DEP = $(CORE_DEBUG_FAST_GTEST_OBJ:.o=.d)
PARSER_DEBUG_TEST_DEP      = $(PARSER_DEBUG_TEST_OBJ:.o=.d)
PARSER_DEBUG_FAST_TEST_DEP = $(PARSER_DEBUG_FAST_TEST_OBJ:.o=.d)
PARSER_DEBUG_GTEST_DEP      = $(PARSER_DEBUG_GTEST_OBJ:.o=.d)
PARSER_DEBUG_FAST_GTEST_DEP = $(PARSER_DEBUG_FAST_GTEST_OBJ:.o=.d)
PARSER_DEP            = $(PARSER_OBJ:.o=.d)
PARSER_DEBUG_DEP      = $(PARSER_DEBUG_OBJ:.o=.d)
PARSER_DEBUG_FAST_DEP = $(PARSER_DEBUG_FAST_OBJ:.o=.d)
CLI_DEBUG_TEST_DEP      = $(CLI_DEBUG_TEST_OBJ:.o=.d)
CLI_DEBUG_FAST_TEST_DEP = $(CLI_DEBUG_FAST_TEST_OBJ:.o=.d)
CLI_DEBUG_GTEST_DEP      = $(CLI_DEBUG_GTEST_OBJ:.o=.d)
CLI_DEBUG_FAST_GTEST_DEP = $(CLI_DEBUG_FAST_GTEST_OBJ:.o=.d)
CLI_DEP            = $(CLI_OBJ:.o=.d)
CLI_DEBUG_DEP      = $(CLI_DEBUG_OBJ:.o=.d)
CLI_DEBUG_FAST_DEP = $(CLI_DEBUG_FAST_OBJ:.o=.d)

# ==============================================================================
# Atlas library build rules  (delegate to atlas/makefile)
# ==============================================================================

$(ATLAS_CORE_LIB):
	$(MAKE) -C atlas build/libatlas_core.a

$(ATLAS_CORE_DEBUG_LIB):
	$(MAKE) -C atlas build/libatlas_core_debug.a

$(ATLAS_CORE_DEBUG_FAST_LIB):
	$(MAKE) -C atlas build/libatlas_core_debug_fast.a

$(ATLAS_PARSER_DEBUG_LIB): $(ATLAS_CORE_DEBUG_LIB)
	$(MAKE) -C atlas parser/generated
	$(MAKE) -C atlas build/libatlas_parser_debug.a

$(ATLAS_PARSER_DEBUG_FAST_LIB): $(ATLAS_CORE_DEBUG_FAST_LIB)
	$(MAKE) -C atlas parser/generated
	$(MAKE) -C atlas build/libatlas_parser_debug_fast.a

# ==============================================================================
# User-facing targets
# ==============================================================================

.PHONY: all core core_debug core_debug_fast parser parser_debug parser_debug_fast \
        cli cli_debug cli_debug_fast aml clean

all: core core_debug core_debug_fast parser parser_debug parser_debug_fast \
     cli cli_debug cli_debug_fast aml

core: $(ATLAS_CORE_LIB) $(CORE_LIB)

core_debug: $(ATLAS_CORE_DEBUG_LIB) $(CORE_DEBUG_BIN)

core_debug_fast: $(ATLAS_CORE_DEBUG_FAST_LIB) $(CORE_DEBUG_FAST_BIN)

# Parser targets use recursive make: the dependency graph is resolved statically
# at startup, before codegen has produced the .cpp files.  Phase 1 runs ANTLR4;
# phase 2 re-invokes make so the pattern rules can find the generated sources.
parser:
	$(MAKE) parser/generated
	$(MAKE) $(PARSER_LIB)

parser_debug: $(CORE_DEBUG_LIB)
	$(MAKE) parser/generated
	$(MAKE) $(PARSER_DEBUG_LIB)
	$(MAKE) $(PARSER_DEBUG_BIN)

parser_debug_fast: $(CORE_DEBUG_FAST_LIB)
	$(MAKE) parser/generated
	$(MAKE) $(PARSER_DEBUG_FAST_LIB)
	$(MAKE) $(PARSER_DEBUG_FAST_BIN)

cli: $(ATLAS_CORE_LIB) $(CORE_LIB)
	$(MAKE) parser/generated
	$(MAKE) $(PARSER_LIB)
	$(MAKE) $(CLI_LIB)

cli_debug: $(ATLAS_CORE_DEBUG_LIB) $(ATLAS_PARSER_DEBUG_LIB) $(CORE_DEBUG_LIB)
	$(MAKE) parser/generated
	$(MAKE) $(PARSER_DEBUG_LIB)
	$(MAKE) $(CLI_DEBUG_LIB)
	$(MAKE) $(CLI_DEBUG_BIN)

cli_debug_fast: $(ATLAS_CORE_DEBUG_FAST_LIB) $(ATLAS_PARSER_DEBUG_FAST_LIB) $(CORE_DEBUG_FAST_LIB)
	$(MAKE) parser/generated
	$(MAKE) $(PARSER_DEBUG_FAST_LIB)
	$(MAKE) $(CLI_DEBUG_FAST_LIB)
	$(MAKE) $(CLI_DEBUG_FAST_BIN)

# Release entrypoint: links cli/entry/main.cpp against the three release libs.
# CLI11 headers are only needed here (the library sources don't use CLI11).
aml: $(ATLAS_CORE_LIB) $(CORE_LIB)
	$(MAKE) parser/generated
	$(MAKE) $(PARSER_LIB)
	$(MAKE) $(CLI_LIB)
	$(CXX) $(CXXFLAGS) $(RELEASE_CXXFLAGS) \
	    -I$(CLI11_INC) -I$(ANTLR4_INC) \
	    -DAML_GIT_TAG=\"$(GIT_TAG)\" \
	    cli/entry/main.cpp \
	    -Lbuild -laml_cli -laml_parser -laml_core \
	    -Latlas/build -latlas_core \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -o $(AML_BIN)

clean:
	rm -rf build
	rm -rf parser/generated

# ==============================================================================
# Library archive rules
# ==============================================================================

$(CORE_LIB): $(CORE_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(CORE_DEBUG_LIB): $(CORE_DEBUG_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(CORE_DEBUG_FAST_LIB): $(CORE_DEBUG_FAST_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(PARSER_LIB): $(PARSER_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(PARSER_DEBUG_LIB): $(PARSER_DEBUG_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(PARSER_DEBUG_FAST_LIB): $(PARSER_DEBUG_FAST_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(CLI_LIB): $(CLI_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(CLI_DEBUG_LIB): $(CLI_DEBUG_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(CLI_DEBUG_FAST_LIB): $(CLI_DEBUG_FAST_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

# ==============================================================================
# Binary link rules
# ==============================================================================

$(CORE_DEBUG_BIN): $(CORE_DEBUG_LIB) $(ATLAS_CORE_DEBUG_LIB) $(ATLAS_PARSER_DEBUG_LIB) \
                   $(CORE_DEBUG_BIN_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(CORE_DEBUG_BIN_OBJ) \
	    -Lbuild -laml_core_debug \
	    -Latlas/build -latlas_parser_debug -latlas_core_debug \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -lpthread

$(CORE_DEBUG_FAST_BIN): $(CORE_DEBUG_FAST_LIB) $(ATLAS_CORE_DEBUG_FAST_LIB) $(ATLAS_PARSER_DEBUG_FAST_LIB) \
                        $(CORE_DEBUG_FAST_BIN_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(CORE_DEBUG_FAST_BIN_OBJ) \
	    -Lbuild -laml_core_debug_fast \
	    -Latlas/build -latlas_parser_debug_fast -latlas_core_debug_fast \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -lpthread

$(PARSER_DEBUG_BIN): $(PARSER_DEBUG_LIB) $(CORE_DEBUG_LIB) $(PARSER_DEBUG_BIN_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(PARSER_DEBUG_BIN_OBJ) \
	    -Lbuild -laml_parser_debug -laml_core_debug \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -lpthread

$(PARSER_DEBUG_FAST_BIN): $(PARSER_DEBUG_FAST_LIB) $(CORE_DEBUG_FAST_LIB) $(PARSER_DEBUG_FAST_BIN_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(PARSER_DEBUG_FAST_BIN_OBJ) \
	    -Lbuild -laml_parser_debug_fast -laml_core_debug_fast \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -lpthread

$(CLI_DEBUG_BIN): $(CLI_DEBUG_LIB) $(PARSER_DEBUG_LIB) $(CORE_DEBUG_LIB) \
                  $(ATLAS_CORE_DEBUG_LIB) $(ATLAS_PARSER_DEBUG_LIB) \
                  $(CLI_DEBUG_BIN_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(CLI_DEBUG_BIN_OBJ) \
	    -Lbuild -laml_cli_debug -laml_parser_debug -laml_core_debug \
	    -Latlas/build -latlas_parser_debug -latlas_core_debug \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -lpthread

$(CLI_DEBUG_FAST_BIN): $(CLI_DEBUG_FAST_LIB) $(PARSER_DEBUG_FAST_LIB) $(CORE_DEBUG_FAST_LIB) \
                       $(ATLAS_CORE_DEBUG_FAST_LIB) $(ATLAS_PARSER_DEBUG_FAST_LIB) \
                       $(CLI_DEBUG_FAST_BIN_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(CLI_DEBUG_FAST_BIN_OBJ) \
	    -Lbuild -laml_cli_debug_fast -laml_parser_debug_fast -laml_core_debug_fast \
	    -Latlas/build -latlas_parser_debug_fast -latlas_core_debug_fast \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -lpthread

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

# --- core tests (debug | debug_fast) ---

build/obj/core_debug_test/%.o: core/test/%.cpp $(ATLAS_PARSER_DEBUG_LIB) | build/obj/core_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CORE_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/core_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CORE_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/core_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CORE_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_fast_test/%.o: core/test/%.cpp $(ATLAS_PARSER_DEBUG_FAST_LIB) | build/obj/core_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CORE_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_fast_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/core_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CORE_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_fast_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/core_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CORE_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

# --- parser tests (debug | debug_fast) ---

build/obj/parser_debug_test/%.o: parser/test/%.cpp | build/obj/parser_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(PARSER_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/parser_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(PARSER_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/parser_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(PARSER_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_fast_test/%.o: parser/test/%.cpp | build/obj/parser_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(PARSER_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_fast_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/parser_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(PARSER_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_fast_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/parser_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(PARSER_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

# --- parser (release | debug | debug_fast) ---

build/obj/parser/%.o: parser/generated/%.cpp | parser/generated build/obj/parser
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) $(RELEASE_CXXFLAGS) -c $< -o $@

build/obj/parser_debug/%.o: parser/generated/%.cpp | parser/generated build/obj/parser_debug
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) $(DEBUG_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_fast/%.o: parser/generated/%.cpp | parser/generated build/obj/parser_debug_fast
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) $(DEBUG_FAST_CXXFLAGS) -c $< -o $@

build/obj/parser/%.o: parser/cpp/%.cpp | build/obj/parser
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) $(RELEASE_CXXFLAGS) -c $< -o $@

build/obj/parser_debug/%.o: parser/cpp/%.cpp | build/obj/parser_debug
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) $(DEBUG_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_fast/%.o: parser/cpp/%.cpp | build/obj/parser_debug_fast
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) $(DEBUG_FAST_CXXFLAGS) -c $< -o $@

# --- cli tests (debug | debug_fast) ---

build/obj/cli_debug_test/%.o: cli/test/%.cpp | build/obj/cli_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CLI_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/cli_debug_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/cli_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CLI_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/cli_debug_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/cli_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CLI_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/cli_debug_fast_test/%.o: cli/test/%.cpp | build/obj/cli_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CLI_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/cli_debug_fast_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/cli_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CLI_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/cli_debug_fast_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/cli_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CLI_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

# --- cli (release | debug | debug_fast) ---

build/obj/cli/%.o: cli/cpp/%.cpp | build/obj/cli
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(RELEASE_CXXFLAGS) -I$(ANTLR4_INC) -c $< -o $@

build/obj/cli_debug/%.o: cli/cpp/%.cpp | build/obj/cli_debug
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_CXXFLAGS) -I$(ANTLR4_INC) -c $< -o $@

build/obj/cli_debug_fast/%.o: cli/cpp/%.cpp | build/obj/cli_debug_fast
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_FAST_CXXFLAGS) -I$(ANTLR4_INC) -c $< -o $@

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
-include $(PARSER_DEBUG_TEST_DEP)
-include $(PARSER_DEBUG_FAST_TEST_DEP)
-include $(PARSER_DEBUG_GTEST_DEP)
-include $(PARSER_DEBUG_FAST_GTEST_DEP)
-include $(CLI_DEBUG_TEST_DEP)
-include $(CLI_DEBUG_FAST_TEST_DEP)
-include $(CLI_DEBUG_GTEST_DEP)
-include $(CLI_DEBUG_FAST_GTEST_DEP)
-include $(PARSER_DEP)
-include $(PARSER_DEBUG_DEP)
-include $(PARSER_DEBUG_FAST_DEP)
-include $(CLI_DEP)
-include $(CLI_DEBUG_DEP)
-include $(CLI_DEBUG_FAST_DEP)

# ==============================================================================
# Build directory creation
# ==============================================================================

build \
build/obj/core build/obj/core_debug build/obj/core_debug_fast \
build/obj/core_debug_test build/obj/core_debug_fast_test \
build/obj/parser_debug_test build/obj/parser_debug_fast_test \
build/obj/cli_debug_test build/obj/cli_debug_fast_test \
build/obj/parser build/obj/parser_debug build/obj/parser_debug_fast \
build/obj/cli build/obj/cli_debug build/obj/cli_debug_fast:
	mkdir -p $@

# ==============================================================================
# ANTLR4 codegen
# ==============================================================================

parser/generated: $(ANTLR4_JAR)
	mkdir -p parser/generated
	cd parser/grammar && java -jar ../../$(ANTLR4_JAR) -Dlanguage=Cpp -visitor -no-listener \
	    -o ../../parser/generated/ AML.g4
	sed -i 's|#include "antlr4-runtime.h"|#include <antlr4-runtime/antlr4-runtime.h>|g' \
	    parser/generated/*.h parser/generated/*.cpp
