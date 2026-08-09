CC     := cc
CFLAGS := -Wall -Wextra -pedantic -std=c99 -Iinclude -MMD -MP
LDFLAGS :=

# platform (override with: make PLATFORM=windows)
PLATFORM ?= unix

# --- host OS detection ---
# Determines which shell commands to use for build operations.
ifeq ($(OS),Windows_NT)
    HOST_OS := windows
else
    HOST_OS := unix
endif

# --- platform-specific toolchain ---
ifeq ($(PLATFORM),windows)
    ifneq ($(HOST_OS),windows)
        CC := x86_64-w64-mingw32-gcc
    endif
    TARGET_EXT := .exe
else
    TARGET_EXT :=
endif

BUILD_DIR := build
TARGET    := $(BUILD_DIR)/ze$(TARGET_EXT)

# --- source discovery ---
# Use shell-appropriate commands to find .c files.
ifeq ($(HOST_OS),windows)
    find_c_sources = $(subst \,/,$(shell dir /s /b $(subst /,\,$(1))\*.c 2>nul))
else
    find_c_sources = $(shell find $(1) -name '*.c' 2>/dev/null | sort)
endif

CORE_SRCS     := $(call find_c_sources,src)
PLATFORM_SRCS := $(call find_c_sources,platforms/$(PLATFORM))

SRCS := $(CORE_SRCS) $(PLATFORM_SRCS)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# dependency files
DEP_FILES := $(OBJS:.o=.d)

# --- shell command abstractions ---
ifeq ($(HOST_OS),windows)
    define mkdir_for
        @if not exist "$(subst /,\,$(dir $(1)))" mkdir "$(subst /,\,$(dir $(1)))"
    endef
    RM_BUILD := if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
else
    define mkdir_for
        @mkdir -p $(dir $(1))
    endef
    RM_BUILD := rm -rf $(BUILD_DIR)
endif

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(call mkdir_for,$@)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	$(call mkdir_for,$@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM_BUILD)

run: $(TARGET)
	./$(TARGET)

# automatic header dependencies
-include $(DEP_FILES)
