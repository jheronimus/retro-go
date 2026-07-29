#ifndef __NES_APU_H__
#define __NES_APU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nesconf.h>

struct nes_ctx_t;

struct nes_apu_pulse_t {
	char enabled;
	uint8_t channel;
	char length_enabled;
	uint8_t length_value;
	uint16_t timer_period;
	uint16_t timer_value;
	uint8_t duty_mode;
	uint8_t duty_value;
	char sweep_reload;
	char sweep_enabled;
	char sweep_negate;
	uint8_t sweep_shift;
	uint8_t sweep_period;
	uint8_t sweep_value;
	char envelope_enabled;
	char envelope_loop;
	char envelope_start;
	uint8_t envelope_period;
	uint8_t envelope_value;
	uint8_t envelope_volume;
	uint8_t constant_volume;
};

struct nes_apu_triangle_t {
	char enabled;
	char length_enabled;
	uint8_t length_value;
	uint16_t timer_period;
	uint16_t timer_value;
	uint8_t duty_value;
	uint8_t counter_period;
	uint8_t counter_value;
	char counter_reload;
};

struct nes_apu_noise_t {
	char enabled;
	char mode;
	uint16_t shift_register;
	char length_enabled;
	uint8_t length_value;
	uint16_t timer_period;
	uint16_t timer_value;
	char envelope_enabled;
	char envelope_loop;
	char envelope_start;
	uint8_t envelope_period;
	uint8_t envelope_value;
	uint8_t envelope_volume;
	uint8_t constant_volume;
};

struct nes_apu_dmc_t {
	struct nes_ctx_t * ctx;

	uint16_t sample_address;
	uint16_t sample_length;
	uint16_t current_address;
	uint16_t current_length;
	uint8_t shift_register;
	uint8_t bit_count;
	uint8_t tick_period;
	uint8_t tick_value;
	uint8_t value;
	char enabled;
	char loop;
	char irq;
};

struct nes_apu_t {
	struct nes_ctx_t * ctx;

	struct nes_apu_pulse_t pulse1;
	struct nes_apu_pulse_t pulse2;
	struct nes_apu_triangle_t triangle;
	struct nes_apu_noise_t noise;
	struct nes_apu_dmc_t dmc;
	uint64_t cycles;
	uint8_t frame_period;
	uint8_t frame_value;
	char frame_irq;
};

void nes_apu_init(struct nes_apu_t * apu, struct nes_ctx_t * ctx);
void nes_apu_reset(struct nes_apu_t * apu);
void nes_apu_step(struct nes_apu_t * apu);
uint8_t nes_apu_read_register(struct nes_apu_t * apu, uint16_t addr);
void nes_apu_write_register(struct nes_apu_t * apu, uint16_t addr, uint8_t val);

#ifdef __cplusplus
}
#endif

#endif /* __NES_APU_H__ */
