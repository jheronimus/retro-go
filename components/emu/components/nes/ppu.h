#ifndef __NES_PPU_H__
#define __NES_PPU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nesconf.h>

struct nes_ctx_t;

struct nes_ppu_t {
	struct nes_ctx_t * ctx;

	int cycles;
	int scanline;
	uint64_t frame;

	uint8_t palette_data[32];
	uint8_t name_table_data[2048];
	uint8_t oam_data[256];
	uint8_t oam_address;
	uint8_t front_buf[256 * 240];
	uint8_t back_buf[256 * 240];
	uint8_t * front;
	uint8_t * back;

	uint16_t v;
	uint16_t t;
	uint8_t x;
	uint8_t w;
	uint8_t f;

	uint8_t reg;

	uint8_t nmi_occurred;
	uint8_t nmi_output;
	uint8_t nmi_previous;
	uint8_t nmi_delay;

	uint8_t name_table_byte;
	uint8_t attributeTableByte;
	uint8_t low_tile_byte;
	uint8_t high_tile_byte;
	uint64_t tile_data;

	int sprite_count;
	uint32_t sprite_patterns[8];
	uint8_t sprite_positions[8];
	uint8_t sprite_priorities[8];
	uint8_t sprite_indexes[8];

	uint8_t flag_name_table;
	uint8_t flag_increment;
	uint8_t flag_sprite_table;
	uint8_t flag_background_table;
	uint8_t flag_sprite_size;
	uint8_t flag_master_slave;

	uint8_t flag_grayscale;
	uint8_t flag_show_left_background;
	uint8_t flag_show_left_sprites;
	uint8_t flag_show_background;
	uint8_t flag_show_sprites;
	uint8_t flag_red_tint;
	uint8_t flag_green_tint;
	uint8_t flag_blue_tint;

	uint8_t flag_sprite_zero_hit;
	uint8_t flag_sprite_overflow;

	uint8_t buffered_data;
};

void nes_ppu_init(struct nes_ppu_t * ppu, struct nes_ctx_t * ctx);
void nes_ppu_reset(struct nes_ppu_t * ppu);
void nes_ppu_step(struct nes_ppu_t * ppu);
uint8_t nes_ppu_read_register(struct nes_ppu_t * ppu, uint16_t addr);
void nes_ppu_write_register(struct nes_ppu_t * ppu, uint16_t addr, uint8_t val);
uint8_t nes_ppu_is_white_pixel(struct nes_ppu_t * ppu, int x, int y);

#ifdef __cplusplus
}
#endif

#endif /* __NES_PPU_H__ */
