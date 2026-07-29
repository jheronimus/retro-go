/******************************************************************************
 * Sega Master System / Game Gear Emulator - Standalone CLI Port Header
 *
 * Provides macro definitions, data structures, and function prototypes
 * required by the smsplus-gx core for headless CLI execution.
 ******************************************************************************/

#ifndef SMSPLUS_H
#define SMSPLUS_H

#include <stdint.h>

#define SOUND_FREQUENCY 44100

#define HOST_WIDTH_RESOLUTION 640
#define HOST_HEIGHT_RESOLUTION 480

#define VIDEO_WIDTH_SMS 256
#define VIDEO_HEIGHT_SMS 192
#define VIDEO_WIDTH_GG 160
#define VIDEO_HEIGHT_GG 144

#define LOCK_VIDEO
#define UNLOCK_VIDEO

/// Game data configuration structure for CLI port.
typedef struct {
	char gamename[256];
	char biosdir[512];
} gamedata_t;

/// Manages SRAM loading and saving for cartridges.
/// @param sram Pointer to SRAM buffer.
/// @param slot_number Memory slot identifier.
/// @param mode SRAM operation mode (SRAM_SAVE or SRAM_LOAD).
void system_manage_sram(uint8_t *sram, uint8_t slot_number, uint8_t mode);

#endif /* SMSPLUS_H */
