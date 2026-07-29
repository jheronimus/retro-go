#include "walnut_cgb.h"

#define IO_JOYP	0x00
#define IO_SB	0x01
#define IO_SC	0x02
#define IO_DIV	0x04
#define IO_TIMA	0x05
#define IO_TMA	0x06
#define IO_TAC	0x07
#define IO_IF	0x0F
#define IO_LCDC	0x40
#define IO_STAT	0x41
#define IO_SCY	0x42
#define IO_SCX	0x43
#define IO_LY	0x44
#define IO_LYC	0x45
#define	IO_DMA	0x46
#define	IO_BGP	0x47
#define	IO_OBP0	0x48
#define IO_OBP1	0x49
#define IO_WY	0x4A
#define IO_WX	0x4B
#define IO_BOOT	0x50
#define IO_IE	0xFF

#define IO_TAC_RATE_MASK	0x3
#define IO_TAC_ENABLE_MASK	0x4

/* LCD Mode defines. */
#define IO_STAT_MODE_HBLANK		0
#define IO_STAT_MODE_VBLANK		1
#define IO_STAT_MODE_OAM_SCAN		2
#define IO_STAT_MODE_LCD_DRAW		3
#define IO_STAT_MODE_VBLANK_OR_TRANSFER_MASK 0x1

#if WALNUT_GB_16BIT_ALIGNED
void gb_run_frame(struct gb_s *gb)
{
	gb->gb_frame = false;
#if (WALNUT_GB_SAFE_DUALFETCH_DMA || WALNUT_GB_SAFE_DUALFETCH_MBC)
	gb->prefetch_invalid=false; // this is toggled internally, only needs to be set once - 60 times a second is no harm though
#endif
	while(!gb->gb_frame)
	{
		__gb_step_cpu_x(gb);
	}
}

void gb_run_frame_dualfetch(struct gb_s *gb)
{
	gb->gb_frame = false;
#if (WALNUT_GB_SAFE_DUALFETCH_DMA || WALNUT_GB_SAFE_DUALFETCH_MBC)
	gb->prefetch_invalid=false; // this is toggled internally, only needs to be set once - 60 times a second is no harm though
#endif
	while(!gb->gb_frame)
	{
			__gb_step_cpu(gb);
	}
}

int gb_get_save_size_s(struct gb_s *gb, size_t *ram_size)
{
	const uint_fast16_t ram_size_location = 0x0149;
	const uint_fast32_t ram_sizes[] =
	{
		/* 0,  2KiB,   8KiB,  32KiB,  128KiB,   64KiB */
		0x00, 0x800, 0x2000, 0x8000, 0x20000, 0x10000
	};
	uint8_t ram_size_code = gb->gb_rom_read(gb, ram_size_location);

	/* MBC2 always has 512 half-bytes of cart RAM.
	 * This assumes that only the lower nibble of each byte is used; the
	 * nibbles are not packed. */
	if(gb->mbc == 2)
	{
		*ram_size = 0x200;
		return 0;
	}

	/* Return -1 on invalid or unsupported RAM size. */
	if(ram_size_code >= WALNUT_GB_ARRAYSIZE(ram_sizes))
		return -1;

	*ram_size = ram_sizes[ram_size_code];
	return 0;
}

WGB_DEPRECATED("Does not return error code. Use gb_get_save_size_s instead.")
uint_fast32_t gb_get_save_size(struct gb_s *gb)
{
	const uint_fast16_t ram_size_location = 0x0149;
	const uint_fast32_t ram_sizes[] =
	{
		/* 0,  2KiB,   8KiB,  32KiB,  128KiB,   64KiB */
		0x00, 0x800, 0x2000, 0x8000, 0x20000, 0x10000
	};
	uint8_t ram_size_code = gb->gb_rom_read(gb, ram_size_location);

	/* MBC2 always has 512 half-bytes of cart RAM.
	 * This assumes that only the lower nibble of each byte is used; the
	 * nibbles are not packed. */
	if(gb->mbc == 2)
		return 0x200;

	/* Return 0 on invalid or unsupported RAM size. */
	if(ram_size_code >= WALNUT_GB_ARRAYSIZE(ram_sizes))
		return 0;

	return ram_sizes[ram_size_code];
}

void gb_init_serial(struct gb_s *gb,
		    void (*gb_serial_tx)(struct gb_s*, const uint8_t),
		    enum gb_serial_rx_ret_e (*gb_serial_rx)(struct gb_s*,
			    uint8_t*))
{
	gb->gb_serial_tx = gb_serial_tx;
	gb->gb_serial_rx = gb_serial_rx;
}

uint8_t gb_colour_hash(struct gb_s *gb)
{
#define ROM_TITLE_START_ADDR	0x0134
#define ROM_TITLE_END_ADDR	0x0143

	uint8_t x = 0;
	uint16_t i;

	for(i = ROM_TITLE_START_ADDR; i <= ROM_TITLE_END_ADDR; i++)
		x += gb->gb_rom_read(gb, i);

	return x;
}

/**
 * Resets the context, and initialises startup values for a DMG console.
 */
void gb_reset(struct gb_s *gb)
{
	gb->gb_halt = false;
	gb->gb_ime = true;

	/* Initialise MBC values. */
	gb->selected_rom_bank = 1;
	gb->cart_ram_bank = 0;
	gb->enable_cart_ram = 0;
	gb->cart_mode_select = 0;

	/* Use values as though the boot ROM was already executed. */
	if(gb->gb_bootrom_read == NULL)
	{
		uint8_t hdr_chk;
		hdr_chk = gb->gb_rom_read(gb, ROM_HEADER_CHECKSUM_LOC) != 0;

		gb->cpu_reg.a = 0x01;
		gb->cpu_reg.f.f_bits.z = 1;
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h = hdr_chk;
		gb->cpu_reg.f.f_bits.c = hdr_chk;
		gb->cpu_reg.bc.reg = 0x0013;
		gb->cpu_reg.de.reg = 0x00D8;
		gb->cpu_reg.hl.reg = 0x014D;
		gb->cpu_reg.sp.reg = 0xFFFE;
		gb->cpu_reg.pc.reg = 0x0100;

		gb->hram_io[IO_DIV ] = 0xAB;
		gb->hram_io[IO_LCDC] = 0x91;
		gb->hram_io[IO_STAT] = 0x85;
		gb->hram_io[IO_BOOT] = 0x01;

		__gb_write(gb, 0xFF26, 0xF1);
#if WALNUT_FULL_GBC_SUPPORT
		if(gb->cgb.cgbMode)
		{
			gb->cpu_reg.a = 0x11;
			gb->cpu_reg.f.f_bits.z = 1;
			gb->cpu_reg.f.f_bits.n = 0;
			gb->cpu_reg.f.f_bits.h = hdr_chk;
			gb->cpu_reg.f.f_bits.c = hdr_chk;
			gb->cpu_reg.bc.reg = 0x0000;
			gb->cpu_reg.de.reg = 0x0008;
			gb->cpu_reg.hl.reg = 0x007C;
			gb->hram_io[IO_DIV] = 0xFF;
		}
#endif

		memset(gb->vram, 0x00, VRAM_SIZE);
	}
	else
	{
		/* Set value as though the console was just switched on.
		 * CPU registers are uninitialised. */
		gb->cpu_reg.pc.reg = 0x0000;
		gb->hram_io[IO_DIV ] = 0x00;
		gb->hram_io[IO_LCDC] = 0x00;
		gb->hram_io[IO_STAT] = 0x84;
		gb->hram_io[IO_BOOT] = 0x00;
	}

	gb->counter.lcd_count = 0;
	gb->counter.div_count = 0;
	gb->counter.tima_count = 0;
	gb->counter.serial_count = 0;
	gb->counter.rtc_count = 0;
	gb->counter.lcd_off_count = 0;

	gb->direct.joypad = 0xFF;
	gb->hram_io[IO_JOYP] = 0xCF;
	gb->hram_io[IO_SB  ] = 0x00;
	gb->hram_io[IO_SC  ] = 0x7E;
#if WALNUT_FULL_GBC_SUPPORT
	if(gb->cgb.cgbMode) gb->hram_io[IO_SC] = 0x7F;
#endif
	/* DIV */
	gb->hram_io[IO_TIMA] = 0x00;
	gb->hram_io[IO_TMA ] = 0x00;
	gb->hram_io[IO_TAC ] = 0xF8;
	gb->hram_io[IO_IF  ] = 0xE1;

	/* LCDC */
	/* STAT */
	gb->hram_io[IO_SCY ] = 0x00;
	gb->hram_io[IO_SCX ] = 0x00;
	gb->hram_io[IO_LY  ] = 0x00;
	gb->hram_io[IO_LYC ] = 0x00;
	__gb_write(gb, 0xFF47, 0xFC); // BGP
	__gb_write(gb, 0xFF48, 0xFF); // OBJP0
	__gb_write(gb, 0xFF49, 0xFF); // OBJP1
	gb->hram_io[IO_WY] = 0x00;
	gb->hram_io[IO_WX] = 0x00;
	gb->hram_io[IO_IE] = 0x00;
	gb->hram_io[IO_IF] = 0xE1;
#if WALNUT_FULL_GBC_SUPPORT
	/* Initialize some CGB registers */
	gb->cgb.doubleSpeed = 0;
	gb->cgb.doubleSpeedPrep = 0;
	gb->cgb.wramBank = 1;
	gb->cgb.wramBankOffset = WRAM_0_ADDR;
	gb->cgb.vramBank = 0;
	gb->cgb.vramBankOffset = VRAM_ADDR;
	for (int i = 0; i < 0x20; i++)
	{
		gb->cgb.OAMPalette[(i << 1)] = gb->cgb.BGPalette[(i << 1)] = 0x7F;
		gb->cgb.OAMPalette[(i << 1) + 1] = gb->cgb.BGPalette[(i << 1) + 1] = 0xFF;
	}
	gb->cgb.OAMPaletteID = 0;
	gb->cgb.BGPaletteID = 0;
	gb->cgb.OAMPaletteInc = 0;
	gb->cgb.BGPaletteInc = 0;
	gb->cgb.dmaActive = 1;  // Not active
	gb->cgb.dmaMode = 0;
	gb->cgb.dmaSize = 0;
	gb->cgb.dmaSource = 0;
	gb->cgb.dmaDest = 0;
#endif
}

enum gb_init_error_e gb_init(struct gb_s *gb,
			     uint8_t (*gb_rom_read)(struct gb_s*, const uint_fast32_t),
					 uint16_t (*gb_rom_read16)(struct gb_s*, const uint_fast32_t),
					 uint32_t (*gb_rom_read32)(struct gb_s*, const uint_fast32_t),
			     uint8_t (*gb_cart_ram_read)(struct gb_s*, const uint_fast32_t),
			     void (*gb_cart_ram_write)(struct gb_s*, const uint_fast32_t, const uint8_t),
			     void (*gb_error)(struct gb_s*, const enum gb_error_e, const uint16_t),
			     void *priv)
{
#if WALNUT_FULL_GBC_SUPPORT
	const uint16_t cgb_flag = 0x0143;
#endif
	const uint16_t mbc_location = 0x0147;
	const uint16_t bank_count_location = 0x0148;
	const uint16_t ram_size_location = 0x0149;
	/**
	 * Table for cartridge type (MBC). -1 if invalid.
	 * TODO: MMM01 is untested.
	 * TODO: MBC6 is untested.
	 * TODO: MBC7 is unsupported.
	 * TODO: POCKET CAMERA is unsupported.
	 * TODO: BANDAI TAMA5 is unsupported.
	 * TODO: HuC3 is unsupported.
	 * TODO: HuC1 is unsupported.
	 **/
	const int8_t cart_mbc[] =
	{
		0, 1, 1, 1, -1, 2, 2, -1, 0, 0, -1, 0, 0, 0, -1, 3,
		3, 3, 3, 3, -1, -1, -1, -1, -1, 5, 5, 5, 5, 5, 5, -1
	};
	/* Whether cart has RAM. */
	const uint8_t cart_ram[] =
	{
		0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0,
		1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0
	};
	/* How large the ROM is in banks of 16 KiB. */
	const uint16_t num_rom_banks_mask[] =
	{
		2, 4, 8, 16, 32, 64, 128, 256, 512
	};
	/* How large the cart RAM is in banks of 8 KiB. Code $01 is unused, but
	 * some early homebrew ROMs supposedly may use this value. */
	const uint8_t num_ram_banks[] = { 0, 1, 1, 4, 16, 8 };

	gb->gb_rom_read = gb_rom_read;
	gb->gb_rom_read_16bit = gb_rom_read16;
	gb->gb_rom_read_32bit = gb_rom_read32;
	gb->gb_cart_ram_read = gb_cart_ram_read;
	gb->gb_cart_ram_write = gb_cart_ram_write;
	gb->gb_error = gb_error;
	gb->direct.priv = priv;

	/* Initialise serial transfer function to NULL. If the front-end does
	 * not provide serial support, Walnut-GB will emulate no cable connected
	 * automatically. */
	gb->gb_serial_tx = NULL;
	gb->gb_serial_rx = NULL;

	gb->gb_bootrom_read = NULL;

	/* Check valid ROM using checksum value. */
	{
		uint8_t x = 0;
		uint16_t i;

		for(i = 0x0134; i <= 0x014C; i++)
			x = x - gb->gb_rom_read(gb, i) - 1;

		if(x != gb->gb_rom_read(gb, ROM_HEADER_CHECKSUM_LOC))
			return GB_INIT_INVALID_CHECKSUM;
	}

	/* Check if cartridge type is supported, and set MBC type. */
	{
#if WALNUT_FULL_GBC_SUPPORT
		gb->cgb.cgbMode = (gb_rom_read(gb, cgb_flag) & 0x80) >> 7;
#endif
		const uint8_t mbc_value = gb->gb_rom_read(gb, mbc_location);

		if(mbc_value > sizeof(cart_mbc) - 1 ||
				(gb->mbc = cart_mbc[mbc_value]) == -1)
			return GB_INIT_CARTRIDGE_UNSUPPORTED;
	}

	gb->num_rom_banks_mask = num_rom_banks_mask[gb_rom_read(gb, bank_count_location)] - 1;
	gb->cart_ram = cart_ram[gb_rom_read(gb, mbc_location)];
	gb->num_ram_banks = num_ram_banks[gb_rom_read(gb, ram_size_location)];

	/* If the ROM says that it support RAM, but has 0 RAM banks, then
	 * disable RAM reads from the cartridge. */
	if(gb->cart_ram == 0 || gb->num_ram_banks == 0)
	{
		gb->cart_ram = 0;
		gb->num_ram_banks = 0;
	}

	/* If MBC3 and number of ROM or RAM banks are larger than 128 or 8,
	 * respectively, then select MBC3O mode. */
	if(gb->mbc == 3)
		gb->cart_is_mbc3O = gb->num_rom_banks_mask > 128 || gb->num_ram_banks > 4;

	/* Note that MBC2 will appear to have no RAM banks, but it actually
	 * always has 512 half-bytes of RAM. Hence, gb->num_ram_banks must be
	 * ignored for MBC2. */

	gb->lcd_blank = false;
	gb->display.lcd_draw_line = NULL;

	gb_reset(gb);

	return GB_INIT_NO_ERROR;
}

const char* gb_get_rom_name(struct gb_s* gb, char *title_str)
{
	uint_fast16_t title_loc = 0x134;
	/* End of title may be 0x13E for newer games. */
	const uint_fast16_t title_end = 0x143;
	const char* title_start = title_str;

	for(; title_loc <= title_end; title_loc++)
	{
		const char title_char = gb->gb_rom_read(gb, title_loc);

		if(title_char >= ' ' && title_char <= '_')
		{
			*title_str = title_char;
			title_str++;
		}
		else
			break;
	}

	*title_str = '\0';
	return title_start;
}

#if ENABLE_LCD
void gb_init_lcd(struct gb_s *gb,
		void (*lcd_draw_line)(struct gb_s *gb,
			const uint8_t *pixels,
			const uint_fast8_t line))
{
	gb->display.lcd_draw_line = lcd_draw_line;

	gb->direct.interlace = false;
	gb->display.interlace_count = false;
	gb->direct.frame_skip = false;
	gb->display.frame_skip_count = false;

	gb->display.window_clear = 0;
	gb->display.WY = 0;

	return;
}
#endif

void gb_set_bootrom(struct gb_s *gb,
		 uint8_t (*gb_bootrom_read)(struct gb_s*, const uint_fast16_t))
{
	gb->gb_bootrom_read = gb_bootrom_read;
}

/**
 * Deprecated. Will be removed in the next major version.
 */
WGB_DEPRECATED("RTC is now ticked internally; this function has no effect")
void gb_tick_rtc(struct gb_s *gb)
{
	(void) gb;
	return;
}

void gb_set_rtc(struct gb_s *gb, const struct tm * const time)
{
	gb->rtc_real.bytes[0] = time->tm_sec;
	gb->rtc_real.bytes[1] = time->tm_min;
	gb->rtc_real.bytes[2] = time->tm_hour;
	gb->rtc_real.bytes[3] = time->tm_yday & 0xFF; /* Low 8 bits of day counter. */
	gb->rtc_real.bytes[4] = time->tm_yday >> 8; /* High 1 bit of day counter. */
}
#endif // WALNUT_GB_HEADER_ONLY
/**
 * Internal function used to step the CPU twice (dual fetch/16-bit).
 */
