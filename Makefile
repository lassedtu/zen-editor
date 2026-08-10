CC     := cc
CFLAGS := -Wall -Wextra -pedantic -std=c99 -Iinclude -MMD -MP
LDFLAGS :=

# platform (override with: make PLATFORM=ginnos)
PLATFORM ?= unix

BUILD_DIR := build
TARGET    := $(BUILD_DIR)/ze

# path mapping helpers
src_c_to_obj   = $(patsubst %.c,$(BUILD_DIR)/%.o,$(1))
find_c_sources = $(shell find $(1) -name '*.c' 2>/dev/null | sort)

# automatic source discovery
CORE_SRCS     := $(call find_c_sources,src)
PLATFORM_SRCS := $(call find_c_sources,platforms/$(PLATFORM))

SRCS := $(CORE_SRCS) $(PLATFORM_SRCS)
OBJS := $(call src_c_to_obj,$(SRCS))

# dependency files
DEP_FILES := $(OBJS:.o=.d)

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# pattern rules
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

# testing
TEST_CORE   := $(filter-out src/main.c,$(CORE_SRCS))
TEST_ALL    := tests/test_main.c $(TEST_CORE) $(PLATFORM_SRCS)
TEST_OBJS   := $(call src_c_to_obj,$(TEST_ALL))
TEST_TARGET := $(BUILD_DIR)/ze_tests

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Itests -o $@ $(TEST_OBJS) $(LDFLAGS)

# automatic header dependencies
-include $(DEP_FILES)
