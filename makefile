# ==============================================================================
# Toolchain
# ==============================================================================

MAKEFLAGS += -j$(shell nproc)

CXX      = g++
DEPFLAGS = -MMD -MP
# -I.                : aml root (debug_assert.hpp)
# -Icore/hpp         : aml core headers
# -Icli/hpp          : aml cli headers
# -Iatlas            : atlas root (atlas/debug_assert.hpp, needed by atlas headers)
# -Iatlas/core/hpp   : atlas core headers (expr, rule, bind_map, db, …)
# -Iatlas/mcts/include : mcts headers pulled in transitively by atlas manifests
CXXFLAGS = -std=c++20 -I. -Icore/hpp -Icli/hpp -Iatlas -Iatlas/core/hpp -Iatlas/mcts/include $(DEPFLAGS)
AR       = ar
ARFLAGS  = rcs

DEBUG_CXXFLAGS      = -DDEBUG -g
DEBUG_FAST_CXXFLAGS = -DDEBUG -g -O3
RELEASE_CXXFLAGS    = -O3

# ==============================================================================
# GoogleTest  (submodule at googletest/)
# ==============================================================================

GTEST_INC    = googletest/googletest/include
GMOCK_INC    = googletest/googlemock/include
GTEST_ALL_CC = googletest/googletest/src/gtest-all.cc
GMOCK_ALL_CC = googletest/googlemock/src/gmock-all.cc
GTEST_CPPFLAGS = \
    -I$(GTEST_INC) -I$(GMOCK_INC) \
    -Igoogletest/googletest -Igoogletest/googlemock

# ==============================================================================
# CLI11  (submodule at CLI11/)
# ==============================================================================

CLI11_INC = CLI11/include

# ==============================================================================
# ANTLR4  (system install — required by atlas parser and aml parser)
# ==============================================================================

ANTLR4_INC = /usr/include/antlr4-runtime
ANTLR4_LIB = /usr/lib/x86_64-linux-gnu
ANTLR4_JAR = atlas/tools/antlr4-4.10.1-complete.jar

# ==============================================================================
# Atlas library dependencies  (built from the atlas/ submodule)
# ==============================================================================

ATLAS_CORE_LIB            = atlas/build/libatlas_core.a
ATLAS_CORE_DEBUG_LIB      = atlas/build/libatlas_core_debug.a
ATLAS_CORE_DEBUG_FAST_LIB = atlas/build/libatlas_core_debug_fast.a

ATLAS_PARSER_DEBUG_LIB      = atlas/build/libatlas_parser_debug.a
ATLAS_PARSER_DEBUG_FAST_LIB = atlas/build/libatlas_parser_debug_fast.a

GIT_TAG := $(shell git describe --tags --always --dirty 2>/dev/null || echo dev)

# ==============================================================================
# Output names  (all under build/)
# ==============================================================================

AML_CORE_LIB            = build/libaml_core.a
AML_CORE_DEBUG_LIB      = build/libaml_core_debug.a
AML_CORE_DEBUG_FAST_LIB = build/libaml_core_debug_fast.a

AML_CLI_LIB            = build/libaml_cli.a
AML_CLI_DEBUG_LIB      = build/libaml_cli_debug.a
AML_CLI_DEBUG_FAST_LIB = build/libaml_cli_debug_fast.a

AML_CORE_DEBUG_BIN      = build/core_debug
AML_CORE_DEBUG_FAST_BIN = build/core_debug_fast

AML_PARSER_LIB            = build/libaml_parser.a
AML_PARSER_DEBUG_LIB      = build/libaml_parser_debug.a
AML_PARSER_DEBUG_FAST_LIB = build/libaml_parser_debug_fast.a

AML_PARSER_DEBUG_BIN      = build/parser_debug
AML_PARSER_DEBUG_FAST_BIN = build/parser_debug_fast

AML_CLI_DEBUG_BIN      = build/cli_debug
AML_CLI_DEBUG_FAST_BIN = build/cli_debug_fast

AML_BIN = build/aml

# ==============================================================================
# Object lists
# ==============================================================================

# --- core ---

CORE_SRC = $(shell find core/cpp -name '*.cpp' 2>/dev/null | sort)

AML_CORE_OBJ            = $(patsubst core/cpp/%.cpp, build/obj/core/%.o,            $(CORE_SRC))
AML_CORE_DEBUG_OBJ      = $(patsubst core/cpp/%.cpp, build/obj/core_debug/%.o,      $(CORE_SRC))
AML_CORE_DEBUG_FAST_OBJ = $(patsubst core/cpp/%.cpp, build/obj/core_debug_fast/%.o, $(CORE_SRC))

CORE_TEST_SRC = $(shell find core/test -name '*.cpp' 2>/dev/null | sort)

AML_CORE_DEBUG_TEST_OBJ = \
    $(patsubst core/test/%.cpp, build/obj/core_debug_test/%.o, $(CORE_TEST_SRC))
AML_CORE_DEBUG_FAST_TEST_OBJ = \
    $(patsubst core/test/%.cpp, build/obj/core_debug_fast_test/%.o, $(CORE_TEST_SRC))

AML_CORE_DEBUG_GTEST_OBJ = \
    build/obj/core_debug_test/gtest-all.o \
    build/obj/core_debug_test/gmock-all.o
AML_CORE_DEBUG_FAST_GTEST_OBJ = \
    build/obj/core_debug_fast_test/gtest-all.o \
    build/obj/core_debug_fast_test/gmock-all.o

AML_CORE_DEBUG_TEST_CXXFLAGS      = $(DEBUG_CXXFLAGS) $(GTEST_CPPFLAGS) -Icore/test -Iatlas/parser/hpp -I$(ANTLR4_INC)
AML_CORE_DEBUG_FAST_TEST_CXXFLAGS = $(DEBUG_FAST_CXXFLAGS) $(GTEST_CPPFLAGS) -Icore/test -Iatlas/parser/hpp -I$(ANTLR4_INC)

# --- parser ---

# Hardcoded because parser/generated/ may not exist at make parse time.
AML_PARSER_GENERATED_STEMS = AMLLexer AMLParser AMLBaseVisitor AMLVisitor

AML_PARSER_SRC = $(wildcard parser/cpp/*.cpp)

AML_PARSER_OBJ = \
    $(patsubst %,                build/obj/parser/%.o,            $(AML_PARSER_GENERATED_STEMS)) \
    $(patsubst parser/cpp/%.cpp, build/obj/parser/%.o,            $(AML_PARSER_SRC))
AML_PARSER_DEBUG_OBJ = \
    $(patsubst %,                build/obj/parser_debug/%.o,      $(AML_PARSER_GENERATED_STEMS)) \
    $(patsubst parser/cpp/%.cpp, build/obj/parser_debug/%.o,      $(AML_PARSER_SRC))
AML_PARSER_DEBUG_FAST_OBJ = \
    $(patsubst %,                build/obj/parser_debug_fast/%.o, $(AML_PARSER_GENERATED_STEMS)) \
    $(patsubst parser/cpp/%.cpp, build/obj/parser_debug_fast/%.o, $(AML_PARSER_SRC))

PARSER_TEST_SRC = $(shell find parser/test -name '*.cpp' 2>/dev/null | sort)

AML_PARSER_DEBUG_TEST_OBJ = \
    $(patsubst parser/test/%.cpp, build/obj/parser_debug_test/%.o, $(PARSER_TEST_SRC))
AML_PARSER_DEBUG_FAST_TEST_OBJ = \
    $(patsubst parser/test/%.cpp, build/obj/parser_debug_fast_test/%.o, $(PARSER_TEST_SRC))

AML_PARSER_DEBUG_GTEST_OBJ = \
    build/obj/parser_debug_test/gtest-all.o \
    build/obj/parser_debug_test/gmock-all.o
AML_PARSER_DEBUG_FAST_GTEST_OBJ = \
    build/obj/parser_debug_fast_test/gtest-all.o \
    build/obj/parser_debug_fast_test/gmock-all.o

AML_PARSER_DEBUG_TEST_CXXFLAGS      = $(DEBUG_CXXFLAGS) $(GTEST_CPPFLAGS) -Iparser/test -I$(ANTLR4_INC)
AML_PARSER_DEBUG_FAST_TEST_CXXFLAGS = $(DEBUG_FAST_CXXFLAGS) $(GTEST_CPPFLAGS) -Iparser/test -I$(ANTLR4_INC)

# --- cli ---

CLI_SRC = $(shell find cli/cpp -name '*.cpp' 2>/dev/null | sort)

AML_CLI_OBJ            = $(patsubst cli/cpp/%.cpp, build/obj/cli/%.o,            $(CLI_SRC))
AML_CLI_DEBUG_OBJ      = $(patsubst cli/cpp/%.cpp, build/obj/cli_debug/%.o,      $(CLI_SRC))
AML_CLI_DEBUG_FAST_OBJ = $(patsubst cli/cpp/%.cpp, build/obj/cli_debug_fast/%.o, $(CLI_SRC))

CLI_TEST_SRC = $(shell find cli/test -name '*.cpp' 2>/dev/null | sort)

AML_CLI_DEBUG_TEST_OBJ = \
    $(patsubst cli/test/%.cpp, build/obj/cli_debug_test/%.o, $(CLI_TEST_SRC))
AML_CLI_DEBUG_FAST_TEST_OBJ = \
    $(patsubst cli/test/%.cpp, build/obj/cli_debug_fast_test/%.o, $(CLI_TEST_SRC))

AML_CLI_DEBUG_GTEST_OBJ = \
    build/obj/cli_debug_test/gtest-all.o \
    build/obj/cli_debug_test/gmock-all.o
AML_CLI_DEBUG_FAST_GTEST_OBJ = \
    build/obj/cli_debug_fast_test/gtest-all.o \
    build/obj/cli_debug_fast_test/gmock-all.o

AML_CLI_DEBUG_TEST_CXXFLAGS      = $(DEBUG_CXXFLAGS) $(GTEST_CPPFLAGS) -Icli/test
AML_CLI_DEBUG_FAST_TEST_CXXFLAGS = $(DEBUG_FAST_CXXFLAGS) $(GTEST_CPPFLAGS) -Icli/test

# --- header dependency files ---

AML_CORE_DEP            = $(AML_CORE_OBJ:.o=.d)
AML_CORE_DEBUG_DEP      = $(AML_CORE_DEBUG_OBJ:.o=.d)
AML_CORE_DEBUG_FAST_DEP = $(AML_CORE_DEBUG_FAST_OBJ:.o=.d)
AML_CORE_DEBUG_TEST_DEP      = $(AML_CORE_DEBUG_TEST_OBJ:.o=.d)
AML_CORE_DEBUG_FAST_TEST_DEP = $(AML_CORE_DEBUG_FAST_TEST_OBJ:.o=.d)
AML_CORE_DEBUG_GTEST_DEP      = $(AML_CORE_DEBUG_GTEST_OBJ:.o=.d)
AML_CORE_DEBUG_FAST_GTEST_DEP = $(AML_CORE_DEBUG_FAST_GTEST_OBJ:.o=.d)

AML_PARSER_DEP            = $(AML_PARSER_OBJ:.o=.d)
AML_PARSER_DEBUG_DEP      = $(AML_PARSER_DEBUG_OBJ:.o=.d)
AML_PARSER_DEBUG_FAST_DEP = $(AML_PARSER_DEBUG_FAST_OBJ:.o=.d)
AML_PARSER_DEBUG_TEST_DEP      = $(AML_PARSER_DEBUG_TEST_OBJ:.o=.d)
AML_PARSER_DEBUG_FAST_TEST_DEP = $(AML_PARSER_DEBUG_FAST_TEST_OBJ:.o=.d)
AML_PARSER_DEBUG_GTEST_DEP      = $(AML_PARSER_DEBUG_GTEST_OBJ:.o=.d)
AML_PARSER_DEBUG_FAST_GTEST_DEP = $(AML_PARSER_DEBUG_FAST_GTEST_OBJ:.o=.d)

AML_CLI_DEP            = $(AML_CLI_OBJ:.o=.d)
AML_CLI_DEBUG_DEP      = $(AML_CLI_DEBUG_OBJ:.o=.d)
AML_CLI_DEBUG_FAST_DEP = $(AML_CLI_DEBUG_FAST_OBJ:.o=.d)
AML_CLI_DEBUG_TEST_DEP      = $(AML_CLI_DEBUG_TEST_OBJ:.o=.d)
AML_CLI_DEBUG_FAST_TEST_DEP = $(AML_CLI_DEBUG_FAST_TEST_OBJ:.o=.d)
AML_CLI_DEBUG_GTEST_DEP      = $(AML_CLI_DEBUG_GTEST_OBJ:.o=.d)
AML_CLI_DEBUG_FAST_GTEST_DEP = $(AML_CLI_DEBUG_FAST_GTEST_OBJ:.o=.d)

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

.PHONY: all core core_debug core_debug_fast \
        parser parser_debug parser_debug_fast \
        cli cli_debug cli_debug_fast \
        aml clean

all: core core_debug parser_debug cli cli_debug aml

core: $(ATLAS_CORE_LIB) $(AML_CORE_LIB)

core_debug: $(ATLAS_CORE_DEBUG_LIB) $(AML_CORE_DEBUG_LIB) $(AML_CORE_DEBUG_BIN)

core_debug_fast: $(ATLAS_CORE_DEBUG_FAST_LIB) $(AML_CORE_DEBUG_FAST_LIB) $(AML_CORE_DEBUG_FAST_BIN)

# Parser targets use recursive make: ANTLR codegen must run before the
# generated .cpp files can be compiled.
parser:
	$(MAKE) parser/generated
	$(MAKE) $(AML_PARSER_LIB)

parser_debug:
	$(MAKE) parser/generated
	$(MAKE) $(AML_PARSER_DEBUG_LIB)
	$(MAKE) $(AML_PARSER_DEBUG_BIN)

parser_debug_fast:
	$(MAKE) parser/generated
	$(MAKE) $(AML_PARSER_DEBUG_FAST_LIB)
	$(MAKE) $(AML_PARSER_DEBUG_FAST_BIN)

cli: $(ATLAS_CORE_LIB) $(AML_CORE_LIB) $(AML_CLI_LIB)

cli_debug: $(ATLAS_CORE_DEBUG_LIB) $(AML_CORE_DEBUG_LIB) $(AML_CLI_DEBUG_LIB) $(AML_CLI_DEBUG_BIN)

cli_debug_fast: $(ATLAS_CORE_DEBUG_FAST_LIB) $(AML_CORE_DEBUG_FAST_LIB) $(AML_CLI_DEBUG_FAST_LIB) $(AML_CLI_DEBUG_FAST_BIN)

aml: $(ATLAS_CORE_LIB) $(AML_CORE_LIB) $(AML_CLI_LIB)
	$(CXX) $(CXXFLAGS) $(RELEASE_CXXFLAGS) \
	    -I$(CLI11_INC) \
	    -DAML_GIT_TAG=\"$(GIT_TAG)\" \
	    cli/entry/main.cpp \
	    -Lbuild -laml_cli -laml_core \
	    -Latlas/build -latlas_core \
	    -o $(AML_BIN)

clean:
	rm -rf build
	rm -rf parser/generated

# ==============================================================================
# aml core library archive rules
# ==============================================================================

$(AML_CORE_LIB): $(AML_CORE_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(AML_CORE_DEBUG_LIB): $(AML_CORE_DEBUG_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(AML_CORE_DEBUG_FAST_LIB): $(AML_CORE_DEBUG_FAST_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

# ==============================================================================
# aml parser library archive rules
# ==============================================================================

$(AML_PARSER_LIB): $(AML_PARSER_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(AML_PARSER_DEBUG_LIB): $(AML_PARSER_DEBUG_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(AML_PARSER_DEBUG_FAST_LIB): $(AML_PARSER_DEBUG_FAST_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

# ==============================================================================
# aml cli library archive rules
# ==============================================================================

$(AML_CLI_LIB): $(AML_CLI_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(AML_CLI_DEBUG_LIB): $(AML_CLI_DEBUG_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

$(AML_CLI_DEBUG_FAST_LIB): $(AML_CLI_DEBUG_FAST_OBJ) | build
	$(AR) $(ARFLAGS) $@ $^

# ==============================================================================
# Test binary link rules
# ==============================================================================

$(AML_CORE_DEBUG_BIN): $(AML_CORE_DEBUG_LIB) $(ATLAS_CORE_DEBUG_LIB) $(ATLAS_PARSER_DEBUG_LIB) \
                       $(AML_CORE_DEBUG_TEST_OBJ) $(AML_CORE_DEBUG_GTEST_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(AML_CORE_DEBUG_TEST_OBJ) $(AML_CORE_DEBUG_GTEST_OBJ) \
	    -Lbuild -laml_core_debug \
	    -Latlas/build -latlas_parser_debug -latlas_core_debug \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -lpthread

$(AML_CORE_DEBUG_FAST_BIN): $(AML_CORE_DEBUG_FAST_LIB) $(ATLAS_CORE_DEBUG_FAST_LIB) $(ATLAS_PARSER_DEBUG_FAST_LIB) \
                             $(AML_CORE_DEBUG_FAST_TEST_OBJ) $(AML_CORE_DEBUG_FAST_GTEST_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(AML_CORE_DEBUG_FAST_TEST_OBJ) $(AML_CORE_DEBUG_FAST_GTEST_OBJ) \
	    -Lbuild -laml_core_debug_fast \
	    -Latlas/build -latlas_parser_debug_fast -latlas_core_debug_fast \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -lpthread

$(AML_PARSER_DEBUG_BIN): $(AML_PARSER_DEBUG_LIB) $(AML_CORE_DEBUG_LIB) $(AML_PARSER_DEBUG_TEST_OBJ) $(AML_PARSER_DEBUG_GTEST_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(AML_PARSER_DEBUG_TEST_OBJ) $(AML_PARSER_DEBUG_GTEST_OBJ) \
	    -Lbuild -laml_parser_debug -laml_core_debug \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -lpthread

$(AML_PARSER_DEBUG_FAST_BIN): $(AML_PARSER_DEBUG_FAST_LIB) $(AML_CORE_DEBUG_FAST_LIB) $(AML_PARSER_DEBUG_FAST_TEST_OBJ) $(AML_PARSER_DEBUG_FAST_GTEST_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(AML_PARSER_DEBUG_FAST_TEST_OBJ) $(AML_PARSER_DEBUG_FAST_GTEST_OBJ) \
	    -Lbuild -laml_parser_debug_fast -laml_core_debug_fast \
	    -L$(ANTLR4_LIB) -lantlr4-runtime \
	    -lpthread

$(AML_CLI_DEBUG_BIN): $(AML_CLI_DEBUG_LIB) $(AML_CORE_DEBUG_LIB) $(ATLAS_CORE_DEBUG_LIB) \
                      $(AML_CLI_DEBUG_TEST_OBJ) $(AML_CLI_DEBUG_GTEST_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(AML_CLI_DEBUG_TEST_OBJ) $(AML_CLI_DEBUG_GTEST_OBJ) \
	    -Lbuild -laml_cli_debug -laml_core_debug \
	    -Latlas/build -latlas_core_debug \
	    -lpthread

$(AML_CLI_DEBUG_FAST_BIN): $(AML_CLI_DEBUG_FAST_LIB) $(AML_CORE_DEBUG_FAST_LIB) $(ATLAS_CORE_DEBUG_FAST_LIB) \
                            $(AML_CLI_DEBUG_FAST_TEST_OBJ) $(AML_CLI_DEBUG_FAST_GTEST_OBJ) | build
	$(CXX) $(CXXFLAGS) -o $@ \
	    $(AML_CLI_DEBUG_FAST_TEST_OBJ) $(AML_CLI_DEBUG_FAST_GTEST_OBJ) \
	    -Lbuild -laml_cli_debug_fast -laml_core_debug_fast \
	    -Latlas/build -latlas_core_debug_fast \
	    -lpthread

# ==============================================================================
# Compilation pattern rules
# ==============================================================================

# --- aml core (release | debug | debug_fast) ---

build/obj/core/%.o: core/cpp/%.cpp | build/obj/core
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(RELEASE_CXXFLAGS) -c $< -o $@

build/obj/core_debug/%.o: core/cpp/%.cpp | build/obj/core_debug
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_CXXFLAGS) -c $< -o $@

build/obj/core_debug_fast/%.o: core/cpp/%.cpp | build/obj/core_debug_fast
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_FAST_CXXFLAGS) -c $< -o $@

# --- aml core tests (debug | debug_fast) ---

build/obj/core_debug_test/%.o: core/test/%.cpp $(ATLAS_PARSER_DEBUG_LIB) | build/obj/core_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_CORE_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/core_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_CORE_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/core_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_CORE_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_fast_test/%.o: core/test/%.cpp $(ATLAS_PARSER_DEBUG_FAST_LIB) | build/obj/core_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_CORE_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_fast_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/core_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_CORE_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/core_debug_fast_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/core_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_CORE_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

# --- aml parser generated (release | debug | debug_fast) ---

build/obj/parser/%.o: parser/generated/%.cpp | parser/generated build/obj/parser
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) $(RELEASE_CXXFLAGS) -c $< -o $@

build/obj/parser_debug/%.o: parser/generated/%.cpp | parser/generated build/obj/parser_debug
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) $(DEBUG_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_fast/%.o: parser/generated/%.cpp | parser/generated build/obj/parser_debug_fast
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) $(DEBUG_FAST_CXXFLAGS) -c $< -o $@

# --- aml parser hand-written (release | debug | debug_fast) ---

build/obj/parser/%.o: parser/cpp/%.cpp | build/obj/parser
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) $(RELEASE_CXXFLAGS) -c $< -o $@

build/obj/parser_debug/%.o: parser/cpp/%.cpp | build/obj/parser_debug
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) $(DEBUG_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_fast/%.o: parser/cpp/%.cpp | build/obj/parser_debug_fast
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(ANTLR4_INC) $(DEBUG_FAST_CXXFLAGS) -c $< -o $@

# --- aml parser tests (debug | debug_fast) ---

build/obj/parser_debug_test/%.o: parser/test/%.cpp | build/obj/parser_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_PARSER_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/parser_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_PARSER_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/parser_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_PARSER_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_fast_test/%.o: parser/test/%.cpp | build/obj/parser_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_PARSER_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_fast_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/parser_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_PARSER_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/parser_debug_fast_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/parser_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_PARSER_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

# --- aml cli (release | debug | debug_fast) ---

build/obj/cli/%.o: cli/cpp/%.cpp | build/obj/cli
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(RELEASE_CXXFLAGS) -c $< -o $@

build/obj/cli_debug/%.o: cli/cpp/%.cpp | build/obj/cli_debug
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_CXXFLAGS) -c $< -o $@

build/obj/cli_debug_fast/%.o: cli/cpp/%.cpp | build/obj/cli_debug_fast
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_FAST_CXXFLAGS) -c $< -o $@

# --- aml cli tests (debug | debug_fast) ---

build/obj/cli_debug_test/%.o: cli/test/%.cpp | build/obj/cli_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_CLI_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/cli_debug_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/cli_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_CLI_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/cli_debug_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/cli_debug_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_CLI_DEBUG_TEST_CXXFLAGS) -c $< -o $@

build/obj/cli_debug_fast_test/%.o: cli/test/%.cpp | build/obj/cli_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_CLI_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/cli_debug_fast_test/gtest-all.o: $(GTEST_ALL_CC) | build/obj/cli_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_CLI_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

build/obj/cli_debug_fast_test/gmock-all.o: $(GMOCK_ALL_CC) | build/obj/cli_debug_fast_test
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(AML_CLI_DEBUG_FAST_TEST_CXXFLAGS) -c $< -o $@

# ==============================================================================
# Build directory creation
# ==============================================================================

build \
build/obj/core build/obj/core_debug build/obj/core_debug_fast \
build/obj/core_debug_test build/obj/core_debug_fast_test \
build/obj/parser build/obj/parser_debug build/obj/parser_debug_fast \
build/obj/parser_debug_test build/obj/parser_debug_fast_test \
build/obj/cli build/obj/cli_debug build/obj/cli_debug_fast \
build/obj/cli_debug_test build/obj/cli_debug_fast_test:
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

# ==============================================================================
# Header dependencies
# ==============================================================================

-include $(AML_PARSER_DEP)
-include $(AML_PARSER_DEBUG_DEP)
-include $(AML_PARSER_DEBUG_FAST_DEP)
-include $(AML_PARSER_DEBUG_TEST_DEP)
-include $(AML_PARSER_DEBUG_FAST_TEST_DEP)
-include $(AML_PARSER_DEBUG_GTEST_DEP)
-include $(AML_PARSER_DEBUG_FAST_GTEST_DEP)

-include $(AML_CORE_DEP)
-include $(AML_CORE_DEBUG_DEP)
-include $(AML_CORE_DEBUG_FAST_DEP)
-include $(AML_CORE_DEBUG_TEST_DEP)
-include $(AML_CORE_DEBUG_FAST_TEST_DEP)
-include $(AML_CORE_DEBUG_GTEST_DEP)
-include $(AML_CORE_DEBUG_FAST_GTEST_DEP)
-include $(AML_CLI_DEP)
-include $(AML_CLI_DEBUG_DEP)
-include $(AML_CLI_DEBUG_FAST_DEP)
-include $(AML_CLI_DEBUG_TEST_DEP)
-include $(AML_CLI_DEBUG_FAST_TEST_DEP)
-include $(AML_CLI_DEBUG_GTEST_DEP)
-include $(AML_CLI_DEBUG_FAST_GTEST_DEP)
