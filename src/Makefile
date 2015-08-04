CC = gcc
CFLAGS = -W `pkg-config --cflags gtk+-3.0 gtksourceview-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0 gtksourceview-3.0`
SRCS = gcsv-main.c gcsv-window.c
OBJS = $(SRCS:.c=.o)
EXECUTABLE = gcsvedit

.PHONY: clean

$(EXECUTABLE): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(EXECUTABLE)

all: $(EXECUTABLE)

clean:
	rm -f $(OBJS) $(EXECUTABLE)
