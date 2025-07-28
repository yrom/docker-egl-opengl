CC = clang
DEBUG ?= 0
CFLAGS ?= -std=gnu11 -Wall -Wno-unused-function \
	$(shell pkg-config --cflags egl libpng) \
	-I./include -I.
ifeq ($(DEBUG), 1)
	CFLAGS += -g -DDEBUG -O1
else
	CFLAGS += -DNDEBUG -O3
endif
LDFLAGS ?= -L/usr/lib/aarch64-linux-gnu
LDLIBS ?= $(shell pkg-config --libs egl libpng)

BUILD_DIR = build
TARGET = $(BUILD_DIR)/shadertoy
OBJ = $(BUILD_DIR)/shadertoy.o
SRC = shadertoy.c
DEPS = include/glad/gl.h $(wildcard *.h)

.PHONY: all clean test install

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
$(BUILD_DIR)/%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<
$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)
clean:
	rm -rf $(BUILD_DIR)

test: all
	@echo "Running tests..."
	@$(TARGET) --output-dir=$(BUILD_DIR)/capture --max-frames=30
	@echo "Tests completed. Check $(BUILD_DIR)/capture for results."

install: all
	@echo "Installing shadertoy..."
	@cp $(TARGET) /usr/local/bin/shadertoy
	@echo "shadertoy installed successfully."