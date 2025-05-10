# Top‐level target
all: lpc21isp

CC := gcc

# Detect OSTYPE if unset
ifeq ($(OSTYPE),)
  OSTYPE := $(shell uname)
endif

CFLAGS := -Wall

ifneq ($(findstring freebsd,$(OSTYPE)),)
  CFLAGS += -D__FREEBSD__
endif

ifneq ($(findstring darwin,$(OSTYPE)),)
  CFLAGS += -D__APPLE__
endif

GLOBAL_DEP := adprog.h lpc21isp.h lpcprog.h lpcterm.h

ifdef LIBGPIOD_SUPPORT
  PKG_CONFIG   := pkg-config
  GPIOD_CFLAGS := $(shell $(PKG_CONFIG) --cflags libgpiod)
  GPIOD_LIBS   := $(shell $(PKG_CONFIG) --libs   libgpiod)
  GLOBAL_DEP   += gpio.h

  CFLAGS       += $(GPIOD_CFLAGS) -DLIBGPIOD_SUPPORT
  LDLIBS       := $(GPIOD_LIBS)

  OBJS         := gpio.o
else
  OBJS         :=
  LDLIBS       :=
endif

OBJS += adprog.o lpcprog.o lpcterm.o

adprog.o: adprog.c $(GLOBAL_DEP)
	$(CC) $(CFLAGS) -c -o $@ $<

lpcprog.o: lpcprog.c $(GLOBAL_DEP)
	$(CC) $(CFLAGS) -c -o $@ $<

lpcterm.o: lpcterm.c $(GLOBAL_DEP)
	$(CC) $(CFLAGS) -c -o $@ $<

gpio.o: gpio.c $(GLOBAL_DEP)
	$(CC) $(CFLAGS) -c -o $@ $<

lpc21isp: lpc21isp.c $(OBJS) $(GLOBAL_DEP)
	$(CC) $(CFLAGS) -o $@ lpc21isp.c $(OBJS) $(LDLIBS)

clean:
	$(RM) adprog.o lpcprog.o lpcterm.o gpio.o lpc21isp
