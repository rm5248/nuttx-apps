include $(APPDIR)/Make.defs


PROGNAME = lcc
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_LCC)

# for now, ignore liblcc being built as library, just define it
CFLAGS += -DLIBLCC_BUILD

MAINSRC = lcc_main.c \
lcc-addressed.c \
lcc.c \
lcc-common-internal.c \
lcc-datagram.c \
lcc-event.c \
lcc-gridconnect.c \
lcc_main.c \
lcc-memory.c \
lcc-network-info.c \
lcc-node-info.c \
lcc-print.c \
lcc-simple.c

include $(APPDIR)/Application.mk
