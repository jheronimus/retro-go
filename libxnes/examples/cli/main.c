/*
 * Copyright(c) 2026 libemu Project.
 * Headless CLI wrapper for libxnes integration with Oracle test suite.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <xnes.h>

#define MAX_SRAM_CHECK_SIZE 8192

static void * file_load(const char * filename, uint64_t * len);
static void run_headless_emulator(struct xnes_ctx_t * ctx, int max_frames);
static bool check_frame_results(struct xnes_ctx_t * ctx);
static bool check_memory_range(struct xnes_ctx_t * ctx, uint16_t start, uint16_t end);
static bool find_string(const uint8_t * buf, size_t buf_len, const char * str);

/// Entry point for headless CLI emulator test runner.
/// @param argc Command line argument count.
/// @param argv Command line argument vectors.
/// @return Exit code 0 on success, 1 on failure.
int main(int argc, char ** argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage: %s <rom.nes>\n", argv[0]);
		return 1;
	}

	uint64_t len = 0;
	void * buf = file_load(argv[1], &len);
	if(!buf)
	{
		fprintf(stderr, "Failed to load ROM file: %s\n", argv[1]);
		return 1;
	}

	struct xnes_ctx_t * ctx = xnes_ctx_alloc(buf, len);
	free(buf);

	if(!ctx)
	{
		fprintf(stderr, "Failed to initialize xnes context\n");
		return 1;
	}

	run_headless_emulator(ctx, 1200);
	xnes_ctx_free(ctx);
	return 0;
}

/// Load a file from disk into a dynamically allocated buffer.
/// @param filename Path to file.
/// @param len Pointer to store file length.
/// @return Pointer to buffer, or NULL on failure.
static void * file_load(const char * filename, uint64_t * len)
{
	FILE * in = fopen(filename, "rb");
	if(!in)
		return NULL;

	fseek(in, 0, SEEK_END);
	long size = ftell(in);
	fseek(in, 0, SEEK_SET);

	if(size <= 0)
	{
		fclose(in);
		return NULL;
	}

	char * buf = malloc((size_t)size);
	if(!buf)
	{
		fclose(in);
		return NULL;
	}

	size_t read_bytes = fread(buf, 1, (size_t)size, in);
	fclose(in);

	if(read_bytes != (size_t)size)
	{
		free(buf);
		return NULL;
	}

	if(len)
		*len = (uint64_t)size;
	return buf;
}

/// Step emulator frames headlessly up to maximum limit and check for completion.
/// @param ctx Pointer to xnes context.
/// @param max_frames Maximum number of frames to execute.
static void run_headless_emulator(struct xnes_ctx_t * ctx, int max_frames)
{
	for(int frame = 0; frame < max_frames; frame++)
	{
		xnes_step_frame(ctx);
		if(check_frame_results(ctx))
			return;
	}
}

/// Check both WRAM and SRAM for test status strings.
/// @param ctx Pointer to xnes context.
/// @return true if test status string was found, false otherwise.
static bool check_frame_results(struct xnes_ctx_t * ctx)
{
	if(check_memory_range(ctx, 0x0000, 0x07FF))
		return true;
	if(check_memory_range(ctx, 0x6000, 0x7FFF))
		return true;
	return false;
}

/// Read CPU memory range into buffer and check for test result strings.
/// @param ctx Pointer to xnes context.
/// @param start Start CPU address.
/// @param end End CPU address (inclusive).
/// @return true if a result string was found and printed, false otherwise.
static bool check_memory_range(struct xnes_ctx_t * ctx, uint16_t start, uint16_t end)
{
	size_t len = (size_t)(end - start + 1);
	if(len > MAX_SRAM_CHECK_SIZE)
		return false;

	static uint8_t buf[MAX_SRAM_CHECK_SIZE];
	for(size_t i = 0; i < len; i++)
		buf[i] = xnes_cpu_read8(&ctx->cpu, (uint16_t)(start + i));

	if(find_string(buf, len, "Passed"))
	{
		puts("Passed");
		return true;
	}
	if(find_string(buf, len, "PASS"))
	{
		puts("PASS");
		return true;
	}
	if(find_string(buf, len, "Failed") || find_string(buf, len, "FAIL"))
	{
		puts("FAIL");
		return true;
	}
	return false;
}

/// Search for a byte pattern inside a buffer.
/// @param buf Pointer to memory buffer.
/// @param buf_len Size of buffer.
/// @param str Target string pattern.
/// @return true if pattern found, false otherwise.
static bool find_string(const uint8_t * buf, size_t buf_len, const char * str)
{
	size_t str_len = strlen(str);
	if(str_len == 0 || buf_len < str_len)
		return false;

	for(size_t i = 0; i <= buf_len - str_len; i++)
	{
		if(memcmp(buf + i, str, str_len) == 0)
			return true;
	}
	return false;
}
