# Makefile for the reconstructed liblgncopenapi.so
#
# Target: MIPS32 (o32 ABI), matching the original binary (ELF32, EM_MIPS,
# ET_DYN). Build with a MIPS cross-compiler, e.g. the Buildroot/OpenWrt
# "mipsel-linux-gnu-gcc" or "mips-linux-gnu-gcc" toolchain.
#
# Usage:
#   make CROSS_COMPILE=mips-linux-gnu-               # big-endian MIPS
#   make CROSS_COMPILE=mipsel-linux-gnu-              # little-endian MIPS
#   make NATIVE=1                                     # sanity-build for the host instead
#
CROSS_COMPILE ?=
CC            := $(CROSS_COMPILE)gcc

TARGET   := liblgncopenapi.so
SRC_DIR  := src
INC_DIR  := include
BUILD_DIR:= build

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

CFLAGS  := -Wall -Wextra -fPIC -std=gnu99 -I$(INC_DIR)
LDFLAGS := -shared -ldl -lpthread

ifeq ($(NATIVE),1)
CC := gcc
endif

.PHONY: all clean syntax-check

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Syntax/type-check every translation unit with the host compiler, without
# linking or targeting MIPS - useful when no MIPS cross toolchain is
# available. Produces no object files, just diagnostics.
syntax-check:
	@for f in $(SRCS); do \
		echo "== $$f =="; \
		gcc -Wall -Wextra -fsyntax-only -std=gnu99 -I$(INC_DIR) $$f || exit 1; \
	done

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
