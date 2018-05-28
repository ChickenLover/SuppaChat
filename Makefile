SRCDIR = src
BINDIR = obj
DEPSDIR = headers
BUILDDIR = build
CC=gcc
CFLAGS=-Wall -I$(DEPSDIR)
MKDIR_P = mkdir -p

OBJS = $(patsubst $(SRCDIR)/%.c,$(BINDIR)/%.o,$(wildcard $(SRCDIR)/*.c))

.PHONY: directories

all: directories ${BUILDDIR}/server

directories: ${SRCDIR} ${BINDIR} ${BUILDDIR} ${DEPSDIR}

${SRCDIR}:
	${MKDIR_P} ${SRCDIR}

${BINDIR}:
	${MKDIR_P} ${BINDIR}

${BUILDDIR}: ${BUILDDIR}/chats ${BUILDDIR}/users

${BUILDDIR}/chats:
	${MKDIR_P} ${BUILDDIR}/chats

${BUILDDIR}/users:
	${MKDIR_P} ${BUILDDIR}/users

${DEPSDIR}:
	${MKDIR_P} ${DEPSDIR}

build/server: $(OBJS) 
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm -pthread

$(BINDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS) : $(DEPSDIR)/*.h

clean:
	rm -rf $(BINDIR) $(BUILDDIR)
