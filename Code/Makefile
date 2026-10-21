CC ?= gcc
CFLAGS ?= -O2 -std=gnu99 -Wall -Wextra
LDLIBS = -lgmp -lm

SOURCES = Harness.c SchoolbookMul.c FftMul.c KaratsubaMul.c OpenAiMul.c GmpMul.c
TARGET = Benchmark

$(TARGET): $(SOURCES) Common.h
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDLIBS)

clean:
	rm -f $(TARGET) $(TARGET).exe

.PHONY: clean
