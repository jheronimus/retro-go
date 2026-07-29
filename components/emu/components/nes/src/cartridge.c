/*
 * Copyright(c) Jianjun Jiang <8192542@qq.com>
 * Mobile phone: +86-18665388956
 * QQ: 8192542
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <nes.h>

extern void nes_mapper0_init(struct nes_cartridge_t * c);
extern void nes_mapper1_init(struct nes_cartridge_t * c);
extern void nes_mapper2_init(struct nes_cartridge_t * c);
extern void nes_mapper3_init(struct nes_cartridge_t * c);
extern void nes_mapper4_init(struct nes_cartridge_t * c);
extern void nes_mapper7_init(struct nes_cartridge_t * c);
extern void nes_mapper15_init(struct nes_cartridge_t * c);
extern void nes_mapper66_init(struct nes_cartridge_t * c);
extern void nes_mapper79_init(struct nes_cartridge_t * c);
extern void nes_mapper87_init(struct nes_cartridge_t * c);
extern void nes_mapper113_init(struct nes_cartridge_t * c);
extern void nes_mapper140_init(struct nes_cartridge_t * c);
extern void nes_mapper177_init(struct nes_cartridge_t * c);
extern void nes_mapper225_init(struct nes_cartridge_t * c);
extern void nes_mapper241_init(struct nes_cartridge_t * c);

static int nes_cartridge_mapper_init(struct nes_cartridge_t * c)
{
	if(c)
	{
		switch(c->mapper_number)
		{
		case 0:
			nes_mapper0_init(c);
			return 1;
		case 1:
			nes_mapper1_init(c);
			return 1;
		case 2:
			nes_mapper2_init(c);
			return 1;
		case 3:
			nes_mapper3_init(c);
			return 1;
		case 4:
			nes_mapper4_init(c);
			return 1;
		case 7:
			nes_mapper7_init(c);
			return 1;
		case 15:
			nes_mapper15_init(c);
			return 1;
		case 66:
			nes_mapper66_init(c);
			return 1;
		case 79:
			nes_mapper79_init(c);
			return 1;
		case 87:
			nes_mapper87_init(c);
			return 1;
		case 113:
			nes_mapper113_init(c);
			return 1;
		case 140:
			nes_mapper140_init(c);
			return 1;
		case 177:
			nes_mapper177_init(c);
			return 1;
		case 225:
			nes_mapper225_init(c);
			return 1;
		case 241:
			nes_mapper241_init(c);
			return 1;
		default:
			break;
		}
	}
	return 0;
}

struct nes_cartridge_t * nes_cartridge_alloc(const void * buf, size_t len, struct nes_ctx_t * ctx)
{
	uint8_t * p;

	if(!buf || len <= 16)
		return NULL;

	p = (uint8_t *)buf;
	if((p[0] != 'N') || (p[1] != 'E') || (p[2] != 'S') || (p[3] != 0x1a))
		return NULL;

	struct nes_cartridge_t * c = nes_malloc(sizeof(struct nes_cartridge_t));
	if(!c)
		return NULL;

	nes_memset(c, 0, sizeof(struct nes_cartridge_t));
	nes_memcpy(&c->header, buf, 16);
	c->ctx = ctx;

	if(((c->header.flags_7 >> 2) & 0x3) == 0x2)
	{
		c->mapper_number = (((c->header.mapper_msb_submapper >> 4) & 0x0f) << 8) | (((c->header.flags_7 >> 4) & 0x0f) << 4) | (((c->header.flags_6 >> 4) & 0x0f) << 0);
		if((c->header.flags_6 >> 0) & 0x1)
			c->mirror = NES_CARTRIDGE_MIRROR_VERTICAL;
		else
			c->mirror = NES_CARTRIDGE_MIRROR_HORIZONTAL;
		if((c->header.flags_6 >> 3) & 0x1)
			c->mirror = NES_CARTRIDGE_MIRROR_FOUR_SCREEN;

		p += 16;
		if((c->header.flags_6 >> 2) & 0x1)
		{
			c->trainer_size = 512;
			c->trainer = nes_malloc(c->trainer_size);
			nes_memcpy(c->trainer, p, c->trainer_size);
			p += c->trainer_size;
		}
		else
		{
			c->trainer_size = 0;
			c->trainer = NULL;
		}

		c->prg_rom_size = c->header.prg_rom_size_lsb * 16384;
		c->prg_rom = nes_malloc(c->prg_rom_size);
		nes_memcpy(c->prg_rom, p, c->prg_rom_size);
		p += c->prg_rom_size;

		if(((c->header.prg_ram_shift_count >> 0) & 0xf) > 0)
		{
			c->prg_ram_size = 64 << ((c->header.prg_ram_shift_count >> 0) & 0xf);
			c->prg_ram = nes_malloc(c->prg_ram_size);
			nes_memset(c->prg_ram, 0, c->prg_ram_size);
		}
		else
		{
			c->prg_ram_size = 0;
			c->prg_ram = NULL;
		}

		if(((c->header.prg_ram_shift_count >> 4) & 0xf) > 0)
		{
			c->prg_nvram_size = 64 << ((c->header.prg_ram_shift_count >> 4) & 0xf);
			c->prg_nvram = nes_malloc(c->prg_nvram_size);
			nes_memset(c->prg_nvram, 0, c->prg_nvram_size);
		}
		else
		{
			c->prg_nvram_size = 0;
			c->prg_nvram = NULL;
		}

		c->chr_rom_size = c->header.chr_rom_size_lsb * 8192;
		c->chr_rom = nes_malloc(c->chr_rom_size);
		nes_memcpy(c->chr_rom, p, c->chr_rom_size);
		p += c->chr_rom_size;

		if(((c->header.chr_ram_shift_count >> 0) & 0xf) > 0 )
		{
			c->chr_ram_size = 64 << ((c->header.chr_ram_shift_count >> 0) & 0xf);
			c->chr_ram = nes_malloc(c->chr_ram_size);
			nes_memset(c->chr_ram, 0, c->chr_ram_size);
		}
		else
		{
			c->chr_ram_size = 0;
			c->chr_ram = NULL;
		}

		if(((c->header.chr_ram_shift_count >> 4) & 0xf) > 0 )
		{
			c->chr_nvram_size = 64 << ((c->header.chr_ram_shift_count >> 4) & 0xf);
			c->chr_nvram = nes_malloc(c->chr_nvram_size);
			nes_memset(c->chr_nvram, 0, c->chr_nvram_size);
		}
		else
		{
			c->chr_nvram_size = 0;
			c->chr_nvram = NULL;
		}

		switch(c->header.cpu_ppu_timing & 0x3)
		{
		case 0x0:
			c->timing = NES_CARTRIDGE_TIMING_NTSC;
			c->cpu_rate = 1789773;
			c->cpu_rate_adjusted = 1789773;
			c->cpu_period_adjusted = 559;
			break;

		case 0x1:
			c->timing = NES_CARTRIDGE_TIMING_PAL;
			c->cpu_rate = 1662607;
			c->cpu_rate_adjusted = 1662607;
			c->cpu_period_adjusted = 601;
			break;

		case 0x3:
			c->timing = NES_CARTRIDGE_TIMING_DENDY;
			c->cpu_rate = 1773448;
			c->cpu_rate_adjusted = 1773448;
			c->cpu_period_adjusted = 564;
			break;

		default:
			c->timing = NES_CARTRIDGE_TIMING_NTSC;
			c->cpu_rate = 1789773;
			c->cpu_rate_adjusted = 1789773;
			c->cpu_period_adjusted = 559;
			break;
		}
	}
	else
	{
		c->mapper_number = (((c->header.flags_7 >> 4) & 0x0f) << 4) | (((c->header.flags_6 >> 4) & 0x0f) << 0);
		if((c->header.flags_6 >> 0) & 0x1)
			c->mirror = NES_CARTRIDGE_MIRROR_VERTICAL;
		else
			c->mirror = NES_CARTRIDGE_MIRROR_HORIZONTAL;
		if((c->header.flags_6 >> 3) & 0x1)
			c->mirror = NES_CARTRIDGE_MIRROR_FOUR_SCREEN;

		p += 16;
		if((c->header.flags_6 >> 2) & 0x1)
		{
			c->trainer_size = 512;
			c->trainer = nes_malloc(c->trainer_size);
			nes_memcpy(c->trainer, p, c->trainer_size);
			p += c->trainer_size;
		}
		else
		{
			c->trainer_size = 0;
			c->trainer = NULL;
		}

		c->prg_rom_size = c->header.prg_rom_size_lsb * 16384;
		c->prg_rom = nes_malloc(c->prg_rom_size);
		nes_memcpy(c->prg_rom, p, c->prg_rom_size);
		p += c->prg_rom_size;

		c->prg_ram_size = 0;
		c->prg_ram = NULL;

		c->prg_nvram_size = 0;
		c->prg_nvram = NULL;

		c->chr_rom_size = c->header.chr_rom_size_lsb * 8192;
		if(c->chr_rom_size == 0)
			c->chr_rom_size = 8192;
		c->chr_rom = nes_malloc(c->chr_rom_size);
		nes_memcpy(c->chr_rom, p, c->chr_rom_size);
		p += c->chr_rom_size;

		c->chr_ram_size = 0;
		c->chr_ram = NULL;

		c->chr_nvram_size = 0;
		c->chr_nvram = NULL;

		if((c->header.prg_chr_rom_size_msb >> 0) & 0x1)
		{
			c->timing = NES_CARTRIDGE_TIMING_PAL;
			c->cpu_rate_adjusted = 1662607;
			c->cpu_period_adjusted = 601;
		}
		else
		{
			c->timing = NES_CARTRIDGE_TIMING_NTSC;
			c->cpu_rate = 1789773;
			c->cpu_rate_adjusted = 1789773;
			c->cpu_period_adjusted = 559;
		}
	}

	if(!nes_cartridge_mapper_init(c))
	{
		nes_free(c);
		return NULL;
	}
	return c;
}

void nes_cartridge_free(struct nes_cartridge_t * c)
{
	if(c)
	{
		if(c->trainer)
			nes_free(c->trainer);
		if(c->prg_rom)
			nes_free(c->prg_rom);
		if(c->chr_rom)
			nes_free(c->chr_rom);
		if(c->prg_ram)
			nes_free(c->prg_ram);
		if(c->chr_ram)
			nes_free(c->chr_ram);
		nes_free(c);
	}
}

uint8_t nes_cartridge_mapper_cpu_read(struct nes_ctx_t * ctx, uint16_t addr)
{
	return ctx->cartridge->mapper.cpu_read(ctx, addr);
}

void nes_cartridge_mapper_cpu_write(struct nes_ctx_t * ctx, uint16_t addr, uint8_t val)
{
	ctx->cartridge->mapper.cpu_write(ctx, addr, val);
}

uint8_t nes_cartridge_mapper_ppu_read(struct nes_ctx_t * ctx, uint16_t addr)
{
	return ctx->cartridge->mapper.ppu_read(ctx, addr);
}

void nes_cartridge_mapper_ppu_write(struct nes_ctx_t * ctx, uint16_t addr, uint8_t val)
{
	ctx->cartridge->mapper.ppu_write(ctx, addr, val);
}

void nes_cartridge_mapper_apu_step(struct nes_ctx_t * ctx)
{
	if(ctx->cartridge->mapper.apu_step)
		ctx->cartridge->mapper.apu_step(ctx);
}

void nes_cartridge_mapper_ppu_step(struct nes_ctx_t * ctx)
{
	if(ctx->cartridge->mapper.ppu_step)
		ctx->cartridge->mapper.ppu_step(ctx);
}
