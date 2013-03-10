CFLAGS= --std=c99 -Wall
LDFLAGS=

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
	CFLAGS += -I/Library/Frameworks/SDL.framework/Headers
	LDFLAGS += -framework SDL
endif
ifeq ($(UNAME), Linux)
	CFLAGS += $(shell sdl-config --cflags)
	LDFLAGS += $(shell sdl-config --static-libs)
endif

SYNTH= synth
OBJ= synth.o audio.o band.o lib.o mq.o

$(SYNTH): $(OBJ)

all: $(SYNTH)

clean:
	$(RM) $(OBJ) $(SYNTH)

.PHONY: all clean

$(OBJ): public.h private.h
