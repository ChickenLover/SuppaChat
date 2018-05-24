SRCDIR = src
BINDIR = obj
DEPSDIR= headers
CC=gcc
CFLAGS=-Wall -I$(DEPSDIR)

OBJS = $(patsubst $(SRCDIR)/%.c,$(BINDIR)/%.o,$(wildcard $(SRCDIR)/*.c))

all: build/server

build/server: $(OBJS) 
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm -pthread

$(BINDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS) : $(DEPSDIR)/*.h

clean:
	rm -rf $(BINDIR)/*.o build/server
