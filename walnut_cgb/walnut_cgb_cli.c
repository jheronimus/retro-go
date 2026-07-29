/*
 * walnut_cgb_cli.c - Headless CLI wrapper for Walnut-CGB test execution.
 *
 * Provides a command-line interface for the Walnut-CGB Game Boy/Color core
 * to integrate with the Oracle test suite. Loads test ROMs, steps frames
 * headlessly, and inspects WRAM and SRAM for test status.
 */

#define ENABLE_SOUND 0
#define ENABLE_LCD 0

struct gb_s;
#include "walnut_cgb.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SRAM_SIZE 0x20000
#define MAX_FRAMES 1200

struct priv_s
{
	uint8_t *rom;
	size_t rom_size;
	uint8_t cart_ram[SRAM_SIZE];
};

/// Callback to read a byte from the ROM buffer.
/// @param gb Pointer to emulator context.
/// @param addr ROM address.
/// @return Byte value at address.
static uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr)
{
	const struct priv_s *p = (const struct priv_s *)gb->direct.priv;
	if (addr >= p->rom_size)
	{
		return 0xFF;
	}
	return p->rom[addr];
}

/// Callback to read 16-bit word from the ROM buffer.
/// @param gb Pointer to emulator context.
/// @param addr ROM address.
/// @return 16-bit word value at address.
static uint16_t gb_rom_read16(struct gb_s *gb, const uint_fast32_t addr)
{
	const struct priv_s *p = (const struct priv_s *)gb->direct.priv;
	if (addr + 1 >= p->rom_size)
	{
		return 0xFFFF;
	}
	return (uint16_t)p->rom[addr] | ((uint16_t)p->rom[addr + 1] << 8);
}

/// Callback to read 32-bit dword from the ROM buffer.
/// @param gb Pointer to emulator context.
/// @param addr ROM address.
/// @return 32-bit dword value at address.
static uint32_t gb_rom_read32(struct gb_s *gb, const uint_fast32_t addr)
{
	const struct priv_s *p = (const struct priv_s *)gb->direct.priv;
	if (addr + 3 >= p->rom_size)
	{
		return 0xFFFFFFFF;
	}
	return (uint32_t)p->rom[addr] |
	       ((uint32_t)p->rom[addr + 1] << 8) |
	       ((uint32_t)p->rom[addr + 2] << 16) |
	       ((uint32_t)p->rom[addr + 3] << 24);
}

/// Callback to read a byte from cart RAM.
/// @param gb Pointer to emulator context.
/// @param addr Cart RAM offset.
/// @return Byte value at cart RAM offset.
static uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr)
{
	const struct priv_s *p = (const struct priv_s *)gb->direct.priv;
	return p->cart_ram[addr & (SRAM_SIZE - 1)];
}

/// Callback to write a byte to cart RAM.
/// @param gb Pointer to emulator context.
/// @param addr Cart RAM offset.
/// @param val Byte value to write.
static void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr, const uint8_t val)
{
	struct priv_s *p = (struct priv_s *)gb->direct.priv;
	p->cart_ram[addr & (SRAM_SIZE - 1)] = val;
}

/// Callback for emulator errors (ignored in headless mode).
/// @param gb Pointer to emulator context.
/// @param gb_err Error enum.
/// @param addr Address of error.
static void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t addr)
{
	(void)gb;
	(void)gb_err;
	(void)addr;
}

/// Callback for serial transmission. Outputs character to stdout.
/// @param gb Pointer to emulator context.
/// @param tx Byte transmitted over serial interface.
static void gb_serial_tx(struct gb_s *gb, const uint8_t tx)
{
	(void)gb;
	putchar((int)tx);
	fflush(stdout);
}

/// Loads a ROM file into allocated heap memory.
/// @param path ROM file path.
/// @param out_size Pointer to receive the ROM file size in bytes.
/// @return Pointer to ROM buffer or NULL on error.
static uint8_t *load_rom_file(const char *path, size_t *out_size)
{
	FILE *f = fopen(path, "rb");
	if (!f)
	{
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	if (sz <= 0)
	{
		fclose(f);
		return NULL;
	}
	rewind(f);
	uint8_t *buf = (uint8_t *)malloc((size_t)sz);
	if (!buf)
	{
		fclose(f);
		return NULL;
	}
	size_t read_bytes = fread(buf, 1, (size_t)sz, f);
	fclose(f);
	if (read_bytes != (size_t)sz)
	{
		free(buf);
		return NULL;
	}
	*out_size = (size_t)sz;
	return buf;
}

/// Reads a byte from WRAM or SRAM address.
/// @param gb Pointer to emulator context.
/// @param p Pointer to private state.
/// @param addr 16-bit address in WRAM or SRAM.
/// @return Byte value at address.
static uint8_t get_ram_byte(struct gb_s *gb, const struct priv_s *p, uint16_t addr)
{
	if (addr >= 0xA000 && addr <= 0xBFFF)
	{
		uint8_t b = __gb_read(gb, addr);
		if (b == 0xFF)
		{
			return p->cart_ram[addr - 0xA000];
		}
		return b;
	}
	return __gb_read(gb, addr);
}

/// Converts an ASCII char to uppercase.
static char to_upper(uint8_t c)
{
	if (c >= 'a' && c <= 'z')
	{
		return (char)(c - 'a' + 'A');
	}
	return (char)c;
}

/// Checks if 4 bytes starting at address match PASS, Passed, FAIL, or Failed.
/// @return 1 for PASS, 2 for Passed, -1 for FAIL, -2 for Failed, 0 for no match.
static int check_status_at(struct gb_s *gb, const struct priv_s *p, uint16_t addr)
{
	uint8_t b0 = get_ram_byte(gb, p, addr);
	uint8_t b1 = get_ram_byte(gb, p, (uint16_t)(addr + 1));
	uint8_t b2 = get_ram_byte(gb, p, (uint16_t)(addr + 2));
	uint8_t b3 = get_ram_byte(gb, p, (uint16_t)(addr + 3));

	if ((b0 == 'P' || b0 == 'p') &&
	    (to_upper(b1) == 'A') &&
	    (to_upper(b2) == 'S') &&
	    (to_upper(b3) == 'S'))
	{
		return (b1 == 'a') ? 2 : 1;
	}

	if ((b0 == 'F' || b0 == 'f') &&
	    (to_upper(b1) == 'A') &&
	    (to_upper(b2) == 'I') &&
	    (to_upper(b3) == 'L'))
	{
		return (b1 == 'a') ? -2 : -1;
	}

	return 0;
}

/// Scans a memory range for PASS or FAIL status.
/// @param gb Pointer to emulator context.
/// @param p Pointer to private state.
/// @param start Start address inclusive.
/// @param end End address inclusive.
/// @return non-zero status code if match found, 0 otherwise.
static int scan_ram_range(struct gb_s *gb, const struct priv_s *p, uint16_t start, uint16_t end)
{
	for (uint32_t addr = start; addr <= (uint32_t)(end - 3); addr++)
	{
		int res = check_status_at(gb, p, (uint16_t)addr);
		if (res != 0)
		{
			return res;
		}
	}
	return 0;
}

/// Checks WRAM and SRAM for test completion status.
/// @param gb Pointer to emulator context.
/// @param p Pointer to private state.
/// @return status code (1: PASS, 2: Passed, -1: FAIL, -2: Failed, 0: pending).
static int check_frame_status(struct gb_s *gb, const struct priv_s *p)
{
	int res = scan_ram_range(gb, p, 0xC000, 0xDFFF);
	if (res != 0)
	{
		return res;
	}
	return scan_ram_range(gb, p, 0xA000, 0xBFFF);
}

/// Runs the emulator loop up to MAX_FRAMES.
/// @param gb Pointer to emulator context.
/// @param p Pointer to private state.
/// @return Exit status code.
static int run_emulator(struct gb_s *gb, const struct priv_s *p)
{
	for (unsigned int frame = 0; frame < MAX_FRAMES; frame++)
	{
		gb_run_frame(gb);
		int status = check_frame_status(gb, p);
		if (status == 1)
		{
			puts("PASS");
			fflush(stdout);
			return EXIT_SUCCESS;
		}
		if (status == 2)
		{
			puts("Passed");
			fflush(stdout);
			return EXIT_SUCCESS;
		}
		if (status == -1)
		{
			puts("FAIL");
			fflush(stdout);
			return EXIT_SUCCESS;
		}
		if (status == -2)
		{
			puts("Failed");
			fflush(stdout);
			return EXIT_SUCCESS;
		}
	}
	return EXIT_SUCCESS;
}

/// Main entry point for standalone CLI test wrapper.
/// @param argc Argument count.
/// @param argv Argument vector.
/// @return Exit status code.
int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <rom_path>\n", argv[0]);
		return EXIT_FAILURE;
	}

	static struct priv_s priv;
	memset(&priv, 0, sizeof(priv));

	priv.rom = load_rom_file(argv[1], &priv.rom_size);
	if (!priv.rom)
	{
		fprintf(stderr, "Error loading ROM: %s\n", argv[1]);
		return EXIT_FAILURE;
	}

	struct gb_s gb;
	enum gb_init_error_e err = gb_init(&gb,
		&gb_rom_read, &gb_rom_read16, &gb_rom_read32,
		&gb_cart_ram_read, &gb_cart_ram_write,
		&gb_error, &priv);

	if (err != GB_INIT_NO_ERROR && err != GB_INIT_INVALID_CHECKSUM)
	{
		fprintf(stderr, "Walnut-CGB init failed with error %d\n", err);
		free(priv.rom);
		return EXIT_FAILURE;
	}

	gb_init_serial(&gb, &gb_serial_tx, NULL);
	int res = run_emulator(&gb, &priv);

	free(priv.rom);
	return res;
}
