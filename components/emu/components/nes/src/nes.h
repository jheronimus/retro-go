#ifndef __NES_H__
#define __NES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nesconf.h>
#include <cpu.h>
#include <dma.h>
#include <ppu.h>
#include <apu.h>
#include <controller.h>
#include <cartridge.h>

struct nes_ctx_t {
	struct nes_cpu_t cpu;
	struct nes_dma_t dma;
	struct nes_ppu_t ppu;
	struct nes_apu_t apu;
	struct nes_controller_t ctl;
	struct nes_cartridge_t * cartridge;
	uint32_t palette[64];
};

struct nes_ctx_t * nes_ctx_alloc(const void * buf, size_t len);
void nes_ctx_free(struct nes_ctx_t * ctx);
void nes_reset(struct nes_ctx_t * ctx);
void nes_set_debugger(struct nes_ctx_t * ctx, int (*debugger)(struct nes_ctx_t *));
void nes_set_speed(struct nes_ctx_t * ctx, float speed);
uint64_t nes_step_frame(struct nes_ctx_t * ctx);

struct nes_state_t {
	struct nes_ctx_t * ctx;
	unsigned char * buffer;
	uint32_t length;
	uint32_t count;
	uint32_t in;
	uint32_t out;
};

struct nes_state_t * nes_state_alloc(struct nes_ctx_t * ctx, int count);
void nes_state_free(struct nes_state_t * state);
void nes_state_push(struct nes_state_t * state);
void nes_state_pop(struct nes_state_t * state);

#ifdef __cplusplus
}
#endif

#endif /* __NES_H__ */
