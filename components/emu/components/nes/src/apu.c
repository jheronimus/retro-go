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

static const uint8_t length_table[] = {
	10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
	12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

static const uint8_t duty_table[4][8] = {
	{ 0, 1, 0, 0, 0, 0, 0, 0 },
	{ 0, 1, 1, 0, 0, 0, 0, 0 },
	{ 0, 1, 1, 1, 1, 0, 0, 0 },
	{ 1, 0, 0, 1, 1, 1, 1, 1 },
};

static const uint8_t triangle_table[] = {
	15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};

static const uint16_t noise_table[] = {
	4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
};




static inline void pulse_write_control(struct nes_apu_pulse_t * p, uint8_t value)
{
	p->duty_mode = (value >> 6) & 3;
	p->length_enabled = (value & 0x20) == 0;
	p->envelope_loop = (value & 0x20) != 0;
	p->envelope_enabled = (value & 0x10) == 0;
	p->constant_volume = (value & 0x10) != 0;
	p->envelope_volume = value & 0x0F;
}

static inline void pulse_write_sweep(struct nes_apu_pulse_t * p, uint8_t value)
{
	p->sweep_enabled = (value & 0x80) != 0;
	p->sweep_period = ((value >> 4) & 0x07) + 1;
	p->sweep_negate = (value & 0x08) != 0;
	p->sweep_shift = value & 0x07;
	p->sweep_reload = 1;
}

static inline void pulse_write_timer_low(struct nes_apu_pulse_t * p, uint8_t value)
{
	p->timer_period = (p->timer_period & 0xFF00) | value;
}

static inline void pulse_write_timer_high(struct nes_apu_pulse_t * p, uint8_t value)
{
	if(p->enabled)
		p->length_value = length_table[value >> 3];
	p->timer_period = (p->timer_period & 0x00FF) | ((uint16_t)(value & 0x7) << 8);
	p->envelope_start = 1;
	p->duty_value = 0;
}

static inline void pulse_step_timer(struct nes_apu_pulse_t * p)
{
	if(p->timer_value == 0)
	{
		p->timer_value = p->timer_period;
		p->duty_value = (p->duty_value + 1) & 0x7;
	}
	else
		p->timer_value--;
}

static inline void pulse_step_envelope(struct nes_apu_pulse_t * p)
{
	if(p->envelope_start)
	{
		p->envelope_volume = 15;
		p->envelope_value = p->envelope_period;
		p->envelope_start = 0;
	}
	else if(p->envelope_value > 0)
	{
		p->envelope_value--;
	}
	else
	{
		if(p->envelope_volume > 0)
			p->envelope_volume--;
		else if(p->envelope_loop)
			p->envelope_volume = 15;
		p->envelope_value = p->envelope_period;
	}
}

static inline void pulse_sweep(struct nes_apu_pulse_t * p)
{
	uint16_t delta = p->timer_period >> p->sweep_shift;
	if(p->sweep_negate)
	{
		p->timer_period -= delta;
		if(p->channel == 1)
			p->timer_period--;
	}
	else
		p->timer_period += delta;
}

static inline void pulse_step_sweep(struct nes_apu_pulse_t * p)
{
	if(p->sweep_reload)
	{
		if(p->sweep_enabled && p->sweep_value == 0)
			pulse_sweep(p);
		p->sweep_value = p->sweep_period;
		p->sweep_reload = 0;
	}
	else if(p->sweep_value > 0)
	{
		p->sweep_value--;
	}
	else
	{
		if(p->sweep_enabled)
			pulse_sweep(p);
		p->sweep_value = p->sweep_period;
	}
}

static inline void pulse_step_length(struct nes_apu_pulse_t * p)
{
	if(p->length_enabled && (p->length_value > 0))
		p->length_value--;
}

static inline uint8_t pulse_output(struct nes_apu_pulse_t * p)
{
	if(!p->enabled)
		return 0;
	if(p->length_value == 0)
		return 0;
	if(duty_table[p->duty_mode][p->duty_value] == 0)
		return 0;
	if((p->timer_period < 8) || (p->timer_period > 0x7ff))
		return 0;
	if(p->envelope_enabled)
		return p->envelope_volume;
	else
		return p->constant_volume;
}

static inline void triangle_write_control(struct nes_apu_triangle_t * t, uint8_t value)
{
	t->length_enabled = (value & 0x80) ? 0 : 1;
	t->counter_period = value & 0x7f;
}

static inline void triangle_write_timer_low(struct nes_apu_triangle_t * t, uint8_t value)
{
	t->timer_period = (t->timer_period & 0xff00) | ((uint16_t)value);
}

static inline void triangle_write_timer_high(struct nes_apu_triangle_t * t, uint8_t value)
{
	t->length_value = length_table[(value >> 3) & 0x1f];
	t->timer_period = (t->timer_period & 0x00ff) | (((uint16_t)(value & 0x7)) << 8);
	t->timer_value = t->timer_period;
	t->counter_reload = 1;
}

static inline void triangle_step_timer(struct nes_apu_triangle_t * t)
{
	if(t->timer_value == 0)
	{
		t->timer_value = t->timer_period;
		if((t->length_value > 0) && (t->counter_value > 0))
			t->duty_value = (t->duty_value + 1) % 32;
	}
	else
		t->timer_value--;
}

static inline void triangle_step_length(struct nes_apu_triangle_t * t)
{
	if(t->length_enabled && (t->length_value > 0))
		t->length_value--;
}

static inline void triangle_step_counter(struct nes_apu_triangle_t * t)
{
	if(t->counter_reload)
		t->counter_value = t->counter_period;
	else if(t->counter_value > 0)
		t->counter_value--;
	if(t->length_enabled)
		t->counter_reload = 0;
}

static inline uint8_t triangle_output(struct nes_apu_triangle_t * t)
{
	if(!t->enabled)
		return 0;
	if(t->timer_period < 3)
		return 0;
	if(t->length_value == 0)
		return 0;
	if(t->counter_value == 0)
		return 0;
	return triangle_table[t->duty_value];
}

static inline void noise_write_control(struct nes_apu_noise_t * n, uint8_t value)
{
	n->length_enabled = (value & 0x20) ? 0 : 1;
	n->envelope_loop = (value & 0x20) ? 1 : 0;
	n->envelope_enabled = (value & 0x10) ? 0 : 1;
	n->envelope_period = value & 0xf;
	n->constant_volume = value & 0xf;
	n->envelope_start = 1;
}

static inline void noise_write_period(struct nes_apu_noise_t * n, uint8_t value)
{
	n->mode = (value & 0x80) ? 1 : 0;
	n->timer_period = noise_table[value & 0xf];
}

static inline void noise_write_length(struct nes_apu_noise_t * n, uint8_t value)
{
	n->length_value = length_table[(value >> 3) & 0x1f];
	n->envelope_start = 1;
}

static inline void noise_step_timer(struct nes_apu_noise_t * n)
{
	if(n->timer_value == 0)
	{
		n->timer_value = n->timer_period;
		uint8_t shift = 0;
		if(n->mode)
			shift = 6;
		else
			shift = 1;
		uint16_t b1 = n->shift_register & 0x1;
		uint16_t b2 = (n->shift_register >> shift) & 0x1;
		n->shift_register >>= 1;
		n->shift_register |= (b1 ^ b2) << 14;
	}
	else
		n->timer_value--;
}

static inline void noise_step_envelope(struct nes_apu_noise_t * n)
{
	if(n->envelope_start)
	{
		n->envelope_volume = 15;
		n->envelope_value = n->envelope_period;
		n->envelope_start = 0;
	}
	else if(n->envelope_value > 0)
	{
		n->envelope_value--;
	}
	else
	{
		if(n->envelope_volume > 0)
			n->envelope_volume--;
		else if(n->envelope_loop)
			n->envelope_volume = 15;
		n->envelope_value = n->envelope_period;
	}
}

static inline void noise_step_length(struct nes_apu_noise_t * n)
{
	if(n->length_enabled && (n->length_value > 0))
		n->length_value--;
}

static inline uint8_t noise_output(struct nes_apu_noise_t * n)
{
	if(!n->enabled)
		return 0;
	if(n->length_value == 0)
		return 0;
	if(n->shift_register & 0x1)
		return 0;
	if(n->envelope_enabled)
		return n->envelope_volume;
	else
		return n->constant_volume;
}

static inline void dmc_restart(struct nes_apu_dmc_t * d)
{
	d->current_address = d->sample_address;
	d->current_length = d->sample_length;
}

static inline void dmc_step_reader(struct nes_apu_dmc_t * d)
{
	if((d->current_length > 0) && (d->bit_count == 0))
	{
		d->ctx->cpu.stall += 4;
		d->shift_register = nes_cpu_read8(&d->ctx->cpu, d->current_address);
		d->bit_count = 8;
		d->current_address++;
		if(d->current_address == 0)
			d->current_address = 0x8000;
		d->current_length--;
		if((d->current_length == 0) && d->loop)
			dmc_restart(d);
	}
}

static inline void dmc_step_shifter(struct nes_apu_dmc_t * d)
{
	if(d->bit_count == 0)
		return;
	if(d->shift_register & 0x1)
	{
		if(d->value <= 125)
			d->value += 2;
	}
	else
	{
		if(d->value >= 2)
			d->value -= 2;
	}
	d->shift_register >>= 1;
	d->bit_count--;
}

static inline void dmc_step_timer(struct nes_apu_dmc_t * d)
{
    if(!d->enabled)
		return;
	dmc_step_reader(d);
	if(d->tick_value == 0)
	{
		d->tick_value = d->tick_period;
		dmc_step_shifter(d);
	}
	else
		d->tick_value--;
}

static inline uint8_t dmc_output(struct nes_apu_dmc_t * d)
{
	return d->value;
}

/*
 * y[n] = b0 * x[n] + b1 * x[n - 1] - a1 * y[n - 1]
 */


static inline void step_envelope(struct nes_apu_t * apu)
{
	pulse_step_envelope(&apu->pulse1);
	pulse_step_envelope(&apu->pulse2);
	triangle_step_counter(&apu->triangle);
	noise_step_envelope(&apu->noise);
}

static inline void step_sweep(struct nes_apu_t * apu)
{
	pulse_step_sweep(&apu->pulse1);
	pulse_step_sweep(&apu->pulse2);
}

static inline void step_length(struct nes_apu_t * apu)
{
	pulse_step_length(&apu->pulse1);
	pulse_step_length(&apu->pulse2);
	triangle_step_length(&apu->triangle);
	noise_step_length(&apu->noise);
}

static inline void fire_irq(struct nes_apu_t * apu)
{
	if(apu->frame_irq)
		nes_cpu_trigger_irq(&apu->ctx->cpu);
}

static inline void step_frame_counter(struct nes_apu_t * apu)
{
	switch(apu->frame_period)
	{
	case 4:
		apu->frame_value = (apu->frame_value + 1) % 4;
		switch(apu->frame_value)
		{
		case 0:
		case 2:
			step_envelope(apu);
			break;
		case 1:
			step_envelope(apu);
			step_sweep(apu);
			step_length(apu);
			break;
		case 3:
			step_envelope(apu);
			step_sweep(apu);
			step_length(apu);
			fire_irq(apu);
			break;
		default:
			break;
		}
		break;
	case 5:
		apu->frame_value = (apu->frame_value + 1) % 5;
		switch(apu->frame_value)
		{
		case 0:
		case 2:
			step_envelope(apu);
			break;
		case 1:
		case 3:
			step_envelope(apu);
			step_sweep(apu);
			step_length(apu);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static inline void step_timer(struct nes_apu_t * apu)
{
	if(!(apu->cycles & 0x1))
	{
		pulse_step_timer(&apu->pulse1);
		pulse_step_timer(&apu->pulse2);
		noise_step_timer(&apu->noise);
		dmc_step_timer(&(apu->dmc));
	}
	triangle_step_timer(&apu->triangle);
}

static inline void write_control(struct nes_apu_t * apu, uint8_t value)
{
	apu->pulse1.enabled = (value & 0x1) ? 1 : 0;
	apu->pulse2.enabled = (value & 0x2) ? 1 : 0;
	apu->triangle.enabled = (value & 0x4) ? 1 : 0;
	apu->noise.enabled = (value & 0x8) ? 1 : 0;
	apu->dmc.enabled = (value & 0x10) ? 1 : 0;
	if(!apu->pulse1.enabled)
		apu->pulse1.length_value = 0;
	if(!apu->pulse2.enabled)
		apu->pulse2.length_value = 0;
	if(!apu->triangle.enabled)
		apu->triangle.length_value = 0;
	if(!apu->noise.enabled)
		apu->noise.length_value = 0;
	if(!apu->dmc.enabled)
		apu->dmc.current_length = 0;
	else
	{
		if(apu->dmc.current_length == 0)
			dmc_restart(&(apu->dmc));
	}
}

static inline void write_frame_counter(struct nes_apu_t * apu, uint8_t value)
{
	apu->frame_period = 4 + ((value & 0x80) ? 1 : 0);
	apu->frame_irq = (value & 0x40) ? 0 : 1;
	if(apu->frame_period == 5)
	{
		step_envelope(apu);
		step_sweep(apu);
		step_length(apu);
	}
}

static inline uint8_t read_status(struct nes_apu_t * apu)
{
	uint8_t val = 0;
	if(apu->pulse1.length_value > 0)
		val |= 0x1;
	if(apu->pulse2.length_value > 0)
		val |= 0x2;
	if(apu->triangle.length_value > 0)
		val |= 0x4;
	if(apu->noise.length_value > 0)
		val |= 0x8;
	if(apu->dmc.current_length > 0)
		val |= 0x10;
	return val;
}

void nes_apu_init(struct nes_apu_t * apu, struct nes_ctx_t * ctx)
{
	apu->ctx = ctx;
	nes_apu_reset(apu);
}

void nes_apu_reset(struct nes_apu_t * apu)
{
	nes_memset(&apu->pulse1, 0, sizeof(struct nes_apu_pulse_t));
	nes_memset(&apu->pulse2, 0, sizeof(struct nes_apu_pulse_t));
	nes_memset(&apu->triangle, 0, sizeof(struct nes_apu_triangle_t));
	nes_memset(&apu->noise, 0, sizeof(struct nes_apu_noise_t));
	nes_memset(&apu->dmc, 0, sizeof(struct nes_apu_dmc_t));
	apu->dmc.ctx = apu->ctx;
	apu->pulse1.channel = 1;
	apu->pulse2.channel = 2;
	apu->noise.shift_register = 1;
	apu->cycles = 0;
	apu->frame_period = 4;
	apu->frame_value = 0;
	apu->frame_irq = 0;
}



void nes_apu_step(struct nes_apu_t * apu)
{
	apu->cycles++;
	step_timer(apu);
	if((apu->cycles % (apu->ctx->cartridge->cpu_rate_adjusted / 240)) == 0)
		step_frame_counter(apu);
}

uint8_t nes_apu_read_register(struct nes_apu_t * apu, uint16_t addr)
{
	switch(addr)
	{
	case 0x4000: /* SQ1_VOL */
		break;
	case 0x4001: /* SQ1_SWEEP */
		break;
	case 0x4002: /* SQ1_LO */
		break;
	case 0x4003: /* SQ1_HI */
		break;
	case 0x4004: /* SQ2_VOL */
		break;
	case 0x4005: /* SQ2_SWEEP */
		break;
	case 0x4006: /* SQ2_LO */
		break;
	case 0x4007: /* SQ2_HI */
		break;
	case 0x4008: /* TRI_LINEAR */
		break;
	case 0x4009: /* UNUSED */
		break;
	case 0x400a: /* TRI_LO */
		break;
	case 0x400b: /* TRI_HI */
		break;
	case 0x400c: /* NOISE_VOL */
		break;
	case 0x400d: /* UNUSED */
		break;
	case 0x400e: /* NOISE_LO */
		break;
	case 0x400f: /* NOISE_HI */
		break;
	case 0x4015: /* SND_CHN */
		return read_status(apu);
	default:
		break;
	}
	return 0;
}

void nes_apu_write_register(struct nes_apu_t * apu, uint16_t addr, uint8_t val)
{
	switch(addr)
	{
	case 0x4000: /* SQ1_VOL */
		pulse_write_control(&apu->pulse1, val);
		break;
	case 0x4001: /* SQ1_SWEEP */
		pulse_write_sweep(&apu->pulse1, val);
		break;
	case 0x4002: /* SQ1_LO */
		pulse_write_timer_low(&apu->pulse1, val);
		break;
	case 0x4003: /* SQ1_HI */
		pulse_write_timer_high(&apu->pulse1, val);
		break;
	case 0x4004: /* SQ2_VOL */
		pulse_write_control(&apu->pulse2, val);
		break;
	case 0x4005: /* SQ2_SWEEP */
		pulse_write_sweep(&apu->pulse2, val);
		break;
	case 0x4006: /* SQ2_LO */
		pulse_write_timer_low(&apu->pulse2, val);
		break;
	case 0x4007: /* SQ2_HI */
		pulse_write_timer_high(&apu->pulse2, val);
		break;
	case 0x4008: /* TRI_LINEAR */
		triangle_write_control(&apu->triangle, val);
		break;
	case 0x4009: /* UNUSED */
		break;
	case 0x400a: /* TRI_LO */
		triangle_write_timer_low(&apu->triangle, val);
		break;
	case 0x400b: /* TRI_HI */
		triangle_write_timer_high(&apu->triangle, val);
		break;
	case 0x400c: /* NOISE_VOL */
		noise_write_control(&apu->noise, val);
		break;
	case 0x400d: /* UNUSED */
		break;
	case 0x400e: /* NOISE_LO */
		noise_write_period(&apu->noise, val);
		break;
	case 0x400f: /* NOISE_HI */
		noise_write_length(&apu->noise, val);
		break;
	case 0x4015: /* SND_CHN */
		write_control(apu, val);
		break;
	case 0x4017: /* APU_FRAME */
		write_frame_counter(apu, val);
		break;
	default:
		break;
	}
}
