# UniDOS Makefile
# By Nguyen Anh Quynh <aquynh@gmail.com>, 2015

CFLAGS += -Wall
LDFLAGS += $(shell pkg-config --libs glib-2.0) -lpthread -lm -lunicorn

TARGET = unidos

SOURCES  := $(wildcard *.c)
INCLUDES := $(wildcard *.h)
OBJECTS  := $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@$(CC) $(LDFLAGS) $(OBJECTS) -o $(TARGET)
	@echo "  LINKED" $@

%.o: %.c
	@$(CC) $(CFLAGS) -c $^ -o $@
	@echo "  CC    " $@

clean:
	@echo "Deleting all binaries"
	@rm -rf $(TARGET) $(OBJECTS)

.PHONY: all clean obj
