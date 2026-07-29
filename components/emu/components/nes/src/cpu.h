#ifndef __NES_CPU_H__
#define __NES_CPU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nesconf.h>

struct nes_ctx_t;

enum {
	NES_CPU_P_C	= (1 << 0),	/* carry */
	NES_CPU_P_Z	= (1 << 1),	/* zero */
	NES_CPU_P_I	= (1 << 2),	/* interrupt */
	NES_CPU_P_D	= (1 << 3),	/* decimal */
	NES_CPU_P_B	= (1 << 4),	/* break */
	NES_CPU_P_U	= (1 << 5),	/* unused */
	NES_CPU_P_V	= (1 << 6),	/* overflow */
	NES_CPU_P_N	= (1 << 7),	/* negative */
};

struct nes_cpu_t {
	struct nes_ctx_t * ctx;

	uint8_t ram[2048];
	uint64_t cycles;
	uint32_t stall;
	uint16_t pc;	/* program counter */
	uint8_t sp;		/* stack pointer */
	uint8_t a;		/* accumulator */
	uint8_t x;		/* index register x */
	uint8_t y;		/* index register y */
	uint8_t p;		/* processor status */
	uint8_t interrupt;

	/*
	 * debugger callback, cpu will be paused when return true.
	 */
	int (*debugger)(struct nes_ctx_t *);
};

void nes_cpu_init(struct nes_cpu_t * cpu, struct nes_ctx_t * ctx);
void nes_cpu_reset(struct nes_cpu_t * cpu);
uint8_t nes_cpu_read8(struct nes_cpu_t * cpu, uint16_t addr);
void nes_cpu_write8(struct nes_cpu_t * cpu, uint16_t addr, uint8_t val);
void nes_cpu_trigger_nmi(struct nes_cpu_t * cpu);
void nes_cpu_trigger_irq(struct nes_cpu_t * cpu);
int nes_cpu_step(struct nes_cpu_t * cpu);

#ifdef __cplusplus
}
#endif

#endif /* __NES_CPU_H__ */
