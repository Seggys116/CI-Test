# plain gnu make build; works with gnu make 3.81 and newer, including msys2/mingw on windows
#
#   make                build build/ciutil and the static library
#   make test           build and run every test executable
#   make asan           rebuild into build/asan with asan+ubsan and run the tests
#   make clean          remove build/
#
# CC, CFLAGS, CPPFLAGS, LDFLAGS, AR and BUILD can all be overridden on the command line or environment

BUILD ?= build
STD := -std=c11
WARNINGS := -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow
CFLAGS ?= -O2 -g $(WARNINGS)
SANITIZE_FLAGS := -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

LIB_NAMES := crc32 adler32 base64 roman semver cli common
TEST_NAMES := crc32 adler32 base64 roman semver cli

LIB := $(BUILD)/libciutil.a
CLI := $(BUILD)/ciutil$(EXE)
LIB_OBJS := $(addprefix $(BUILD)/obj/,$(addsuffix .o,$(LIB_NAMES)))
MAIN_OBJ := $(BUILD)/obj/main.o
TEST_EXES := $(addprefix $(BUILD)/tests/test_,$(addsuffix $(EXE),$(TEST_NAMES)))

.PHONY: all test asan clean
.SECONDARY:

all: $(CLI)

$(CLI): $(MAIN_OBJ) $(LIB)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -o $@ $(MAIN_OBJ) $(LIB)

$(LIB): $(LIB_OBJS)
	@mkdir -p $(@D)
	$(AR) rcs $@ $(LIB_OBJS)

$(BUILD)/obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -Iinclude $(STD) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILD)/obj/tests/%.o: tests/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -Iinclude -Itests $(STD) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILD)/tests/test_%$(EXE): $(BUILD)/obj/tests/test_%.o $(LIB)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -o $@ $< $(LIB)

test: $(TEST_EXES)
	@set -e; for t in $(TEST_EXES); do echo "== $$t"; ./$$t; done

asan:
	$(MAKE) BUILD=$(BUILD)/asan CFLAGS="$(CFLAGS) $(SANITIZE_FLAGS)" LDFLAGS="$(LDFLAGS) $(SANITIZE_FLAGS)" test

clean:
	rm -rf build

-include $(wildcard $(BUILD)/obj/*.d $(BUILD)/obj/tests/*.d)
