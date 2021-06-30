TARGET = rpipdi
SRC = $(wildcard src/*.c) $(wildcard src/*.cc)
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))
OBJ := $(patsubst src/%.cc,build/%.o,$(OBJ))

CFLAGS += -MD -MP -MT $@ -MF build/dep/$(@F).d
CFLAGS += -O3 -g -Wall -Werror -Isrc -std=c99
CFLAGS += -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=500

all: $(TARGET)

build/%.o: src/%.c
	$(CC) $(CFLAGS) $< -c -o $@

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@

clean:
	rm -rf $(TARGET) build

.PHONY: all clean

# Dependencies
-include $(shell mkdir -p build/dep) $(wildcard build/dep/*)
