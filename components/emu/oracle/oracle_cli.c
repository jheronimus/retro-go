#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAMES_TO_RUN 1200

static void write_sav(const char *rom_path, const uint8_t *data, size_t len)
{
	char sav_path[4096];
	strncpy(sav_path, rom_path, sizeof(sav_path) - 1);
	sav_path[sizeof(sav_path) - 1] = '\0';
	char *dot = strrchr(sav_path, '.');
	if (dot)
		strcpy(dot, ".sav");
	else
		strcat(sav_path, ".sav");

	FILE *f = fopen(sav_path, "wb");
	if (f)
	{
		fwrite(data, 1, len, f);
		fclose(f);
	}
}

#ifdef CORE_GB

#include "gb.h"

static uint8_t  *gb_rom_buf;
static uint8_t  *gb_sram_buf;

static uint8_t  gb_rom_read_cb(struct gb_s *gb, const uint_fast32_t addr)
{
	(void)gb;
	return gb_rom_buf[addr];
}

static uint16_t gb_rom_read16_cb(struct gb_s *gb, const uint_fast32_t addr)
{
	(void)gb;
	return gb_rom_buf[addr] | (gb_rom_buf[addr + 1] << 8);
}

static uint32_t gb_rom_read32_cb(struct gb_s *gb, const uint_fast32_t addr)
{
	(void)gb;
	return gb_rom_buf[addr] | (gb_rom_buf[addr + 1] << 8)
	    | (gb_rom_buf[addr + 2] << 16) | (gb_rom_buf[addr + 3] << 24);
}

static uint8_t  gb_cart_ram_read_cb(struct gb_s *gb, const uint_fast32_t addr)
{
	(void)gb;
	return gb_sram_buf[addr];
}

static void gb_cart_ram_write_cb(struct gb_s *gb, const uint_fast32_t addr, const uint8_t val)
{
	(void)gb;
	gb_sram_buf[addr] = val;
}

static void gb_error_cb(struct gb_s *gb, const enum gb_error_e code, const uint16_t addr)
{
	(void)gb;
	(void)code;
	(void)addr;
}

int main(int argc, char *argv[])
{
	if (argc < 2) return 1;

	FILE *f = fopen(argv[1], "rb");
	if (!f) return 1;
	fseek(f, 0, SEEK_END);
	long rom_size = ftell(f);
	fseek(f, 0, SEEK_SET);
	gb_rom_buf = (uint8_t *)malloc(rom_size);
	fread(gb_rom_buf, 1, rom_size, f);
	fclose(f);

	gb_sram_buf = (uint8_t *)calloc(32768, 1);

	if (gb_rom_buf[0x149] == 0x00)
		gb_rom_buf[0x149] = 0x01;

	struct gb_s gb_ctx;
	memset(&gb_ctx, 0, sizeof(gb_ctx));
	gb_init(&gb_ctx, gb_rom_read_cb, gb_rom_read16_cb, gb_rom_read32_cb,
	        gb_cart_ram_read_cb, gb_cart_ram_write_cb, gb_error_cb, NULL);

	for (int i = 0; i < FRAMES_TO_RUN; i++)
		gb_run_frame(&gb_ctx);

	uint8_t *dump = (uint8_t *)malloc(sizeof(gb_ctx.wram) + 32768);
	memcpy(dump, gb_ctx.wram, sizeof(gb_ctx.wram));
	memcpy(dump + sizeof(gb_ctx.wram), gb_sram_buf, 32768);
	write_sav(argv[1], dump, sizeof(gb_ctx.wram) + 32768);
	free(dump);

	free(gb_rom_buf);
	free(gb_sram_buf);
	return 0;
}

#elif defined(CORE_NES)

#include "nes.h"

int main(int argc, char *argv[])
{
	if (argc < 2) return 1;

	FILE *f = fopen(argv[1], "rb");
	if (!f) return 1;
	fseek(f, 0, SEEK_END);
	long rom_size = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t *rom = (uint8_t *)malloc(rom_size);
	fread(rom, 1, rom_size, f);
	fclose(f);

	rom[6] |= 0x02;

	struct nes_ctx_t *ctx = nes_ctx_alloc(rom, rom_size);
	free(rom);
	if (!ctx) return 1;

	for (int i = 0; i < FRAMES_TO_RUN; i++)
		(void)nes_step_frame(ctx);

	size_t dump_size = sizeof(ctx->cpu.ram) + sizeof(ctx->cartridge->sram);
	uint8_t *dump = (uint8_t *)malloc(dump_size);
	memcpy(dump, ctx->cpu.ram, sizeof(ctx->cpu.ram));
	memcpy(dump + sizeof(ctx->cpu.ram), ctx->cartridge->sram, sizeof(ctx->cartridge->sram));
	write_sav(argv[1], dump, dump_size);
	free(dump);

	nes_ctx_free(ctx);
	return 0;
}

#elif defined(CORE_SMS)

#include "shared.h"
#include "sms.h"
#include "system.h"
#include "loadrom.h"
#include "config.h"

t_config option = {0};

void system_manage_sram(uint8_t *sram, uint8_t slot_number, uint8_t mode)
{
	(void)sram; (void)slot_number; (void)mode;
}

int main(int argc, char *argv[])
{
	if (argc < 2) return 1;

	if (!load_rom((char *)argv[1])) return 1;

	system_init();
	system_poweron();

	for (int i = 0; i < FRAMES_TO_RUN; i++)
		system_frame(0);

	size_t dump_size = sizeof(sms.wram) + sizeof(cart.sram);
	uint8_t *dump = (uint8_t *)malloc(dump_size);
	memcpy(dump, sms.wram, sizeof(sms.wram));
	memcpy(dump + sizeof(sms.wram), cart.sram, sizeof(cart.sram));
	write_sav(argv[1], dump, dump_size);
	free(dump);

	return 0;
}

#elif defined(CORE_PCE)

#include "main.h"
#include "common.h"
#include "memory.h"
#include "cd.h"
#include "audio.h"

config_struct config = {0};
u32 isrunning = 1;

u32 audio_pause(void) { return 0; }
void audio_unpause(void) { }
void quit(void) { }

u8 *preload_state(char *file_name, u32 *_file_length, u32 trim_snapshot)
{
	(void)file_name; (void)_file_length; (void)trim_snapshot;
	return NULL;
}
savestate_extension_enum load_state(char *file_name, u8 *in_memory_state,
                                    u32 in_memory_state_size)
{
	(void)file_name; (void)in_memory_state; (void)in_memory_state_size;
	return 0;
}
void save_state(char *file_name, u16 *snapshot)
{
	(void)file_name; (void)snapshot;
}

void initialize_pce(void)
{
	initialize_memory();
	initialize_video();
	initialize_vce();
	initialize_io();
	initialize_psg();
	initialize_cpu();
	initialize_irq();
	initialize_timer();
	initialize_cd();
	initialize_adpcm();
	initialize_arcade_card();
	initialize_debug();
	initialize_audio();
	initialize_event();
}

int main(int argc, char *argv[])
{
	if (argc < 2) return 1;

	initialize_pce();

	if (load_rom((char *)argv[1]) == -1) return 1;

	for (int i = 0; i < FRAMES_TO_RUN; i++)
		update_frame(0);

	size_t dump_size = sizeof(memory.work_ram) + sizeof(cd.bram);
	uint8_t *dump = (uint8_t *)malloc(dump_size);
	memcpy(dump, memory.work_ram, sizeof(memory.work_ram));
	memcpy(dump + sizeof(memory.work_ram), cd.bram, sizeof(cd.bram));
	write_sav(argv[1], dump, dump_size);
	free(dump);

	return 0;
}

#else
#error "No core defined. Use -DCORE_GB, -DCORE_NES, -DCORE_PCE, or -DCORE_SMS"
#endif
