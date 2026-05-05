CC      := gcc
CFLAGS  := -Iinclude -Wall -Wextra -Wpedantic -MMD -MP
LIBS    := -lpng
TARGET  := bin/fbcondecorctl

SRCDIR  := src
OBJDIR  := obj
BINDIR  := bin

SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS    := $(OBJECTS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) $(LIBS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

$(BINDIR) $(OBJDIR):
	mkdir -p $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean
