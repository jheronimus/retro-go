/******************************************************************************
 * Sega Master System / Game Gear Emulator - Dummy Sound Output Implementation
 *
 * Provides stub implementations of sound output functions for headless running.
 ******************************************************************************/

#include <stdint.h>
#include "../sound_output.h"

/// Initializes the dummy sound output subsystem.
void Sound_Init(void)
{
}

/// Updates the dummy sound output stream.
/// @param sound_buffer Pointer to audio sample buffer.
/// @param len Number of samples to output.
void Sound_Update(int16_t *sound_buffer, unsigned long len)
{
	(void)sound_buffer;
	(void)len;
}

/// Closes the dummy sound output subsystem.
void Sound_Close(void)
{
}

/// Pauses dummy sound output.
void Sound_Pause(void)
{
}

/// Resumes dummy sound output.
void Sound_Unpause(void)
{
}
