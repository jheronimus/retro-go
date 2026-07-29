CC ?= gcc
CFLAGS ?= -O3 -Wall -Wextra -std=c99 -I.

SRCS = walnut_cgb_cli.c core.c cpu.c mmu.c ppu.c mapper/dummy.c
OBJS = $(SRCS:.c=.o)

all: walnut_cgb_cli

walnut_cgb_cli: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) walnut_cgb_cli
