PRGNAME = sms_cli

SRCDIR = ./source ./source/cpu_cores/z80 ./source/sound ./source/sound/maxim_sn76489 ./source/sound_output/dummy ./source/ports/cli ./source/unzip
VPATH = $(SRCDIR)

SRC_C = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
OBJ_C = $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJS = $(OBJ_C) render.o tms.o

CFLAGS = -O0 -g3 -Wall -Wextra -DLSB_FIRST -std=gnu99 -DALIGN_DWORD -DNOYUV -DMAXIM_PSG
CFLAGS += -Isource -Isource/cpu_cores/z80 -Isource/ports/cli -Isource/sound -Isource/unzip -Isource/sound/maxim_sn76489 -Isource/sound_output

LDFLAGS = -lm

$(PRGNAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(PRGNAME) $^ $(LDFLAGS)

$(OBJ_C) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(PRGNAME) *.o
