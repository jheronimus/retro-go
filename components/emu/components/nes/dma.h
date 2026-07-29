#ifndef __NES_DMA_H__
#define __NES_DMA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nesconf.h>

struct nes_ctx_t;

struct nes_dma_t {
	struct nes_ctx_t * ctx;
};

void nes_dma_init(struct nes_dma_t * dma, struct nes_ctx_t * ctx);
void nes_dma_reset(struct nes_dma_t * dma);
uint8_t nes_dma_read_register(struct nes_dma_t * dma, uint16_t addr);
void nes_dma_write_register(struct nes_dma_t * dma, uint16_t addr, uint8_t val);

#ifdef __cplusplus
}
#endif

#endif /* __NES_DMA_H__ */
