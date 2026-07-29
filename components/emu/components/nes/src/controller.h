#ifndef __NES_CONTROLLER_H__
#define __NES_CONTROLLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nesconf.h>

struct nes_ctx_t;

enum {
	NES_CONTROLLER_TURBO_SPEED_FAST	= (1 << 1),
	NES_CONTROLLER_TURBO_SPEED_NORMAL	= (1 << 2),
	NES_CONTROLLER_TURBO_SPEED_SLOW	= (1 << 3),
};

enum {
	NES_JOYSTICK_A						= (1 << 7),
	NES_JOYSTICK_B						= (1 << 6),
	NES_JOYSTICK_SELECT				= (1 << 5),
	NES_JOYSTICK_START					= (1 << 4),
	NES_JOYSTICK_UP					= (1 << 3),
	NES_JOYSTICK_DOWN					= (1 << 2),
	NES_JOYSTICK_LEFT					= (1 << 1),
	NES_JOYSTICK_RIGHT					= (1 << 0),
};

struct nes_controller_t {
	struct nes_ctx_t * ctx;

	uint8_t turbo_count;
	uint8_t turbo_speed;
	uint8_t latch;

	struct {
		struct {
			uint8_t key;
			uint8_t key_turbo;
			uint8_t key_index;
		} p1;
		struct {
			uint8_t key;
			uint8_t key_turbo;
			uint8_t key_index;
		} p2;
	} joystick;

	struct {
		uint8_t x, y;
		uint8_t trigger;
	} zapper;
};

void nes_controller_init(struct nes_controller_t * ctl, struct nes_ctx_t * ctx);
void nes_controller_reset(struct nes_controller_t * ctl);
void nes_controller_set_turbo_speed(struct nes_controller_t * ctl, uint8_t speed);
uint8_t nes_controller_read_register(struct nes_controller_t * ctl, uint16_t addr);
void nes_controller_write_register(struct nes_controller_t * ctl, uint16_t addr, uint8_t val);

void nes_controller_joystick_p1(struct nes_controller_t * ctl, uint8_t down, uint8_t up);
void nes_controller_joystick_p2(struct nes_controller_t * ctl, uint8_t down, uint8_t up);
void nes_controller_joystick_p1_turbo(struct nes_controller_t * ctl, uint8_t down, uint8_t up);
void nes_controller_joystick_p2_turbo(struct nes_controller_t * ctl, uint8_t down, uint8_t up);
void nes_controller_zapper(struct nes_controller_t * ctl, uint8_t x, uint8_t y, uint8_t trigger);

#ifdef __cplusplus
}
#endif

#endif /* __NES_CONTROLLER_H__ */
