/******************************************************************************
 * Sega Master System / Game Gear Emulator - Standalone CLI Wrapper Entry
 *
 * Implements a headless CLI entry point for oracle test suite execution.
 * Loads a ROM, steps frames fast, checks WRAM for PASS/FAIL result strings,
 * and exits upon result or after 1200 frames.
 ******************************************************************************/

#include "shared.h"
#include "smsplus.h"

#define MAX_FRAMES 1200
#define WRAM_SIZE 0x2000
#define WRAM_RESULT_LEN 4

t_config option = { 0 };

static uint16_t sms_bitmap_buf[VIDEO_WIDTH_SMS * 240];

/* Private static helper function prototypes */
static void init_options(void);
static void init_bitmap(void);
static int check_wram_match(const uint8_t *wram, size_t idx);
static int check_wram_status(void);
static int run_emulation_loop(void);

/* --- Public Functions --- */

void system_manage_sram(uint8_t *sram, uint8_t slot_number, uint8_t mode)
{
	(void)sram;
	(void)slot_number;
	(void)mode;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <rom_path>\n", argv[0]);
		return 1;
	}

	init_options();
	init_bitmap();

	if (!load_rom(argv[1]))
	{
		fprintf(stderr, "Failed to load ROM: %s\n", argv[1]);
		return 1;
	}

	system_poweron();
	run_emulation_loop();
	system_poweroff();
	system_shutdown();

	return 0;
}

/* --- Private Static Helper Functions --- */

static void init_options(void)
{
	memset(&option, 0, sizeof(option));
	option.fullscreen = 1;
	option.fm = 1;
	option.spritelimit = 1;
	option.tms_pal = 2;
	option.nosound = 1;
	option.soundlevel = 0;
	option.sndrate = SOUND_FREQUENCY;
	option.country = 0;
	option.console = 0;
}

static void init_bitmap(void)
{
	bitmap.width = VIDEO_WIDTH_SMS;
	bitmap.height = 240;
	bitmap.depth = 16;
	bitmap.data = (uint8_t *)sms_bitmap_buf;
	bitmap.pitch = VIDEO_WIDTH_SMS * sizeof(uint16_t);
	bitmap.viewport.w = VIDEO_WIDTH_SMS;
	bitmap.viewport.h = VIDEO_HEIGHT_SMS;
	bitmap.viewport.x = 0;
	bitmap.viewport.y = 0;
}

static int check_wram_match(const uint8_t *wram, size_t idx)
{
	if (wram[idx] == 'P' && wram[idx + 1] == 'A' &&
	    wram[idx + 2] == 'S' && wram[idx + 3] == 'S')
	{
		puts("PASS");
		return 1;
	}
	if (wram[idx] == 'F' && wram[idx + 1] == 'A' &&
	    wram[idx + 2] == 'I' && wram[idx + 3] == 'L')
	{
		puts("FAIL");
		return 1;
	}
	return 0;
}

static int check_wram_status(void)
{
	size_t max_offset = sizeof(sms.wram) - WRAM_RESULT_LEN;

	for (size_t i = 0; i <= max_offset; i++)
	{
		if (check_wram_match(sms.wram, i))
		{
			return 1;
		}
	}
	return 0;
}

static int run_emulation_loop(void)
{
	for (int frame = 0; frame < MAX_FRAMES; frame++)
	{
		system_frame(1);
		if (check_wram_status())
		{
			return 0;
		}
	}
	return 0;
}
