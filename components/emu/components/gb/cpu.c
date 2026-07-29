#include "walnut_cgb.h"
uint8_t __gb_execute_cb(struct gb_s *gb)
{
	uint8_t inst_cycles;
	uint8_t cbop = __gb_read(gb, gb->cpu_reg.pc.reg++);
	uint8_t r = (cbop & 0x7);
	uint8_t b = (cbop >> 3) & 0x7;
	uint8_t d = (cbop >> 3) & 0x1;
	uint8_t val;
	uint8_t writeback = 1;

	inst_cycles = 8;
	/* Add an additional 8 cycles to these sets of instructions. */
	switch(cbop & 0xC7)
	{
	case 0x06:
	case 0x86:
    	case 0xC6:
		inst_cycles += 8;
    	break;
    	case 0x46:
		inst_cycles += 4;
    	break;
	}

	switch(r)
	{
	case 0:
		val = gb->cpu_reg.bc.bytes.b;
		break;

	case 1:
		val = gb->cpu_reg.bc.bytes.c;
		break;

	case 2:
		val = gb->cpu_reg.de.bytes.d;
		break;

	case 3:
		val = gb->cpu_reg.de.bytes.e;
		break;

	case 4:
		val = gb->cpu_reg.hl.bytes.h;
		break;

	case 5:
		val = gb->cpu_reg.hl.bytes.l;
		break;

	case 6:
		val = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	/* Only values 0-7 are possible here, so we make the final case
	 * default to satisfy -Wmaybe-uninitialized warning. */
	default:
		val = gb->cpu_reg.a;
		break;
	}

	switch(cbop >> 6)
	{
	case 0x0:
		cbop = (cbop >> 4) & 0x3;

		switch(cbop)
		{
		case 0x0: /* RdC R */
		case 0x1: /* Rd R */
			if(d) /* RRC R / RR R */
			{
				uint8_t temp = val;
				val = (val >> 1);
				val |= cbop ? (gb->cpu_reg.f.f_bits.c << 7) : (temp << 7);
				gb->cpu_reg.f.reg = 0;
				gb->cpu_reg.f.f_bits.z = (val == 0x00);
				gb->cpu_reg.f.f_bits.c = (temp & 0x01);
			}
			else /* RLC R / RL R */
			{
				uint8_t temp = val;
				val = (val << 1);
				val |= cbop ? gb->cpu_reg.f.f_bits.c : (temp >> 7);
				gb->cpu_reg.f.reg = 0;
				gb->cpu_reg.f.f_bits.z = (val == 0x00);
				gb->cpu_reg.f.f_bits.c = (temp >> 7);
			}

			break;

		case 0x2:
			if(d) /* SRA R */
			{
				gb->cpu_reg.f.reg = 0;
				gb->cpu_reg.f.f_bits.c = val & 0x01;
				val = (val >> 1) | (val & 0x80);
				gb->cpu_reg.f.f_bits.z = (val == 0x00);
			}
			else /* SLA R */
			{
				gb->cpu_reg.f.reg = 0;
				gb->cpu_reg.f.f_bits.c = (val >> 7);
				val = val << 1;
				gb->cpu_reg.f.f_bits.z = (val == 0x00);
			}

			break;

		case 0x3:
			if(d) /* SRL R */
			{
				gb->cpu_reg.f.reg = 0;
				gb->cpu_reg.f.f_bits.c = val & 0x01;
				val = val >> 1;
				gb->cpu_reg.f.f_bits.z = (val == 0x00);
			}
			else /* SWAP R */
			{
				uint8_t temp = (val >> 4) & 0x0F;
				temp |= (val << 4) & 0xF0;
				val = temp;
				gb->cpu_reg.f.reg = 0;
				gb->cpu_reg.f.f_bits.z = (val == 0x00);
			}

			break;
		}

		break;

	case 0x1: /* BIT B, R */
		gb->cpu_reg.f.f_bits.z = !((val >> b) & 0x1);
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h = 1;
		writeback = 0;
		break;

	case 0x2: /* RES B, R */
		val &= (0xFE << b) | (0xFF >> (8 - b));
		break;

	case 0x3: /* SET B, R */
		val |= (0x1 << b);
		break;
	}

	if(writeback)
	{
		switch(r)
		{
		case 0:
			gb->cpu_reg.bc.bytes.b = val;
			break;

		case 1:
			gb->cpu_reg.bc.bytes.c = val;
			break;

		case 2:
			gb->cpu_reg.de.bytes.d = val;
			break;

		case 3:
			gb->cpu_reg.de.bytes.e = val;
			break;

		case 4:
			gb->cpu_reg.hl.bytes.h = val;
			break;

		case 5:
			gb->cpu_reg.hl.bytes.l = val;
			break;

		case 6:
			__gb_write(gb, gb->cpu_reg.hl.reg, val);
			break;

		case 7:
			gb->cpu_reg.a = val;
			break;
		}
	}
	return inst_cycles;
}

#if ENABLE_LCD
struct sprite_data {
	uint8_t sprite_number;
	uint8_t x;
};

#if WALNUT_GB_HIGH_LCD_ACCURACY
static int compare_sprites(const struct sprite_data *const sd1, const struct sprite_data *const sd2)
{
	int x_res;

	x_res = (int)sd1->x - (int)sd2->x;
	if(x_res != 0)
		return x_res;

	return (int)sd1->sprite_number - (int)sd2->sprite_number;
}
#endif


#endif

void __gb_step_cpu_x(struct gb_s *gb)
{
	uint8_t opcode;
	uint_fast16_t inst_cycles;
	static const uint8_t op_cycles[0x100] =
	{
		/* *INDENT-OFF* */
		/*0 1 2  3  4  5  6  7  8  9  A  B  C  D  E  F	*/
		4,12, 8, 8, 4, 4, 8, 4,20, 8, 8, 8, 4, 4, 8, 4,	/* 0x00 */
		4,12, 8, 8, 4, 4, 8, 4,12, 8, 8, 8, 4, 4, 8, 4,	/* 0x10 */
		8,12, 8, 8, 4, 4, 8, 4, 8, 8, 8, 8, 4, 4, 8, 4,	/* 0x20 */
		8,12, 8, 8,12,12,12, 4, 8, 8, 8, 8, 4, 4, 8, 4,	/* 0x30 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x40 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x50 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x60 */
		8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, /* 0x70 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x80 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x90 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0xA0 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0xB0 */
		8,12,12,16,12,16, 8,16, 8,16,12, 8,12,24, 8,16,	/* 0xC0 */
		8,12,12, 0,12,16, 8,16, 8,16,12, 0,12, 0, 8,16,	/* 0xD0 */
		12,12,8, 0, 0,16, 8,16,16, 4,16, 0, 0, 0, 8,16,	/* 0xE0 */
		12,12,8, 4, 0,16, 8,16,12, 8,16, 4, 0, 0, 8,16	/* 0xF0 */
		/* *INDENT-ON* */
	};
	static const uint_fast16_t TAC_CYCLES[4] = {1024, 16, 64, 256};

	/* Handle interrupts */
	/* If gb_halt is positive, then an interrupt must have occurred by the
	 * time we reach here, because on HALT, we jump to the next interrupt
	 * immediately. */
	while(gb->gb_halt || (gb->gb_ime &&
			gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & ANY_INTR))
	{
		gb->gb_halt = false;

		if(!gb->gb_ime)
			break;

		/* Disable interrupts */
		gb->gb_ime = false;

		/* Push Program Counter */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);

		/* Call interrupt handler if required. */
		if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & VBLANK_INTR)
		{
			gb->cpu_reg.pc.reg = VBLANK_INTR_ADDR;
			gb->hram_io[IO_IF] ^= VBLANK_INTR;
		}
		else if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & LCDC_INTR)
		{
			gb->cpu_reg.pc.reg = LCDC_INTR_ADDR;
			gb->hram_io[IO_IF] ^= LCDC_INTR;
		}
		else if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & TIMER_INTR)
		{
			gb->cpu_reg.pc.reg = TIMER_INTR_ADDR;
			gb->hram_io[IO_IF] ^= TIMER_INTR;
		}
		else if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & SERIAL_INTR)
		{
			gb->cpu_reg.pc.reg = SERIAL_INTR_ADDR;
			gb->hram_io[IO_IF] ^= SERIAL_INTR;
		}
		else if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & CONTROL_INTR)
		{
			gb->cpu_reg.pc.reg = CONTROL_INTR_ADDR;
			gb->hram_io[IO_IF] ^= CONTROL_INTR;
		}

		break;
	}

	/* Obtain opcode */
	opcode = __gb_read(gb, gb->cpu_reg.pc.reg++);
	inst_cycles = op_cycles[opcode];

	/* Execute opcode */
	switch(opcode)
	{
	case 0x00: /* NOP */
		break;

	case 0x01: /* LD BC, imm */
#if WALNUT_GB_16_BIT_DISABLED
    gb->cpu_reg.bc.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2;
#else
    gb->cpu_reg.bc.bytes.c = __gb_read(gb, gb->cpu_reg.pc.reg++);
    gb->cpu_reg.bc.bytes.b = __gb_read(gb, gb->cpu_reg.pc.reg++);
#endif
		break;
	case 0x02: /* LD (BC), A */
		__gb_write(gb, gb->cpu_reg.bc.reg, gb->cpu_reg.a);
		break;

	case 0x03: /* INC BC */
		gb->cpu_reg.bc.reg++;
		break;

	case 0x04: /* INC B */
		WGB_INSTR_INC_R8(gb->cpu_reg.bc.bytes.b);
		break;

	case 0x05: /* DEC B */
		WGB_INSTR_DEC_R8(gb->cpu_reg.bc.bytes.b);
		break;

	case 0x06: /* LD B, imm */
		gb->cpu_reg.bc.bytes.b = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x07: /* RLCA */
		gb->cpu_reg.a = (gb->cpu_reg.a << 1) | (gb->cpu_reg.a >> 7);
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.c = (gb->cpu_reg.a & 0x01);
		break;

	case 0x08: /* LD (imm), SP */
	{		
#if WALNUT_GB_16_BIT_DISABLED
    uint16_t temp = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2;
#else
    uint8_t l = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint8_t h = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint16_t temp = WALNUT_GB_U8_TO_U16(h, l);
#endif
    __gb_write(gb, temp++, gb->cpu_reg.sp.bytes.p);
    __gb_write(gb, temp, gb->cpu_reg.sp.bytes.s);
    break;
	}

	case 0x09: /* ADD HL, BC */
	{
		uint_fast32_t temp = gb->cpu_reg.hl.reg + gb->cpu_reg.bc.reg;
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h =
			(temp ^ gb->cpu_reg.hl.reg ^ gb->cpu_reg.bc.reg) & 0x1000 ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = (temp & 0xFFFF0000) ? 1 : 0;
		gb->cpu_reg.hl.reg = (temp & 0x0000FFFF);
		break;
	}

	case 0x0A: /* LD A, (BC) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.bc.reg);
		break;

	case 0x0B: /* DEC BC */
		gb->cpu_reg.bc.reg--;
		break;

	case 0x0C: /* INC C */
		WGB_INSTR_INC_R8(gb->cpu_reg.bc.bytes.c);
		break;

	case 0x0D: /* DEC C */
		WGB_INSTR_DEC_R8(gb->cpu_reg.bc.bytes.c);
		break;

	case 0x0E: /* LD C, imm */
		gb->cpu_reg.bc.bytes.c = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x0F: /* RRCA */
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.c = gb->cpu_reg.a & 0x01;
		gb->cpu_reg.a = (gb->cpu_reg.a >> 1) | (gb->cpu_reg.a << 7);
		break;

	case 0x10: /* STOP */
		//gb->gb_halt = true;
#if WALNUT_FULL_GBC_SUPPORT
		if(gb->cgb.cgbMode & gb->cgb.doubleSpeedPrep)
		{
			gb->cgb.doubleSpeedPrep = 0;
			gb->cgb.doubleSpeed ^= 1;
		}
#endif
		break;

	case 0x11: /* LD DE, imm */
#if WALNUT_GB_16_BIT_DISABLED
    gb->cpu_reg.de.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2;
#else
    gb->cpu_reg.de.bytes.e = __gb_read(gb, gb->cpu_reg.pc.reg++);
    gb->cpu_reg.de.bytes.d = __gb_read(gb, gb->cpu_reg.pc.reg++);
#endif
		break;

	case 0x12: /* LD (DE), A */
		__gb_write(gb, gb->cpu_reg.de.reg, gb->cpu_reg.a);
		break;

	case 0x13: /* INC DE */
		gb->cpu_reg.de.reg++;
		break;

	case 0x14: /* INC D */
		WGB_INSTR_INC_R8(gb->cpu_reg.de.bytes.d);
		break;

	case 0x15: /* DEC D */
		WGB_INSTR_DEC_R8(gb->cpu_reg.de.bytes.d);
		break;

	case 0x16: /* LD D, imm */
		gb->cpu_reg.de.bytes.d = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x17: /* RLA */
	{
		uint8_t temp = gb->cpu_reg.a;
		gb->cpu_reg.a = (gb->cpu_reg.a << 1) | gb->cpu_reg.f.f_bits.c;
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.c = (temp >> 7) & 0x01;
		break;
	}

	case 0x18: /* JR imm */
	{
		int8_t temp = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
		gb->cpu_reg.pc.reg += temp;
		break;
	}

	case 0x19: /* ADD HL, DE */
	{
		uint_fast32_t temp = gb->cpu_reg.hl.reg + gb->cpu_reg.de.reg;
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h =
			(temp ^ gb->cpu_reg.hl.reg ^ gb->cpu_reg.de.reg) & 0x1000 ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = (temp & 0xFFFF0000) ? 1 : 0;
		gb->cpu_reg.hl.reg = (temp & 0x0000FFFF);
		break;
	}

	case 0x1A: /* LD A, (DE) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.de.reg);
		break;

	case 0x1B: /* DEC DE */
		gb->cpu_reg.de.reg--;
		break;

	case 0x1C: /* INC E */
		WGB_INSTR_INC_R8(gb->cpu_reg.de.bytes.e);
		break;

	case 0x1D: /* DEC E */
		WGB_INSTR_DEC_R8(gb->cpu_reg.de.bytes.e);
		break;

	case 0x1E: /* LD E, imm */
		gb->cpu_reg.de.bytes.e = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x1F: /* RRA */
	{
		uint8_t temp = gb->cpu_reg.a;
		gb->cpu_reg.a = gb->cpu_reg.a >> 1 | (gb->cpu_reg.f.f_bits.c << 7);
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.c = temp & 0x1;
		break;
	}

	case 0x20: /* JR NZ, imm */
		if(!gb->cpu_reg.f.f_bits.z)
		{
			int8_t temp = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
			gb->cpu_reg.pc.reg += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg++;

		break;

	case 0x21: /* LD HL, imm */
#if WALNUT_GB_16_BIT_DISABLED
    gb->cpu_reg.hl.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2;
#else
    gb->cpu_reg.hl.bytes.l = __gb_read(gb, gb->cpu_reg.pc.reg++);
    gb->cpu_reg.hl.bytes.h = __gb_read(gb, gb->cpu_reg.pc.reg++);
#endif
		break;

	case 0x22: /* LDI (HL), A */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.a);
		gb->cpu_reg.hl.reg++;
		break;

	case 0x23: /* INC HL */
		gb->cpu_reg.hl.reg++;
		break;

	case 0x24: /* INC H */
		WGB_INSTR_INC_R8(gb->cpu_reg.hl.bytes.h);
		break;

	case 0x25: /* DEC H */
		WGB_INSTR_DEC_R8(gb->cpu_reg.hl.bytes.h);
		break;

	case 0x26: /* LD H, imm */
		gb->cpu_reg.hl.bytes.h = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x27: /* DAA */
	{
		/* The following is from SameBoy. MIT License. */
		int16_t a = gb->cpu_reg.a;

		if(gb->cpu_reg.f.f_bits.n)
		{
			if(gb->cpu_reg.f.f_bits.h)
				a = (a - 0x06) & 0xFF;

			if(gb->cpu_reg.f.f_bits.c)
				a -= 0x60;
		}
		else
		{
			if(gb->cpu_reg.f.f_bits.h || (a & 0x0F) > 9)
				a += 0x06;

			if(gb->cpu_reg.f.f_bits.c || a > 0x9F)
				a += 0x60;
		}

		if((a & 0x100) == 0x100)
			gb->cpu_reg.f.f_bits.c = 1;

		gb->cpu_reg.a = (uint8_t)a;
		gb->cpu_reg.f.f_bits.z = (gb->cpu_reg.a == 0);
		gb->cpu_reg.f.f_bits.h = 0;

		break;
	}

	case 0x28: /* JR Z, imm */
		if(gb->cpu_reg.f.f_bits.z)
		{
			int8_t temp = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
			gb->cpu_reg.pc.reg += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg++;

		break;

	case 0x29: /* ADD HL, HL */
	{
		gb->cpu_reg.f.f_bits.c = (gb->cpu_reg.hl.reg & 0x8000) > 0;
		gb->cpu_reg.hl.reg <<= 1;
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h = (gb->cpu_reg.hl.reg & 0x1000) > 0;
		break;
	}

	case 0x2A: /* LD A, (HL+) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.hl.reg++);
		break;

	case 0x2B: /* DEC HL */
		gb->cpu_reg.hl.reg--;
		break;

	case 0x2C: /* INC L */
		WGB_INSTR_INC_R8(gb->cpu_reg.hl.bytes.l);
		break;

	case 0x2D: /* DEC L */
		WGB_INSTR_DEC_R8(gb->cpu_reg.hl.bytes.l);
		break;

	case 0x2E: /* LD L, imm */
		gb->cpu_reg.hl.bytes.l = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x2F: /* CPL */
		gb->cpu_reg.a = ~gb->cpu_reg.a;
		gb->cpu_reg.f.f_bits.n = 1;
		gb->cpu_reg.f.f_bits.h = 1;
		break;

	case 0x30: /* JR NC, imm */
		if(!gb->cpu_reg.f.f_bits.c)
		{
			int8_t temp = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
			gb->cpu_reg.pc.reg += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg++;

		break;

	case 0x31: /* LD SP, imm */
#if WALNUT_GB_16_BIT_DISABLED
    gb->cpu_reg.sp.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2;
#else
    gb->cpu_reg.sp.bytes.p = __gb_read(gb, gb->cpu_reg.pc.reg++);
    gb->cpu_reg.sp.bytes.s = __gb_read(gb, gb->cpu_reg.pc.reg++);
#endif
		break;

	case 0x32: /* LD (HL), A */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.a);
		gb->cpu_reg.hl.reg--;
		break;

	case 0x33: /* INC SP */
		gb->cpu_reg.sp.reg++;
		break;

	case 0x34: /* INC (HL) */
	{
		uint8_t temp = __gb_read(gb, gb->cpu_reg.hl.reg);
		WGB_INSTR_INC_R8(temp);
		__gb_write(gb, gb->cpu_reg.hl.reg, temp);
		break;
	}

	case 0x35: /* DEC (HL) */
	{
		uint8_t temp = __gb_read(gb, gb->cpu_reg.hl.reg);
		WGB_INSTR_DEC_R8(temp);
		__gb_write(gb, gb->cpu_reg.hl.reg, temp);
		break;
	}

	case 0x36: /* LD (HL), imm */
		__gb_write(gb, gb->cpu_reg.hl.reg, __gb_read(gb, gb->cpu_reg.pc.reg++));
		break;

	case 0x37: /* SCF */
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h = 0;
		gb->cpu_reg.f.f_bits.c = 1;
		break;

	case 0x38: /* JR C, imm */
		if(gb->cpu_reg.f.f_bits.c)
		{
			int8_t temp = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
			gb->cpu_reg.pc.reg += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg++;

		break;

	case 0x39: /* ADD HL, SP */
	{
		uint_fast32_t temp = gb->cpu_reg.hl.reg + gb->cpu_reg.sp.reg;
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h =
			((gb->cpu_reg.hl.reg & 0xFFF) + (gb->cpu_reg.sp.reg & 0xFFF)) & 0x1000 ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = temp & 0x10000 ? 1 : 0;
		gb->cpu_reg.hl.reg = (uint16_t)temp;
		break;
	}

	case 0x3A: /* LD A, (HL) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.hl.reg--);
		break;

	case 0x3B: /* DEC SP */
		gb->cpu_reg.sp.reg--;
		break;

	case 0x3C: /* INC A */
		WGB_INSTR_INC_R8(gb->cpu_reg.a);
		break;

	case 0x3D: /* DEC A */
		WGB_INSTR_DEC_R8(gb->cpu_reg.a);
		break;

	case 0x3E: /* LD A, imm */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x3F: /* CCF */
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h = 0;
		gb->cpu_reg.f.f_bits.c = ~gb->cpu_reg.f.f_bits.c;
		break;

	case 0x40: /* LD B, B */
		break;

	case 0x41: /* LD B, C */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.bc.bytes.c;
		break;

	case 0x42: /* LD B, D */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.de.bytes.d;
		break;

	case 0x43: /* LD B, E */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.de.bytes.e;
		break;

	case 0x44: /* LD B, H */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.hl.bytes.h;
		break;

	case 0x45: /* LD B, L */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.hl.bytes.l;
		break;

	case 0x46: /* LD B, (HL) */
		gb->cpu_reg.bc.bytes.b = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x47: /* LD B, A */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.a;
		break;

	case 0x48: /* LD C, B */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.bc.bytes.b;
		break;

	case 0x49: /* LD C, C */
		break;

	case 0x4A: /* LD C, D */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.de.bytes.d;
		break;

	case 0x4B: /* LD C, E */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.de.bytes.e;
		break;

	case 0x4C: /* LD C, H */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.hl.bytes.h;
		break;

	case 0x4D: /* LD C, L */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.hl.bytes.l;
		break;

	case 0x4E: /* LD C, (HL) */
		gb->cpu_reg.bc.bytes.c = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x4F: /* LD C, A */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.a;
		break;

	case 0x50: /* LD D, B */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.bc.bytes.b;
		break;

	case 0x51: /* LD D, C */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.bc.bytes.c;
		break;

	case 0x52: /* LD D, D */
		break;

	case 0x53: /* LD D, E */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.de.bytes.e;
		break;

	case 0x54: /* LD D, H */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.hl.bytes.h;
		break;

	case 0x55: /* LD D, L */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.hl.bytes.l;
		break;

	case 0x56: /* LD D, (HL) */
		gb->cpu_reg.de.bytes.d = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x57: /* LD D, A */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.a;
		break;

	case 0x58: /* LD E, B */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.bc.bytes.b;
		break;

	case 0x59: /* LD E, C */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.bc.bytes.c;
		break;

	case 0x5A: /* LD E, D */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.de.bytes.d;
		break;

	case 0x5B: /* LD E, E */
		break;

	case 0x5C: /* LD E, H */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.hl.bytes.h;
		break;

	case 0x5D: /* LD E, L */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.hl.bytes.l;
		break;

	case 0x5E: /* LD E, (HL) */
		gb->cpu_reg.de.bytes.e = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x5F: /* LD E, A */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.a;
		break;

	case 0x60: /* LD H, B */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.bc.bytes.b;
		break;

	case 0x61: /* LD H, C */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.bc.bytes.c;
		break;

	case 0x62: /* LD H, D */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.de.bytes.d;
		break;

	case 0x63: /* LD H, E */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.de.bytes.e;
		break;

	case 0x64: /* LD H, H */
		break;

	case 0x65: /* LD H, L */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.hl.bytes.l;
		break;

	case 0x66: /* LD H, (HL) */
		gb->cpu_reg.hl.bytes.h = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x67: /* LD H, A */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.a;
		break;

	case 0x68: /* LD L, B */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.bc.bytes.b;
		break;

	case 0x69: /* LD L, C */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.bc.bytes.c;
		break;

	case 0x6A: /* LD L, D */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.de.bytes.d;
		break;

	case 0x6B: /* LD L, E */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.de.bytes.e;
		break;

	case 0x6C: /* LD L, H */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.hl.bytes.h;
		break;

	case 0x6D: /* LD L, L */
		break;

	case 0x6E: /* LD L, (HL) */
		gb->cpu_reg.hl.bytes.l = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x6F: /* LD L, A */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.a;
		break;

	case 0x70: /* LD (HL), B */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.bc.bytes.b);
		break;

	case 0x71: /* LD (HL), C */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.bc.bytes.c);
		break;

	case 0x72: /* LD (HL), D */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.de.bytes.d);
		break;

	case 0x73: /* LD (HL), E */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.de.bytes.e);
		break;

	case 0x74: /* LD (HL), H */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.hl.bytes.h);
		break;

	case 0x75: /* LD (HL), L */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.hl.bytes.l);
		break;

	case 0x76: /* HALT */
	{
		int_fast16_t halt_cycles = INT_FAST16_MAX;

		/* TODO: Emulate HALT bug? */
		gb->gb_halt = true;

		if(gb->hram_io[IO_SC] & SERIAL_SC_TX_START)
		{
			int serial_cycles = SERIAL_CYCLES -
				gb->counter.serial_count;

			if(serial_cycles < halt_cycles)
				halt_cycles = serial_cycles;
		}

		if(gb->hram_io[IO_TAC] & IO_TAC_ENABLE_MASK)
		{
			int tac_cycles = TAC_CYCLES[gb->hram_io[IO_TAC] & IO_TAC_RATE_MASK] -
				gb->counter.tima_count;

			if(tac_cycles < halt_cycles)
				halt_cycles = tac_cycles;
		}

		if((gb->hram_io[IO_LCDC] & LCDC_ENABLE))
		{
			int lcd_cycles;

			/* If LCD is in HBlank, calculate the number of cycles
			 * until the end of HBlank and the start of mode 2 or
			 * mode 1. */
			if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_HBLANK)
			{
				lcd_cycles = LCD_MODE0_HBLANK_MAX_DRUATION - gb->counter.lcd_count;
			}
			else if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_OAM_SCAN)
			{
				lcd_cycles = LCD_MODE3_LCD_DRAW_MIN_DURATION - gb->counter.lcd_count;
			}
			else if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_LCD_DRAW)
			{
				lcd_cycles = LCD_MODE0_HBLANK_MAX_DRUATION - gb->counter.lcd_count;
			}
			else
			{
				/* VBlank */
				lcd_cycles = LCD_LINE_CYCLES - gb->counter.lcd_count;
			}

			if(lcd_cycles < halt_cycles)
				halt_cycles = lcd_cycles;
		}

		/* Some halt cycles may already be very high, so make sure we
		 * don't underflow here. */
		if(halt_cycles <= 0)
			halt_cycles = 4;

		inst_cycles = (uint_fast16_t)halt_cycles;
		break;
	}

	case 0x77: /* LD (HL), A */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.a);
		break;

	case 0x78: /* LD A, B */
		gb->cpu_reg.a = gb->cpu_reg.bc.bytes.b;
		break;

	case 0x79: /* LD A, C */
		gb->cpu_reg.a = gb->cpu_reg.bc.bytes.c;
		break;

	case 0x7A: /* LD A, D */
		gb->cpu_reg.a = gb->cpu_reg.de.bytes.d;
		break;

	case 0x7B: /* LD A, E */
		gb->cpu_reg.a = gb->cpu_reg.de.bytes.e;
		break;

	case 0x7C: /* LD A, H */
		gb->cpu_reg.a = gb->cpu_reg.hl.bytes.h;
		break;

	case 0x7D: /* LD A, L */
		gb->cpu_reg.a = gb->cpu_reg.hl.bytes.l;
		break;

	case 0x7E: /* LD A, (HL) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x7F: /* LD A, A */
		break;

	case 0x80: /* ADD A, B */
		WGB_INSTR_ADC_R8(gb->cpu_reg.bc.bytes.b, 0);
		break;

	case 0x81: /* ADD A, C */
		WGB_INSTR_ADC_R8(gb->cpu_reg.bc.bytes.c, 0);
		break;

	case 0x82: /* ADD A, D */
		WGB_INSTR_ADC_R8(gb->cpu_reg.de.bytes.d, 0);
		break;

	case 0x83: /* ADD A, E */
		WGB_INSTR_ADC_R8(gb->cpu_reg.de.bytes.e, 0);
		break;

	case 0x84: /* ADD A, H */
		WGB_INSTR_ADC_R8(gb->cpu_reg.hl.bytes.h, 0);
		break;

	case 0x85: /* ADD A, L */
		WGB_INSTR_ADC_R8(gb->cpu_reg.hl.bytes.l, 0);
		break;

	case 0x86: /* ADD A, (HL) */
		WGB_INSTR_ADC_R8(__gb_read(gb, gb->cpu_reg.hl.reg), 0);
		break;

	case 0x87: /* ADD A, A */
		WGB_INSTR_ADC_R8(gb->cpu_reg.a, 0);
		break;

	case 0x88: /* ADC A, B */
		WGB_INSTR_ADC_R8(gb->cpu_reg.bc.bytes.b, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x89: /* ADC A, C */
		WGB_INSTR_ADC_R8(gb->cpu_reg.bc.bytes.c, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x8A: /* ADC A, D */
		WGB_INSTR_ADC_R8(gb->cpu_reg.de.bytes.d, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x8B: /* ADC A, E */
		WGB_INSTR_ADC_R8(gb->cpu_reg.de.bytes.e, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x8C: /* ADC A, H */
		WGB_INSTR_ADC_R8(gb->cpu_reg.hl.bytes.h, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x8D: /* ADC A, L */
		WGB_INSTR_ADC_R8(gb->cpu_reg.hl.bytes.l, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x8E: /* ADC A, (HL) */
		WGB_INSTR_ADC_R8(__gb_read(gb, gb->cpu_reg.hl.reg), gb->cpu_reg.f.f_bits.c);
		break;

	case 0x8F: /* ADC A, A */
		WGB_INSTR_ADC_R8(gb->cpu_reg.a, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x90: /* SUB B */
		WGB_INSTR_SBC_R8(gb->cpu_reg.bc.bytes.b, 0);
		break;

	case 0x91: /* SUB C */
		WGB_INSTR_SBC_R8(gb->cpu_reg.bc.bytes.c, 0);
		break;

	case 0x92: /* SUB D */
		WGB_INSTR_SBC_R8(gb->cpu_reg.de.bytes.d, 0);
		break;

	case 0x93: /* SUB E */
		WGB_INSTR_SBC_R8(gb->cpu_reg.de.bytes.e, 0);
		break;

	case 0x94: /* SUB H */
		WGB_INSTR_SBC_R8(gb->cpu_reg.hl.bytes.h, 0);
		break;

	case 0x95: /* SUB L */
		WGB_INSTR_SBC_R8(gb->cpu_reg.hl.bytes.l, 0);
		break;

	case 0x96: /* SUB (HL) */
		WGB_INSTR_SBC_R8(__gb_read(gb, gb->cpu_reg.hl.reg), 0);
		break;

	case 0x97: /* SUB A */
		gb->cpu_reg.a = 0;
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.z = 1;
		gb->cpu_reg.f.f_bits.n = 1;
		break;

	case 0x98: /* SBC A, B */
		WGB_INSTR_SBC_R8(gb->cpu_reg.bc.bytes.b, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x99: /* SBC A, C */
		WGB_INSTR_SBC_R8(gb->cpu_reg.bc.bytes.c, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x9A: /* SBC A, D */
		WGB_INSTR_SBC_R8(gb->cpu_reg.de.bytes.d, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x9B: /* SBC A, E */
		WGB_INSTR_SBC_R8(gb->cpu_reg.de.bytes.e, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x9C: /* SBC A, H */
		WGB_INSTR_SBC_R8(gb->cpu_reg.hl.bytes.h, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x9D: /* SBC A, L */
		WGB_INSTR_SBC_R8(gb->cpu_reg.hl.bytes.l, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x9E: /* SBC A, (HL) */
		WGB_INSTR_SBC_R8(__gb_read(gb, gb->cpu_reg.hl.reg), gb->cpu_reg.f.f_bits.c);
		break;

	case 0x9F: /* SBC A, A */
		gb->cpu_reg.a = gb->cpu_reg.f.f_bits.c ? 0xFF : 0x00;
		gb->cpu_reg.f.f_bits.z = !gb->cpu_reg.f.f_bits.c;
		gb->cpu_reg.f.f_bits.n = 1;
		gb->cpu_reg.f.f_bits.h = gb->cpu_reg.f.f_bits.c;
		break;

	case 0xA0: /* AND B */
		WGB_INSTR_AND_R8(gb->cpu_reg.bc.bytes.b);
		break;

	case 0xA1: /* AND C */
		WGB_INSTR_AND_R8(gb->cpu_reg.bc.bytes.c);
		break;

	case 0xA2: /* AND D */
		WGB_INSTR_AND_R8(gb->cpu_reg.de.bytes.d);
		break;

	case 0xA3: /* AND E */
		WGB_INSTR_AND_R8(gb->cpu_reg.de.bytes.e);
		break;

	case 0xA4: /* AND H */
		WGB_INSTR_AND_R8(gb->cpu_reg.hl.bytes.h);
		break;

	case 0xA5: /* AND L */
		WGB_INSTR_AND_R8(gb->cpu_reg.hl.bytes.l);
		break;

	case 0xA6: /* AND (HL) */
		WGB_INSTR_AND_R8(__gb_read(gb, gb->cpu_reg.hl.reg));
		break;

	case 0xA7: /* AND A */
		WGB_INSTR_AND_R8(gb->cpu_reg.a);
		break;

	case 0xA8: /* XOR B */
		WGB_INSTR_XOR_R8(gb->cpu_reg.bc.bytes.b);
		break;

	case 0xA9: /* XOR C */
		WGB_INSTR_XOR_R8(gb->cpu_reg.bc.bytes.c);
		break;

	case 0xAA: /* XOR D */
		WGB_INSTR_XOR_R8(gb->cpu_reg.de.bytes.d);
		break;

	case 0xAB: /* XOR E */
		WGB_INSTR_XOR_R8(gb->cpu_reg.de.bytes.e);
		break;

	case 0xAC: /* XOR H */
		WGB_INSTR_XOR_R8(gb->cpu_reg.hl.bytes.h);
		break;

	case 0xAD: /* XOR L */
		WGB_INSTR_XOR_R8(gb->cpu_reg.hl.bytes.l);
		break;

	case 0xAE: /* XOR (HL) */
		WGB_INSTR_XOR_R8(__gb_read(gb, gb->cpu_reg.hl.reg));
		break;

	case 0xAF: /* XOR A */
		WGB_INSTR_XOR_R8(gb->cpu_reg.a);
		break;

	case 0xB0: /* OR B */
		WGB_INSTR_OR_R8(gb->cpu_reg.bc.bytes.b);
		break;

	case 0xB1: /* OR C */
		WGB_INSTR_OR_R8(gb->cpu_reg.bc.bytes.c);
		break;

	case 0xB2: /* OR D */
		WGB_INSTR_OR_R8(gb->cpu_reg.de.bytes.d);
		break;

	case 0xB3: /* OR E */
		WGB_INSTR_OR_R8(gb->cpu_reg.de.bytes.e);
		break;

	case 0xB4: /* OR H */
		WGB_INSTR_OR_R8(gb->cpu_reg.hl.bytes.h);
		break;

	case 0xB5: /* OR L */
		WGB_INSTR_OR_R8(gb->cpu_reg.hl.bytes.l);
		break;

	case 0xB6: /* OR (HL) */
		WGB_INSTR_OR_R8(__gb_read(gb, gb->cpu_reg.hl.reg));
		break;

	case 0xB7: /* OR A */
		WGB_INSTR_OR_R8(gb->cpu_reg.a);
		break;

	case 0xB8: /* CP B */
		WGB_INSTR_CP_R8(gb->cpu_reg.bc.bytes.b);
		break;

	case 0xB9: /* CP C */
		WGB_INSTR_CP_R8(gb->cpu_reg.bc.bytes.c);
		break;

	case 0xBA: /* CP D */
		WGB_INSTR_CP_R8(gb->cpu_reg.de.bytes.d);
		break;

	case 0xBB: /* CP E */
		WGB_INSTR_CP_R8(gb->cpu_reg.de.bytes.e);
		break;

	case 0xBC: /* CP H */
		WGB_INSTR_CP_R8(gb->cpu_reg.hl.bytes.h);
		break;

	case 0xBD: /* CP L */
		WGB_INSTR_CP_R8(gb->cpu_reg.hl.bytes.l);
		break;

	case 0xBE: /* CP (HL) */
		WGB_INSTR_CP_R8(__gb_read(gb, gb->cpu_reg.hl.reg));
		break;

	case 0xBF: /* CP A */
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.z = 1;
		gb->cpu_reg.f.f_bits.n = 1;
		break;

	case 0xC0: /* RET NZ */
		if(!gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_DISABLED
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
        gb->cpu_reg.sp.reg += 2;
#else
        gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
        gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
        inst_cycles += 12;
		}

		break;

	case 0xC1: /* POP BC */
#if WALNUT_GB_16_BIT_DISABLED
    gb->cpu_reg.bc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2;
#else
    gb->cpu_reg.bc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.bc.bytes.b = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		break;

	case 0xC2: /* JP NZ, imm */
		if(!gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_DISABLED
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xC3: /* JP imm */
#if WALNUT_GB_16_BIT_DISABLED
    gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
#else
{
    uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
    gb->cpu_reg.pc.bytes.c = c;
    gb->cpu_reg.pc.bytes.p = p;
}
#endif
		break;

	case 0xC4: /* CALL NZ imm */
		if(!gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_DISABLED
			uint16_t addr = __gb_read16(gb, gb->cpu_reg.pc.reg);
			gb->cpu_reg.pc.reg += 2;

			__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
			__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
			gb->cpu_reg.pc.reg = addr;
#else
			uint8_t c, p;
			c = __gb_read(gb, gb->cpu_reg.pc.reg++);
			p = __gb_read(gb, gb->cpu_reg.pc.reg++);
			__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
			__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
			gb->cpu_reg.pc.bytes.c = c;
			gb->cpu_reg.pc.bytes.p = p;
#endif        
			inst_cycles += 12;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xC5: /* PUSH BC */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.bc.bytes.b);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.bc.bytes.c);
		break;

	case 0xC6: /* ADD A, imm */
	{
		uint8_t val = __gb_read(gb, gb->cpu_reg.pc.reg++);
		WGB_INSTR_ADC_R8(val, 0);
		break;
	}

	case 0xC7: /* RST 0x0000 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0000;
		break;

	case 0xC8: /* RET Z */
		if(gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_DISABLED
        // Optional 16-bit path if stack is aligned
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
        gb->cpu_reg.sp.reg += 2;
#else
        gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
        gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
			inst_cycles += 12;
		}
		break;

	case 0xC9: /* RET */
	{
#if WALNUT_GB_16_BIT_DISABLED
    gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2;
#else
    gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		break;
	}

	case 0xCA: /* JP Z, imm */
		if(gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_DISABLED
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
        gb->cpu_reg.pc.reg += 2;  // advance past the immediate word
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xCB: /* CB INST */
		inst_cycles = __gb_execute_cb(gb);
		break;

	case 0xCC: /* CALL Z, imm */
		if(gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_DISABLED
        uint16_t addr = __gb_read16(gb, gb->cpu_reg.pc.reg);
        gb->cpu_reg.pc.reg += 2; // advance past immediate

        // push old PC
        __gb_write(gb, --gb->cpu_reg.sp.reg, (gb->cpu_reg.pc.reg >> 8) & 0xFF);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.reg & 0xFF);

        gb->cpu_reg.pc.reg = addr;
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 12;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xCD: /* CALL imm */
#if WALNUT_GB_16_BIT_DISABLED
{
    uint16_t addr = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2; // advance past immediate

    // push old PC
    __gb_write(gb, --gb->cpu_reg.sp.reg, (gb->cpu_reg.pc.reg >> 8) & 0xFF);
    __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.reg & 0xFF);

    gb->cpu_reg.pc.reg = addr;
}
#else
{
    uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
    __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
    __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
    gb->cpu_reg.pc.bytes.c = c;
    gb->cpu_reg.pc.bytes.p = p;
}
#endif
	break;

	case 0xCE: /* ADC A, imm */
	{
		uint8_t val = __gb_read(gb, gb->cpu_reg.pc.reg++);
		WGB_INSTR_ADC_R8(val, gb->cpu_reg.f.f_bits.c);
		break;
	}

	case 0xCF: /* RST 0x0008 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0008;
		break;

	case 0xD0: /* RET NC */

    if (!gb->cpu_reg.f.f_bits.c)
    {
#if WALNUT_GB_16_BIT_DISABLED
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
        gb->cpu_reg.sp.reg += 2; // advance SP past the popped PC
#else
        gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
        gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
        inst_cycles += 12;
    }

		break;

	case 0xD1: /* POP DE */
#if WALNUT_GB_16_BIT_DISABLED
    gb->cpu_reg.de.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2;
#else
    gb->cpu_reg.de.bytes.e = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.de.bytes.d = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		break;

	case 0xD2: /* JP NC, imm */
		if(!gb->cpu_reg.f.f_bits.c)
		{
#if WALNUT_GB_16_BIT_DISABLED
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
        gb->cpu_reg.pc.reg += 2; // advance PC past immediate
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xD4: /* CALL NC, imm */
		if(!gb->cpu_reg.f.f_bits.c)
		{
#if WALNUT_GB_16_BIT_DISABLED
        uint16_t addr = __gb_read16(gb, gb->cpu_reg.pc.reg);
        gb->cpu_reg.pc.reg += 2; // advance PC past immediate
        __gb_write(gb, --gb->cpu_reg.sp.reg, (addr >> 8) & 0xFF);
        __gb_write(gb, --gb->cpu_reg.sp.reg, addr & 0xFF);
        gb->cpu_reg.pc.reg = addr;
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 12;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xD5: /* PUSH DE */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.de.bytes.d);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.de.bytes.e);
		break;

	case 0xD6: /* SUB imm */
	{
		uint8_t val = __gb_read(gb, gb->cpu_reg.pc.reg++);
		uint16_t temp = gb->cpu_reg.a - val;
		gb->cpu_reg.f.f_bits.z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.f.f_bits.n = 1;
		gb->cpu_reg.f.f_bits.h =
			(gb->cpu_reg.a ^ val ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0xD7: /* RST 0x0010 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0010;
		break;

	case 0xD8: /* RET C */
		if(gb->cpu_reg.f.f_bits.c)
		{
#if WALNUT_GB_16_BIT_DISABLED
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
        gb->cpu_reg.sp.reg += 2; // advance SP past the popped PC
#else
        gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
        gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
			inst_cycles += 12;
		}

		break;

	case 0xD9: /* RETI */
	{
#if WALNUT_GB_16_BIT_DISABLED
    gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2; // advance SP past the popped PC
#else
    gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		gb->gb_ime = true;
	}
	break;

	case 0xDA: /* JP C, imm */
		if(gb->cpu_reg.f.f_bits.c)
		{
#if WALNUT_GB_16_BIT_DISABLED
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
        gb->cpu_reg.pc.reg += 2; // advance PC past the immediate word
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xDC: /* CALL C, imm */
		if(gb->cpu_reg.f.f_bits.c)
		{
#if WALNUT_GB_16_BIT_DISABLED
        uint16_t target = __gb_read16(gb, gb->cpu_reg.pc.reg);
        gb->cpu_reg.pc.reg += 2; // advance PC past the immediate word
        // push current PC onto stack
        gb->cpu_reg.sp.reg -= 2;
        __gb_write16(gb, gb->cpu_reg.sp.reg, gb->cpu_reg.pc.reg);
        gb->cpu_reg.pc.reg = target;
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 12;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xDE: /* SBC A, imm */
	{
		uint8_t val = __gb_read(gb, gb->cpu_reg.pc.reg++);
		WGB_INSTR_SBC_R8(val, gb->cpu_reg.f.f_bits.c);
		break;
	}

	case 0xDF: /* RST 0x0018 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0018;
		break;

	case 0xE0: /* LD (0xFF00+imm), A */
		__gb_write(gb, 0xFF00 | __gb_read(gb, gb->cpu_reg.pc.reg++),
			   gb->cpu_reg.a);
		break;

	case 0xE1: /* POP HL */
#if WALNUT_GB_16_BIT_DISABLED
    gb->cpu_reg.hl.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2; // advance SP past the popped value
#else
    gb->cpu_reg.hl.bytes.l = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.hl.bytes.h = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		break;

	case 0xE2: /* LD (C), A */
		__gb_write(gb, 0xFF00 | gb->cpu_reg.bc.bytes.c, gb->cpu_reg.a);
		break;

	case 0xE5: /* PUSH HL */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.hl.bytes.h);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.hl.bytes.l);
		break;

	case 0xE6: /* AND imm */
	{
		uint8_t temp = __gb_read(gb, gb->cpu_reg.pc.reg++);
		WGB_INSTR_AND_R8(temp);
		break;
	}

	case 0xE7: /* RST 0x0020 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0020;
		break;

	case 0xE8: /* ADD SP, imm */
	{
		int8_t offset = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.h = ((gb->cpu_reg.sp.reg & 0xF) + (offset & 0xF) > 0xF) ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = ((gb->cpu_reg.sp.reg & 0xFF) + (offset & 0xFF) > 0xFF);
		gb->cpu_reg.sp.reg += offset;
		break;
	}

	case 0xE9: /* JP (HL) */
		gb->cpu_reg.pc.reg = gb->cpu_reg.hl.reg;
		break;

	case 0xEA: /* LD (imm), A */
	{
#if WALNUT_GB_16_BIT_DISABLED
    uint16_t addr = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2; // advance past the immediate
#else
    uint8_t l = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint8_t h = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint16_t addr = WALNUT_GB_U8_TO_U16(h, l);
#endif
		__gb_write(gb, addr, gb->cpu_reg.a);
		break;
	}

	case 0xEE: /* XOR imm */
		WGB_INSTR_XOR_R8(__gb_read(gb, gb->cpu_reg.pc.reg++));
		break;

	case 0xEF: /* RST 0x0028 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0028;
		break;

	case 0xF0: /* LD A, (0xFF00+imm) */
		gb->cpu_reg.a =
			__gb_read(gb, 0xFF00 | __gb_read(gb, gb->cpu_reg.pc.reg++));
		break;

	case 0xF1: /* POP AF */
	{
		uint8_t temp_8 = __gb_read(gb, gb->cpu_reg.sp.reg++);
		gb->cpu_reg.f.f_bits.z = (temp_8 >> 7) & 1;
		gb->cpu_reg.f.f_bits.n = (temp_8 >> 6) & 1;
		gb->cpu_reg.f.f_bits.h = (temp_8 >> 5) & 1;
		gb->cpu_reg.f.f_bits.c = (temp_8 >> 4) & 1;
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.sp.reg++);
		break;
	}

	case 0xF2: /* LD A, (C) */
		gb->cpu_reg.a = __gb_read(gb, 0xFF00 | gb->cpu_reg.bc.bytes.c);
		break;

	case 0xF3: /* DI */
		gb->gb_ime = false;
		break;

	case 0xF5: /* PUSH AF */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.a);
		__gb_write(gb, --gb->cpu_reg.sp.reg,
			   gb->cpu_reg.f.f_bits.z << 7 | gb->cpu_reg.f.f_bits.n << 6 |
			   gb->cpu_reg.f.f_bits.h << 5 | gb->cpu_reg.f.f_bits.c << 4);
		break;

	case 0xF6: /* OR imm */
		WGB_INSTR_OR_R8(__gb_read(gb, gb->cpu_reg.pc.reg++));
		break;

	case 0xF7: /* PUSH AF */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0030;
		break;

	case 0xF8: /* LD HL, SP+/-imm */
	{
		/* Taken from SameBoy, which is released under MIT Licence. */
		int8_t offset = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
		gb->cpu_reg.hl.reg = gb->cpu_reg.sp.reg + offset;
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.h = ((gb->cpu_reg.sp.reg & 0xF) + (offset & 0xF) > 0xF) ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = ((gb->cpu_reg.sp.reg & 0xFF) + (offset & 0xFF) > 0xFF) ? 1 : 0;
		break;
	}

	case 0xF9: /* LD SP, HL */
		gb->cpu_reg.sp.reg = gb->cpu_reg.hl.reg;
		break;

	case 0xFA: /* LD A, (imm) */
	{
#if WALNUT_GB_16_BIT_DISABLED
    uint16_t addr = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2; // advance past the immediate
#else
    uint8_t l = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint8_t h = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint16_t addr = WALNUT_GB_U8_TO_U16(h, l);
#endif
		gb->cpu_reg.a = __gb_read(gb, addr);
		break;
	}

	case 0xFB: /* EI */
		gb->gb_ime = true;
		break;

	case 0xFE: /* CP imm */
	{
		uint8_t val = __gb_read(gb, gb->cpu_reg.pc.reg++);
		WGB_INSTR_CP_R8(val);
		break;
	}

	case 0xFF: /* RST 0x0038 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0038;
		break;

	default:
		/* Return address where invalid opcode that was read. */
		(gb->gb_error)(gb, GB_INVALID_OPCODE, gb->cpu_reg.pc.reg - 1);
		WGB_UNREACHABLE();
	}

	do
	{
		/* DIV register timing */
		gb->counter.div_count += inst_cycles;
		while(gb->counter.div_count >= DIV_CYCLES)
		{
			gb->hram_io[IO_DIV]++;
			gb->counter.div_count -= DIV_CYCLES;
		}

		/* Check for RTC tick. */
		if(gb->mbc == 3 && (gb->rtc_real.reg.high & 0x40) == 0)
		{
			gb->counter.rtc_count += inst_cycles;
			while(WGB_UNLIKELY(gb->counter.rtc_count >= RTC_CYCLES))
			{
				gb->counter.rtc_count -= RTC_CYCLES;

				/* Detect invalid rollover. */
				if(WGB_UNLIKELY(gb->rtc_real.reg.sec == 63))
				{
					gb->rtc_real.reg.sec = 0;
					continue;
				}

				if(++gb->rtc_real.reg.sec != 60)
					continue;

				gb->rtc_real.reg.sec = 0;
				if(gb->rtc_real.reg.min == 63)
				{
					gb->rtc_real.reg.min = 0;
					continue;
				}
				if(++gb->rtc_real.reg.min != 60)
					continue;

				gb->rtc_real.reg.min = 0;
				if(gb->rtc_real.reg.hour == 31)
				{
					gb->rtc_real.reg.hour = 0;
					continue;
				}
				if(++gb->rtc_real.reg.hour != 24)
					continue;

				gb->rtc_real.reg.hour = 0;
				if(++gb->rtc_real.reg.yday != 0)
					continue;

				if(gb->rtc_real.reg.high & 1)  /* Bit 8 of days*/
					gb->rtc_real.reg.high |= 0x80; /* Overflow bit */

				gb->rtc_real.reg.high ^= 1;
			}
		}

		/* Check serial transmission. */
		if(gb->hram_io[IO_SC] & SERIAL_SC_TX_START)
		{
			unsigned int serial_cycles = SERIAL_CYCLES_1KB;

			/* If new transfer, call TX function. */
			if(gb->counter.serial_count == 0 &&
				gb->gb_serial_tx != NULL)
				(gb->gb_serial_tx)(gb, gb->hram_io[IO_SB]);

#if WALNUT_FULL_GBC_SUPPORT
			if(gb->hram_io[IO_SC] & 0x3)
				serial_cycles = SERIAL_CYCLES_32KB;
#endif

			gb->counter.serial_count += inst_cycles;

			/* If it's time to receive byte, call RX function. */
			if(gb->counter.serial_count >= serial_cycles)
			{
				/* If RX can be done, do it. */
				/* If RX failed, do not change SB if using external
				 * clock, or set to 0xFF if using internal clock. */
				uint8_t rx;

				if(gb->gb_serial_rx != NULL &&
					(gb->gb_serial_rx(gb, &rx) ==
						GB_SERIAL_RX_SUCCESS))
				{
					gb->hram_io[IO_SB] = rx;

					/* Inform game of serial TX/RX completion. */
					gb->hram_io[IO_SC] &= 0x01;
					gb->hram_io[IO_IF] |= SERIAL_INTR;
				}
				else if(gb->hram_io[IO_SC] & SERIAL_SC_CLOCK_SRC)
				{
					/* If using internal clock, and console is not
					 * attached to any external peripheral, shifted
					 * bits are replaced with logic 1. */
					gb->hram_io[IO_SB] = 0xFF;

					/* Inform game of serial TX/RX completion. */
					gb->hram_io[IO_SC] &= 0x01;
					gb->hram_io[IO_IF] |= SERIAL_INTR;
				}
				else
				{
					/* If using external clock, and console is not
					 * attached to any external peripheral, bits are
					 * not shifted, so SB is not modified. */
				}

				gb->counter.serial_count = 0;
			}
		}

		/* TIMA register timing */
		/* TODO: Change tac_enable to struct of TAC timer control bits. */
		if(gb->hram_io[IO_TAC] & IO_TAC_ENABLE_MASK)
		{
			gb->counter.tima_count += inst_cycles;

			while(gb->counter.tima_count >=
				TAC_CYCLES[gb->hram_io[IO_TAC] & IO_TAC_RATE_MASK])
			{
				gb->counter.tima_count -=
					TAC_CYCLES[gb->hram_io[IO_TAC] & IO_TAC_RATE_MASK];

				if(++gb->hram_io[IO_TIMA] == 0)
				{
					gb->hram_io[IO_IF] |= TIMER_INTR;
					/* On overflow, set TMA to TIMA. */
					gb->hram_io[IO_TIMA] = gb->hram_io[IO_TMA];
				}
			}
		}

		/* If LCD is off, don't update LCD state or increase the LCD
		 * ticks. Instead, keep track of the amount of time that is
		 * being passed. */
		if(!(gb->hram_io[IO_LCDC] & LCDC_ENABLE))
		{
			gb->counter.lcd_off_count += inst_cycles;
			if(gb->counter.lcd_off_count >= LCD_FRAME_CYCLES)
			{
				gb->counter.lcd_off_count -= LCD_FRAME_CYCLES;
				gb->gb_frame = true;
			}
			continue;
		}

		/* LCD Timing */
#if WALNUT_FULL_GBC_SUPPORT
        if (inst_cycles > 1) {
            gb->counter.lcd_count += (inst_cycles >> gb->cgb.doubleSpeed);
        } else {
#endif
		gb->counter.lcd_count += inst_cycles;
#if WALNUT_FULL_GBC_SUPPORT
	}
#endif

		/* New Scanline. HBlank -> VBlank or OAM Scan */
		if(gb->counter.lcd_count >= LCD_LINE_CYCLES)
		{
			gb->counter.lcd_count -= LCD_LINE_CYCLES;

			/* Next line */
			gb->hram_io[IO_LY] = gb->hram_io[IO_LY] + 1;
			if (gb->hram_io[IO_LY] == LCD_VERT_LINES)
				gb->hram_io[IO_LY] = 0;

			/* LYC Update */
			if(gb->hram_io[IO_LY] == gb->hram_io[IO_LYC])
			{
				gb->hram_io[IO_STAT] |= STAT_LYC_COINC;

				if(gb->hram_io[IO_STAT] & STAT_LYC_INTR)
					gb->hram_io[IO_IF] |= LCDC_INTR;
			}
			else
				gb->hram_io[IO_STAT] &= 0xFB;

			/* Check if LCD should be in Mode 1 (VBLANK) state */
			if(gb->hram_io[IO_LY] == LCD_HEIGHT)
			{
				gb->hram_io[IO_STAT] =
					(gb->hram_io[IO_STAT] & ~STAT_MODE) | IO_STAT_MODE_VBLANK;
				gb->gb_frame = true;
				gb->hram_io[IO_IF] |= VBLANK_INTR;
				gb->lcd_blank = false;

				if(gb->hram_io[IO_STAT] & STAT_MODE_1_INTR)
					gb->hram_io[IO_IF] |= LCDC_INTR;

#if ENABLE_LCD
				/* If frame skip is activated, check if we need to draw
				 * the frame or skip it. */
				if(gb->direct.frame_skip)
				{
					gb->display.frame_skip_count =
						!gb->display.frame_skip_count;
				}

				/* If interlaced is activated, change which lines get
				 * updated. Also, only update lines on frames that are
				 * actually drawn when frame skip is enabled. */
				if(gb->direct.interlace &&
						(!gb->direct.frame_skip ||
						 gb->display.frame_skip_count))
				{
					gb->display.interlace_count =
						!gb->display.interlace_count;
				}
#endif
                                /* If halted forever, then return on VBLANK. */
                                if(gb->gb_halt && !gb->hram_io[IO_IE])
					break;
			}
			/* Start of normal Line (not in VBLANK) */
			else if(gb->hram_io[IO_LY] < LCD_HEIGHT)
			{
				if(gb->hram_io[IO_LY] == 0)
				{
					/* Clear Screen */
					gb->display.WY = gb->hram_io[IO_WY];
					gb->display.window_clear = 0;
				}

				/* OAM Search occurs at the start of the line. */
				gb->hram_io[IO_STAT] = (gb->hram_io[IO_STAT] & ~STAT_MODE) | IO_STAT_MODE_OAM_SCAN;
				gb->counter.lcd_count = 0;

#if WALNUT_FULL_GBC_SUPPORT
				//DMA GBC
				if(gb->cgb.cgbMode && !gb->cgb.dmaActive && gb->cgb.dmaMode)
				{
#if WALNUT_GB_32BIT_DMA
					// Optimized 16-bit path
					for (uint8_t i = 0; i < 0x10; i += 4)
					{
							uint32_t val = __gb_read32(gb, (gb->cgb.dmaSource & 0xFFF0) + i);
							__gb_write32(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i, val);
							// 8-bit logic if there is some cause to fall back
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i, val);
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i + 1, val >> 8);
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i + 2, val >> 16);
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i + 3, val >> 24);
					}
#elif WALNUT_GB_16BIT_DMA
					// Optimized 16-bit path
					for (uint8_t i = 0; i < 0x10; i += 2)
					{
							uint16_t val = __gb_read16(gb, (gb->cgb.dmaSource & 0xFFF0) + i);
							__gb_write16(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i, val);
							// 8-bit logic if there is some cause to fall back
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i, val & 0xFF);
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i + 1, val >> 8);
					}
#else
			    // Original 8-bit path
					for (uint8_t i = 0; i < 0x10; i++)
					{
						__gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i,
										__gb_read(gb, (gb->cgb.dmaSource & 0xFFF0) + i));
					}
#endif

					gb->cgb.dmaSource += 0x10;
					gb->cgb.dmaDest += 0x10;
					if(!(--gb->cgb.dmaSize)) {gb->cgb.dmaActive = 1;
					}
				}
#endif
				if(gb->hram_io[IO_STAT] & STAT_MODE_2_INTR)
					gb->hram_io[IO_IF] |= LCDC_INTR;

				/* If halted immediately jump to next LCD mode.
				 * From OAM Search to LCD Draw. */
				//if(gb->counter.lcd_count < LCD_MODE2_OAM_SCAN_END)
				//	inst_cycles = LCD_MODE2_OAM_SCAN_END - gb->counter.lcd_count;
				inst_cycles = LCD_MODE2_OAM_SCAN_DURATION;
			}
		}
		/* Go from Mode 3 (LCD Draw) to Mode 0 (HBLANK). */
		else if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_LCD_DRAW &&
				gb->counter.lcd_count >= LCD_MODE3_LCD_DRAW_END)
		{
			gb->hram_io[IO_STAT] = (gb->hram_io[IO_STAT] & ~STAT_MODE) | IO_STAT_MODE_HBLANK;

			if(gb->hram_io[IO_STAT] & STAT_MODE_0_INTR)
				gb->hram_io[IO_IF] |= LCDC_INTR;

			/* If halted immediately, jump from OAM Scan to LCD Draw. */
			if (gb->counter.lcd_count < LCD_MODE0_HBLANK_MAX_DRUATION)
				inst_cycles = LCD_MODE0_HBLANK_MAX_DRUATION - gb->counter.lcd_count;
		}
		/* Go from Mode 2 (OAM Scan) to Mode 3 (LCD Draw). */
		else if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_OAM_SCAN &&
				gb->counter.lcd_count >= LCD_MODE2_OAM_SCAN_END)
		{
			gb->hram_io[IO_STAT] = (gb->hram_io[IO_STAT] & ~STAT_MODE) | IO_STAT_MODE_LCD_DRAW;
#if ENABLE_LCD
			if(!gb->lcd_blank)
				__gb_draw_line(gb);
#endif
			/* If halted immediately jump to next LCD mode. */
			if (gb->counter.lcd_count < LCD_MODE3_LCD_DRAW_MIN_DURATION)
				inst_cycles = LCD_MODE3_LCD_DRAW_MIN_DURATION - gb->counter.lcd_count;
		}
	} while(gb->gb_halt && (gb->hram_io[IO_IF] & gb->hram_io[IO_IE]) == 0);
	/* If halted, loop until an interrupt occurs. */
}

/* Undefine CPU Flag helper functions. */
#undef WALNUT_GB_CPUFLAG_MASK_CARRY
#undef WALNUT_GB_CPUFLAG_MASK_HALFC
#undef WALNUT_GB_CPUFLAG_MASK_ARITH
#undef WALNUT_GB_CPUFLAG_MASK_ZERO
#undef WALNUT_GB_CPUFLAG_BIT_CARRY
#undef WALNUT_GB_CPUFLAG_BIT_HALFC
#undef WALNUT_GB_CPUFLAG_BIT_ARITH
#undef WALNUT_GB_CPUFLAG_BIT_ZERO
#undef WGB_SET_CARRY
#undef WGB_SET_HALFC
#undef WGB_SET_ARITH
#undef WGB_SET_ZERO
#undef WGB_GET_CARRY
#undef WGB_GET_HALFC
#undef WGB_GET_ARITH
#undef WGB_GET_ZERO

#if WALNUT_GB_16BIT_ALIGNED
#else
#endif

#if WALNUT_GB_32BIT_ALIGNED
#else
#endif


/**
 * Internal function used to read bytes.
 * addr is host platform endian.
 */



/**
 * Internal function used to write bytes.
 */

#if WALNUT_GB_32BIT_ALIGNED
#else
#endif

// The 16-bit write function is mainly for DMA transfers so focuses on accesible memory regions and falls back to the 8-bit version for all other cases.




void __gb_step_cpu(struct gb_s *gb)
{
	uint16_t oppair;
	uint8_t opcode;
	uint_fast16_t inst_cycles;
	static const uint8_t op_cycles[0x100] =
	{
		/* *INDENT-OFF* */
		/*0 1 2  3  4  5  6  7  8  9  A  B  C  D  E  F	*/
		4,12, 8, 8, 4, 4, 8, 4,20, 8, 8, 8, 4, 4, 8, 4,	/* 0x00 */
		4,12, 8, 8, 4, 4, 8, 4,12, 8, 8, 8, 4, 4, 8, 4,	/* 0x10 */
		8,12, 8, 8, 4, 4, 8, 4, 8, 8, 8, 8, 4, 4, 8, 4,	/* 0x20 */
		8,12, 8, 8,12,12,12, 4, 8, 8, 8, 8, 4, 4, 8, 4,	/* 0x30 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x40 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x50 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x60 */
		8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, /* 0x70 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x80 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x90 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0xA0 */
		4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0xB0 */
		8,12,12,16,12,16, 8,16, 8,16,12, 8,12,24, 8,16,	/* 0xC0 */
		8,12,12, 0,12,16, 8,16, 8,16,12, 0,12, 0, 8,16,	/* 0xD0 */
		12,12,8, 0, 0,16, 8,16,16, 4,16, 0, 0, 0, 8,16,	/* 0xE0 */
		12,12,8, 4, 0,16, 8,16,12, 8,16, 4, 0, 0, 8,16	/* 0xF0 */
		/* *INDENT-ON* */
	};
	static const uint_fast16_t TAC_CYCLES[4] = {1024, 16, 64, 256};

	/* Handle interrupts */
	/* If gb_halt is positive, then an interrupt must have occurred by the
	 * time we reach here, because on HALT, we jump to the next interrupt
	 * immediately. */
	while(gb->gb_halt || (gb->gb_ime &&
			gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & ANY_INTR))
	{
		gb->gb_halt = false;

		if(!gb->gb_ime)
			break;

		/* Disable interrupts */
		gb->gb_ime = false;

		/* Push Program Counter */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);

		/* Call interrupt handler if required. */
		if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & VBLANK_INTR)
		{
			gb->cpu_reg.pc.reg = VBLANK_INTR_ADDR;
			gb->hram_io[IO_IF] ^= VBLANK_INTR;
		}
		else if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & LCDC_INTR)
		{
			gb->cpu_reg.pc.reg = LCDC_INTR_ADDR;
			gb->hram_io[IO_IF] ^= LCDC_INTR;
		}
		else if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & TIMER_INTR)
		{
			gb->cpu_reg.pc.reg = TIMER_INTR_ADDR;
			gb->hram_io[IO_IF] ^= TIMER_INTR;
		}
		else if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & SERIAL_INTR)
		{
			gb->cpu_reg.pc.reg = SERIAL_INTR_ADDR;
			gb->hram_io[IO_IF] ^= SERIAL_INTR;
		}
		else if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & CONTROL_INTR)
		{
			gb->cpu_reg.pc.reg = CONTROL_INTR_ADDR;
			gb->hram_io[IO_IF] ^= CONTROL_INTR;
		}

		break;
	}

	/* Obtain opcode */
	
	oppair = __gb_read16(gb, gb->cpu_reg.pc.reg++);
	opcode = (uint8_t)oppair; // auto-truncate
#if (WALNUT_GB_SAFE_DUALFETCH_DMA || WALNUT_GB_SAFE_DUALFETCH_MBC)
  gb->prefetch_invalid=false;
#endif
	inst_cycles = op_cycles[opcode];

	/* Execute opcode */
	switch(opcode)
	{
	case 0x00: /* NOP */
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x01: /* LD BC, imm */
#if WALNUT_GB_16_BIT_OPS_DUALFETCH
    gb->cpu_reg.bc.bytes.c = (uint8_t)(oppair >> 8); // C was already partially loaded in oppair
    oppair = __gb_read16(gb, gb->cpu_reg.pc.reg + 1); // Read 16-bit immediate starting from PC+1    
    gb->cpu_reg.bc.bytes.b = (uint8_t)(oppair);// Store lower byte of immediate into B
    gb->cpu_reg.pc.reg += 2;// Increment PC by 2 to skip over the 16-bit immediate
    // Prefetch next opcode from upper byte of read16
    opcode = (uint8_t)(oppair >> 8);
#else
    // Original 8-bit fetch path
    gb->cpu_reg.bc.bytes.c = (uint8_t)(oppair >> 8);
    gb->cpu_reg.pc.reg++;
    gb->cpu_reg.bc.bytes.b = __gb_read(gb, gb->cpu_reg.pc.reg++);
    opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
#endif
		break;
	case 0x02: /* LD (BC), A */
		__gb_write(gb, gb->cpu_reg.bc.reg, gb->cpu_reg.a);	  
#if WALNUT_GB_SAFE_DUALFETCH_OPCODES
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); // GTA SAFE TEST
#else
		opcode = (uint8_t)(oppair >> 8);
#endif
		break;

	case 0x03: /* INC BC */
		gb->cpu_reg.bc.reg++;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x04: /* INC B */
		WGB_INSTR_INC_R8(gb->cpu_reg.bc.bytes.b);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x05: /* DEC B */
		WGB_INSTR_DEC_R8(gb->cpu_reg.bc.bytes.b);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x06: /* LD B, imm */
		gb->cpu_reg.bc.bytes.b = (uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;
	case 0x07: /* RLCA */
		gb->cpu_reg.a = (gb->cpu_reg.a << 1) | (gb->cpu_reg.a >> 7);
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.c = (gb->cpu_reg.a & 0x01);
		opcode = (uint8_t)(oppair >> 8);
		break;
	case 0x08: /* LD (imm), SP */
	{		
#if WALNUT_GB_16_BIT_OPS_DUALFETCH
		uint8_t l = (uint8_t)(oppair >> 8);
		gb->cpu_reg.pc.reg++;
    oppair = __gb_read16(gb, gb->cpu_reg.pc.reg++);
		uint8_t h = oppair;
		uint16_t temp = WALNUT_GB_U8_TO_U16(h, l);
		__gb_write16(gb,temp,gb->cpu_reg.sp.reg);
    // gb->cpu_reg.pc.reg+=2;
		opcode = oppair >> 8;
#else
    uint8_t l = (uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
    uint8_t h = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint16_t temp = WALNUT_GB_U8_TO_U16(h, l);
    __gb_write(gb, temp++, gb->cpu_reg.sp.bytes.p);
    __gb_write(gb, temp, gb->cpu_reg.sp.bytes.s);
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
#endif
    break;
	}

	case 0x09: /* ADD HL, BC */
	{
		uint_fast32_t temp = gb->cpu_reg.hl.reg + gb->cpu_reg.bc.reg;
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h =
			(temp ^ gb->cpu_reg.hl.reg ^ gb->cpu_reg.bc.reg) & 0x1000 ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = (temp & 0xFFFF0000) ? 1 : 0;
		gb->cpu_reg.hl.reg = (temp & 0x0000FFFF);
		opcode = (uint8_t)(oppair >> 8);
		break;
	}

	case 0x0A: /* LD A, (BC) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.bc.reg);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x0B: /* DEC BC */
		gb->cpu_reg.bc.reg--;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x0C: /* INC C */
		WGB_INSTR_INC_R8(gb->cpu_reg.bc.bytes.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x0D: /* DEC C */
		WGB_INSTR_DEC_R8(gb->cpu_reg.bc.bytes.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x0E: /* LD C, imm */
		gb->cpu_reg.bc.bytes.c = (uint8_t)(oppair >> 8);
		gb->cpu_reg.pc.reg++;
	  opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0x0F: /* RRCA */
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.c = gb->cpu_reg.a & 0x01;
		gb->cpu_reg.a = (gb->cpu_reg.a >> 1) | (gb->cpu_reg.a << 7);
		opcode = (uint8_t)(oppair >> 8);
		break;
	case 0x10: /* STOP */
		//gb->gb_halt = true;
#if WALNUT_FULL_GBC_SUPPORT
		if(gb->cgb.cgbMode & gb->cgb.doubleSpeedPrep)
		{
			gb->cgb.doubleSpeedPrep = 0;
			gb->cgb.doubleSpeed ^= 1;
		}
#endif
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x11: /* LD DE, imm */
#if WALNUT_GB_16_BIT_OPS_DUALFETCH
    gb->cpu_reg.de.bytes.e = (uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
		oppair = __gb_read16(gb, gb->cpu_reg.pc.reg++);
    gb->cpu_reg.de.bytes.d = oppair;
		opcode = oppair >> 8;
#else
    gb->cpu_reg.de.bytes.e = (uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
    gb->cpu_reg.de.bytes.d = __gb_read(gb, gb->cpu_reg.pc.reg++);
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
#endif
		break;
	case 0x12: /* LD (DE), A */
		__gb_write(gb, gb->cpu_reg.de.reg, gb->cpu_reg.a);
#if WALNUT_GB_SAFE_DUALFETCH_OPCODES
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); // GTA SAFE TEST
#else
		opcode = (uint8_t)(oppair >> 8);
#endif
		break;

	case 0x13: /* INC DE */
		gb->cpu_reg.de.reg++;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x14: /* INC D */
		WGB_INSTR_INC_R8(gb->cpu_reg.de.bytes.d);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x15: /* DEC D */
		WGB_INSTR_DEC_R8(gb->cpu_reg.de.bytes.d);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x16: /* LD D, imm */
		gb->cpu_reg.de.bytes.d = (uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
	  opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0x17: /* RLA */
	{
		uint8_t temp = gb->cpu_reg.a;
		gb->cpu_reg.a = (gb->cpu_reg.a << 1) | gb->cpu_reg.f.f_bits.c;
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.c = (temp >> 7) & 0x01;
		opcode = (uint8_t)(oppair >> 8);
		break;
	}

	case 0x18: /* JR imm */
	{
		int8_t temp = (int8_t) (uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
		gb->cpu_reg.pc.reg += temp;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;
	}
	case 0x19: /* ADD HL, DE */
	{
		uint_fast32_t temp = gb->cpu_reg.hl.reg + gb->cpu_reg.de.reg;
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h =
			(temp ^ gb->cpu_reg.hl.reg ^ gb->cpu_reg.de.reg) & 0x1000 ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = (temp & 0xFFFF0000) ? 1 : 0;
		gb->cpu_reg.hl.reg = (temp & 0x0000FFFF);
		opcode = (uint8_t)(oppair >> 8);
		break;
	}
	case 0x1A: /* LD A, (DE) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.de.reg);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x1B: /* DEC DE */
		gb->cpu_reg.de.reg--;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x1C: /* INC E */
		WGB_INSTR_INC_R8(gb->cpu_reg.de.bytes.e);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x1D: /* DEC E */
		WGB_INSTR_DEC_R8(gb->cpu_reg.de.bytes.e);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x1E: /* LD E, imm */
		gb->cpu_reg.de.bytes.e =  (uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
	  opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0x1F: /* RRA */
	{
		uint8_t temp = gb->cpu_reg.a;
		gb->cpu_reg.a = gb->cpu_reg.a >> 1 | (gb->cpu_reg.f.f_bits.c << 7);
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.c = temp & 0x1;
		opcode = (uint8_t)(oppair >> 8);
		break;		
	}

	case 0x20: /* JR NZ, imm */
		if(!gb->cpu_reg.f.f_bits.z)
		{
			int8_t temp = (int8_t) (oppair >> 8);
	    gb->cpu_reg.pc.reg++;
			gb->cpu_reg.pc.reg += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg++;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0x21: /* LD HL, imm */
#if WALNUT_GB_16_BIT_OPS_DUALFETCH
		gb->cpu_reg.hl.bytes.l =(uint8_t)(oppair >> 8);
		gb->cpu_reg.pc.reg++;
		oppair = __gb_read16(gb, gb->cpu_reg.pc.reg++);
    gb->cpu_reg.hl.bytes.h = oppair;
		opcode = oppair >> 8;
#else
    gb->cpu_reg.hl.bytes.l =(uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
    gb->cpu_reg.hl.bytes.h = __gb_read(gb, gb->cpu_reg.pc.reg++);
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
#endif
		break;

	case 0x22: /* LDI (HL), A */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.a);
		gb->cpu_reg.hl.reg++;
#if WALNUT_GB_SAFE_DUALFETCH_OPCODES
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); // GTA SAFE TEST
#else
		opcode = (uint8_t)(oppair >> 8);
#endif
		break;

	case 0x23: /* INC HL */
		gb->cpu_reg.hl.reg++;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x24: /* INC H */
		WGB_INSTR_INC_R8(gb->cpu_reg.hl.bytes.h);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x25: /* DEC H */
		WGB_INSTR_DEC_R8(gb->cpu_reg.hl.bytes.h);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x26: /* LD H, imm */
		gb->cpu_reg.hl.bytes.h =(uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
	  opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0x27: /* DAA */
	{
		/* The following is from SameBoy. MIT License. */
		int16_t a = gb->cpu_reg.a;

		if(gb->cpu_reg.f.f_bits.n)
		{
			if(gb->cpu_reg.f.f_bits.h)
				a = (a - 0x06) & 0xFF;

			if(gb->cpu_reg.f.f_bits.c)
				a -= 0x60;
		}
		else
		{
			if(gb->cpu_reg.f.f_bits.h || (a & 0x0F) > 9)
				a += 0x06;

			if(gb->cpu_reg.f.f_bits.c || a > 0x9F)
				a += 0x60;
		}

		if((a & 0x100) == 0x100)
			gb->cpu_reg.f.f_bits.c = 1;

		gb->cpu_reg.a = (uint8_t)a;
		gb->cpu_reg.f.f_bits.z = (gb->cpu_reg.a == 0);
		gb->cpu_reg.f.f_bits.h = 0;
		opcode = (uint8_t)(oppair >> 8);
		break;
	}

	case 0x28: /* JR Z, imm */
		if(gb->cpu_reg.f.f_bits.z)
		{
			int8_t temp = (int8_t) (oppair >> 8);
	    gb->cpu_reg.pc.reg++;
			gb->cpu_reg.pc.reg += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg++;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0x29: /* ADD HL, HL */
	{
		gb->cpu_reg.f.f_bits.c = (gb->cpu_reg.hl.reg & 0x8000) > 0;
		gb->cpu_reg.hl.reg <<= 1;
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h = (gb->cpu_reg.hl.reg & 0x1000) > 0;
		opcode = (uint8_t)(oppair >> 8);
		break;
	}

	case 0x2A: /* LD A, (HL+) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.hl.reg++);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x2B: /* DEC HL */
		gb->cpu_reg.hl.reg--;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x2C: /* INC L */
		WGB_INSTR_INC_R8(gb->cpu_reg.hl.bytes.l);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x2D: /* DEC L */
		WGB_INSTR_DEC_R8(gb->cpu_reg.hl.bytes.l);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x2E: /* LD L, imm */
		gb->cpu_reg.hl.bytes.l = (uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
	  opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;
	
	case 0x2F: /* CPL */
		gb->cpu_reg.a = ~gb->cpu_reg.a;
		gb->cpu_reg.f.f_bits.n = 1;
		gb->cpu_reg.f.f_bits.h = 1;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x30: /* JR NC, imm */
		if(!gb->cpu_reg.f.f_bits.c)
		{
			int8_t temp = (int8_t) (oppair >> 8);
	    gb->cpu_reg.pc.reg++;
			gb->cpu_reg.pc.reg += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg++;
    opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0x31: /* LD SP, imm */
#if WALNUT_GB_16_BIT_OPS_DUALFETCH
		gb->cpu_reg.sp.bytes.p = (uint8_t)(oppair >> 8);
		gb->cpu_reg.pc.reg ++;
    oppair = __gb_read16(gb, gb->cpu_reg.pc.reg++);
		gb->cpu_reg.sp.bytes.s = oppair; // auto-truncated
		opcode = oppair >> 8; 
#else
    gb->cpu_reg.sp.bytes.p = (uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
    gb->cpu_reg.sp.bytes.s = __gb_read(gb, gb->cpu_reg.pc.reg++);
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); 
#endif  
		break;

	case 0x32: /* LD (HL), A */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.a);
		gb->cpu_reg.hl.reg--;
#if WALNUT_GB_SAFE_DUALFETCH_OPCODES
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); // GTA SAFE TEST
#else
		opcode = (uint8_t)(oppair >> 8);
#endif
		break;

	case 0x33: /* INC SP */
		gb->cpu_reg.sp.reg++;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x34: /* INC (HL) */
	{
		uint8_t temp = __gb_read(gb, gb->cpu_reg.hl.reg);
		WGB_INSTR_INC_R8(temp);
		__gb_write(gb, gb->cpu_reg.hl.reg, temp);
		opcode = (uint8_t)(oppair >> 8);
		break;
	}

	case 0x35: /* DEC (HL) */
	{
		uint8_t temp = __gb_read(gb, gb->cpu_reg.hl.reg);
		WGB_INSTR_DEC_R8(temp);
		__gb_write(gb, gb->cpu_reg.hl.reg, temp);
		opcode = (uint8_t)(oppair >> 8);
		break;
	}

	case 0x36: /* LD (HL), imm */
		__gb_write(gb, gb->cpu_reg.hl.reg,(uint8_t)(oppair >> 8));
		gb->cpu_reg.pc.reg++;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0x37: /* SCF */
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h = 0;
		gb->cpu_reg.f.f_bits.c = 1;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x38: /* JR C, imm */
		if(gb->cpu_reg.f.f_bits.c)
		{
			int8_t temp = (int8_t)(oppair >> 8);
	    gb->cpu_reg.pc.reg++;
			gb->cpu_reg.pc.reg += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg++;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0x39: /* ADD HL, SP */
	{
		uint_fast32_t temp = gb->cpu_reg.hl.reg + gb->cpu_reg.sp.reg;
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h =
			((gb->cpu_reg.hl.reg & 0xFFF) + (gb->cpu_reg.sp.reg & 0xFFF)) & 0x1000 ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = temp & 0x10000 ? 1 : 0;
		gb->cpu_reg.hl.reg = (uint16_t)temp;
		opcode = (uint8_t)(oppair >> 8);
		break;
	}

	case 0x3A: /* LD A, (HL) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.hl.reg--);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x3B: /* DEC SP */
		gb->cpu_reg.sp.reg--;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x3C: /* INC A */
		WGB_INSTR_INC_R8(gb->cpu_reg.a);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x3D: /* DEC A */
		WGB_INSTR_DEC_R8(gb->cpu_reg.a);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x3E: /* LD A, imm */
		gb->cpu_reg.a = (uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
	  opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0x3F: /* CCF */
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h = 0;
		gb->cpu_reg.f.f_bits.c = ~gb->cpu_reg.f.f_bits.c;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x40: /* LD B, B */
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x41: /* LD B, C */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.bc.bytes.c;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x42: /* LD B, D */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.de.bytes.d;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x43: /* LD B, E */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.de.bytes.e;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x44: /* LD B, H */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.hl.bytes.h;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x45: /* LD B, L */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.hl.bytes.l;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x46: /* LD B, (HL) */
		gb->cpu_reg.bc.bytes.b = __gb_read(gb, gb->cpu_reg.hl.reg);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x47: /* LD B, A */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.a;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x48: /* LD C, B */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.bc.bytes.b;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x49: /* LD C, C */
	  opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x4A: /* LD C, D */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.de.bytes.d;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x4B: /* LD C, E */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.de.bytes.e;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x4C: /* LD C, H */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.hl.bytes.h;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x4D: /* LD C, L */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.hl.bytes.l;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x4E: /* LD C, (HL) */
		gb->cpu_reg.bc.bytes.c = __gb_read(gb, gb->cpu_reg.hl.reg);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x4F: /* LD C, A */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.a;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x50: /* LD D, B */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.bc.bytes.b;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x51: /* LD D, C */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.bc.bytes.c;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x52: /* LD D, D */
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x53: /* LD D, E */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.de.bytes.e;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x54: /* LD D, H */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.hl.bytes.h;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x55: /* LD D, L */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.hl.bytes.l;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x56: /* LD D, (HL) */
		gb->cpu_reg.de.bytes.d = __gb_read(gb, gb->cpu_reg.hl.reg);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x57: /* LD D, A */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.a;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x58: /* LD E, B */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.bc.bytes.b;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x59: /* LD E, C */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.bc.bytes.c;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x5A: /* LD E, D */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.de.bytes.d;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x5B: /* LD E, E */
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x5C: /* LD E, H */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.hl.bytes.h;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x5D: /* LD E, L */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.hl.bytes.l;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x5E: /* LD E, (HL) */
		gb->cpu_reg.de.bytes.e = __gb_read(gb, gb->cpu_reg.hl.reg);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x5F: /* LD E, A */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.a;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x60: /* LD H, B */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.bc.bytes.b;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x61: /* LD H, C */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.bc.bytes.c;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x62: /* LD H, D */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.de.bytes.d;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x63: /* LD H, E */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.de.bytes.e;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x64: /* LD H, H */
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x65: /* LD H, L */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.hl.bytes.l;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x66: /* LD H, (HL) */
		gb->cpu_reg.hl.bytes.h = __gb_read(gb, gb->cpu_reg.hl.reg);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x67: /* LD H, A */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.a;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x68: /* LD L, B */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.bc.bytes.b;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x69: /* LD L, C */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.bc.bytes.c;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x6A: /* LD L, D */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.de.bytes.d;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x6B: /* LD L, E */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.de.bytes.e;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x6C: /* LD L, H */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.hl.bytes.h;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x6D: /* LD L, L */
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x6E: /* LD L, (HL) */
		gb->cpu_reg.hl.bytes.l = __gb_read(gb, gb->cpu_reg.hl.reg);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x6F: /* LD L, A */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.a;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x70: /* LD (HL), B */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.bc.bytes.b);
#if WALNUT_GB_SAFE_DUALFETCH_OPCODES
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); // GTA SAFE TEST
#else
		opcode = (uint8_t)(oppair >> 8);
#endif	
		break;

	case 0x71: /* LD (HL), C */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.bc.bytes.c);
#if WALNUT_GB_SAFE_DUALFETCH_OPCODES
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); // GTA SAFE TEST
#else
		opcode = (uint8_t)(oppair >> 8);
#endif		
		break;

	case 0x72: /* LD (HL), D */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.de.bytes.d);
#if WALNUT_GB_SAFE_DUALFETCH_OPCODES
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); // GTA SAFE TEST
#else
		opcode = (uint8_t)(oppair >> 8);
#endif
		break;

	case 0x73: /* LD (HL), E */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.de.bytes.e);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x74: /* LD (HL), H */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.hl.bytes.h);
#if WALNUT_GB_SAFE_DUALFETCH_OPCODES
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); // GTA SAFE TEST
#else
		opcode = (uint8_t)(oppair >> 8);
#endif
		break;

	case 0x75: /* LD (HL), L */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.hl.bytes.l);
#if WALNUT_GB_SAFE_DUALFETCH_OPCODES
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); // GTA SAFE TEST
#else
		opcode = (uint8_t)(oppair >> 8);
#endif	
		break;

	case 0x76: /* HALT */
	{
		int_fast16_t halt_cycles = INT_FAST16_MAX;

		/* TODO: Emulate HALT bug? */
		gb->gb_halt = true;

		if(gb->hram_io[IO_SC] & SERIAL_SC_TX_START)
		{
			int serial_cycles = SERIAL_CYCLES -
				gb->counter.serial_count;

			if(serial_cycles < halt_cycles)
				halt_cycles = serial_cycles;
		}

		if(gb->hram_io[IO_TAC] & IO_TAC_ENABLE_MASK)
		{
			int tac_cycles = TAC_CYCLES[gb->hram_io[IO_TAC] & IO_TAC_RATE_MASK] -
				gb->counter.tima_count;

			if(tac_cycles < halt_cycles)
				halt_cycles = tac_cycles;
		}

		if((gb->hram_io[IO_LCDC] & LCDC_ENABLE))
		{
			int lcd_cycles;

			/* If LCD is in HBlank, calculate the number of cycles
			 * until the end of HBlank and the start of mode 2 or
			 * mode 1. */
			if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_HBLANK)
			{
				lcd_cycles = LCD_MODE0_HBLANK_MAX_DRUATION - gb->counter.lcd_count;
			}
			else if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_OAM_SCAN)
			{
				lcd_cycles = LCD_MODE3_LCD_DRAW_MIN_DURATION - gb->counter.lcd_count;
			}
			else if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_LCD_DRAW)
			{
				lcd_cycles = LCD_MODE0_HBLANK_MAX_DRUATION - gb->counter.lcd_count;
			}
			else
			{
				/* VBlank */
				lcd_cycles = LCD_LINE_CYCLES - gb->counter.lcd_count;
			}

			if(lcd_cycles < halt_cycles)
				halt_cycles = lcd_cycles;
		}

		/* Some halt cycles may already be very high, so make sure we
		 * don't underflow here. */
		if(halt_cycles <= 0)
			halt_cycles = 4;

		inst_cycles = (uint_fast16_t)halt_cycles;

#if WALNUT_GB_SAFE_DUALFETCH_OPCODES
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); // possibly changes mbc behaviour
#else
    opcode = (uint8_t)(oppair >> 8);
#endif
		break;
	}
	

	case 0x77: /* LD (HL), A */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.a);
#if WALNUT_GB_SAFE_DUALFETCH_OPCODES
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); // GTA SAFE TEST
#else
		opcode = (uint8_t)(oppair >> 8);
#endif
		break;

	case 0x78: /* LD A, B */
		gb->cpu_reg.a = gb->cpu_reg.bc.bytes.b;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x79: /* LD A, C */
		gb->cpu_reg.a = gb->cpu_reg.bc.bytes.c;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x7A: /* LD A, D */
		gb->cpu_reg.a = gb->cpu_reg.de.bytes.d;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x7B: /* LD A, E */
		gb->cpu_reg.a = gb->cpu_reg.de.bytes.e;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x7C: /* LD A, H */
		gb->cpu_reg.a = gb->cpu_reg.hl.bytes.h;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x7D: /* LD A, L */
		gb->cpu_reg.a = gb->cpu_reg.hl.bytes.l;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x7E: /* LD A, (HL) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.hl.reg);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x7F: /* LD A, A */
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x80: /* ADD A, B */
		WGB_INSTR_ADC_R8(gb->cpu_reg.bc.bytes.b, 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x81: /* ADD A, C */
		WGB_INSTR_ADC_R8(gb->cpu_reg.bc.bytes.c, 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x82: /* ADD A, D */
		WGB_INSTR_ADC_R8(gb->cpu_reg.de.bytes.d, 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x83: /* ADD A, E */
		WGB_INSTR_ADC_R8(gb->cpu_reg.de.bytes.e, 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x84: /* ADD A, H */
		WGB_INSTR_ADC_R8(gb->cpu_reg.hl.bytes.h, 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x85: /* ADD A, L */
		WGB_INSTR_ADC_R8(gb->cpu_reg.hl.bytes.l, 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x86: /* ADD A, (HL) */
		WGB_INSTR_ADC_R8(__gb_read(gb, gb->cpu_reg.hl.reg), 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x87: /* ADD A, A */
		WGB_INSTR_ADC_R8(gb->cpu_reg.a, 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x88: /* ADC A, B */
		WGB_INSTR_ADC_R8(gb->cpu_reg.bc.bytes.b, gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x89: /* ADC A, C */
		WGB_INSTR_ADC_R8(gb->cpu_reg.bc.bytes.c, gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x8A: /* ADC A, D */
		WGB_INSTR_ADC_R8(gb->cpu_reg.de.bytes.d, gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x8B: /* ADC A, E */
		WGB_INSTR_ADC_R8(gb->cpu_reg.de.bytes.e, gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x8C: /* ADC A, H */
		WGB_INSTR_ADC_R8(gb->cpu_reg.hl.bytes.h, gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x8D: /* ADC A, L */
		WGB_INSTR_ADC_R8(gb->cpu_reg.hl.bytes.l, gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x8E: /* ADC A, (HL) */
		WGB_INSTR_ADC_R8(__gb_read(gb, gb->cpu_reg.hl.reg), gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x8F: /* ADC A, A */
		WGB_INSTR_ADC_R8(gb->cpu_reg.a, gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x90: /* SUB B */
		WGB_INSTR_SBC_R8(gb->cpu_reg.bc.bytes.b, 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x91: /* SUB C */
		WGB_INSTR_SBC_R8(gb->cpu_reg.bc.bytes.c, 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x92: /* SUB D */
		WGB_INSTR_SBC_R8(gb->cpu_reg.de.bytes.d, 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x93: /* SUB E */
		WGB_INSTR_SBC_R8(gb->cpu_reg.de.bytes.e, 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x94: /* SUB H */
		WGB_INSTR_SBC_R8(gb->cpu_reg.hl.bytes.h, 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x95: /* SUB L */
		WGB_INSTR_SBC_R8(gb->cpu_reg.hl.bytes.l, 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x96: /* SUB (HL) */
		WGB_INSTR_SBC_R8(__gb_read(gb, gb->cpu_reg.hl.reg), 0);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x97: /* SUB A */
		gb->cpu_reg.a = 0;
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.z = 1;
		gb->cpu_reg.f.f_bits.n = 1;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x98: /* SBC A, B */
		WGB_INSTR_SBC_R8(gb->cpu_reg.bc.bytes.b, gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x99: /* SBC A, C */
		WGB_INSTR_SBC_R8(gb->cpu_reg.bc.bytes.c, gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x9A: /* SBC A, D */
		WGB_INSTR_SBC_R8(gb->cpu_reg.de.bytes.d, gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x9B: /* SBC A, E */
		WGB_INSTR_SBC_R8(gb->cpu_reg.de.bytes.e, gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x9C: /* SBC A, H */
		WGB_INSTR_SBC_R8(gb->cpu_reg.hl.bytes.h, gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x9D: /* SBC A, L */
		WGB_INSTR_SBC_R8(gb->cpu_reg.hl.bytes.l, gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x9E: /* SBC A, (HL) */
		WGB_INSTR_SBC_R8(__gb_read(gb, gb->cpu_reg.hl.reg), gb->cpu_reg.f.f_bits.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0x9F: /* SBC A, A */
		gb->cpu_reg.a = gb->cpu_reg.f.f_bits.c ? 0xFF : 0x00;
		gb->cpu_reg.f.f_bits.z = !gb->cpu_reg.f.f_bits.c;
		gb->cpu_reg.f.f_bits.n = 1;
		gb->cpu_reg.f.f_bits.h = gb->cpu_reg.f.f_bits.c;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xA0: /* AND B */
		WGB_INSTR_AND_R8(gb->cpu_reg.bc.bytes.b);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xA1: /* AND C */
		WGB_INSTR_AND_R8(gb->cpu_reg.bc.bytes.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xA2: /* AND D */
		WGB_INSTR_AND_R8(gb->cpu_reg.de.bytes.d);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xA3: /* AND E */
		WGB_INSTR_AND_R8(gb->cpu_reg.de.bytes.e);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xA4: /* AND H */
		WGB_INSTR_AND_R8(gb->cpu_reg.hl.bytes.h);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xA5: /* AND L */
		WGB_INSTR_AND_R8(gb->cpu_reg.hl.bytes.l);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xA6: /* AND (HL) */
		WGB_INSTR_AND_R8(__gb_read(gb, gb->cpu_reg.hl.reg));
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xA7: /* AND A */
		WGB_INSTR_AND_R8(gb->cpu_reg.a);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xA8: /* XOR B */
		WGB_INSTR_XOR_R8(gb->cpu_reg.bc.bytes.b);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xA9: /* XOR C */
		WGB_INSTR_XOR_R8(gb->cpu_reg.bc.bytes.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xAA: /* XOR D */
		WGB_INSTR_XOR_R8(gb->cpu_reg.de.bytes.d);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xAB: /* XOR E */
		WGB_INSTR_XOR_R8(gb->cpu_reg.de.bytes.e);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xAC: /* XOR H */
		WGB_INSTR_XOR_R8(gb->cpu_reg.hl.bytes.h);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xAD: /* XOR L */
		WGB_INSTR_XOR_R8(gb->cpu_reg.hl.bytes.l);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xAE: /* XOR (HL) */
		WGB_INSTR_XOR_R8(__gb_read(gb, gb->cpu_reg.hl.reg));
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xAF: /* XOR A */
		WGB_INSTR_XOR_R8(gb->cpu_reg.a);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xB0: /* OR B */
		WGB_INSTR_OR_R8(gb->cpu_reg.bc.bytes.b);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xB1: /* OR C */
		WGB_INSTR_OR_R8(gb->cpu_reg.bc.bytes.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xB2: /* OR D */
		WGB_INSTR_OR_R8(gb->cpu_reg.de.bytes.d);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xB3: /* OR E */
		WGB_INSTR_OR_R8(gb->cpu_reg.de.bytes.e);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xB4: /* OR H */
		WGB_INSTR_OR_R8(gb->cpu_reg.hl.bytes.h);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xB5: /* OR L */
		WGB_INSTR_OR_R8(gb->cpu_reg.hl.bytes.l);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xB6: /* OR (HL) */
		WGB_INSTR_OR_R8(__gb_read(gb, gb->cpu_reg.hl.reg));
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xB7: /* OR A */
		WGB_INSTR_OR_R8(gb->cpu_reg.a);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xB8: /* CP B */
		WGB_INSTR_CP_R8(gb->cpu_reg.bc.bytes.b);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xB9: /* CP C */
		WGB_INSTR_CP_R8(gb->cpu_reg.bc.bytes.c);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xBA: /* CP D */
		WGB_INSTR_CP_R8(gb->cpu_reg.de.bytes.d);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xBB: /* CP E */
		WGB_INSTR_CP_R8(gb->cpu_reg.de.bytes.e);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xBC: /* CP H */
		WGB_INSTR_CP_R8(gb->cpu_reg.hl.bytes.h);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xBD: /* CP L */
		WGB_INSTR_CP_R8(gb->cpu_reg.hl.bytes.l);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xBE: /* CP (HL) */
		WGB_INSTR_CP_R8(__gb_read(gb, gb->cpu_reg.hl.reg));
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xBF: /* CP A */
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.z = 1;
		gb->cpu_reg.f.f_bits.n = 1;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xC0: /* RET NZ */
		if(!gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED3
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
        gb->cpu_reg.sp.reg += 2;
#else
        gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
        gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
        inst_cycles += 12;
		}
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xC1: /* POP BC */
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED3
    gb->cpu_reg.bc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2;
#else
    gb->cpu_reg.bc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.bc.bytes.b = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xC2: /* JP NZ, imm */
		if(!gb->cpu_reg.f.f_bits.z)
		{		// best without 16-bit read when used in first half of dual fetch chain
        uint8_t c = (uint8_t)(oppair >> 8);
				gb->cpu_reg.pc.reg++;
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg += 2;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xC3: /* JP imm */
		{ // best without 16-bit read when used in first half of dual fetch chain
    uint8_t c = (uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
    uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
    gb->cpu_reg.pc.bytes.c = c;
    gb->cpu_reg.pc.bytes.p = p;
		}
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xC4: /* CALL NZ imm */
		if(!gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED3
        uint8_t c, p;
        c = (uint8_t)(oppair >> 8);        // upper byte of immediate
        gb->cpu_reg.pc.reg++;              // advance to low byte
        p = __gb_read(gb, gb->cpu_reg.pc.reg++); // read low byte
        // Push current PC onto stack
        gb->cpu_reg.sp.reg -= 2;          // reserve space for 2 bytes
        __gb_write16(gb, gb->cpu_reg.sp.reg, gb->cpu_reg.pc.reg); 
        // Load immediate address into PC
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#else
			uint8_t c, p;
			c = (uint8_t)(oppair >> 8);
	    gb->cpu_reg.pc.reg++;
			p = __gb_read(gb, gb->cpu_reg.pc.reg++);
			__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
			__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
			gb->cpu_reg.pc.bytes.c = c;
			gb->cpu_reg.pc.bytes.p = p;
#endif        
			inst_cycles += 12;
		}
		else
			gb->cpu_reg.pc.reg += 2;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xC5: /* PUSH BC */
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED3
		gb->cpu_reg.sp.reg-=2;
		__gb_write16(gb, gb->cpu_reg.sp.reg, gb->cpu_reg.bc.reg);
#else
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.bc.bytes.b);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.bc.bytes.c);
#endif
		opcode = (uint8_t)(oppair >> 8); 
		break;

	case 0xC6: /* ADD A, imm */
	{
		uint8_t val = (uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
		WGB_INSTR_ADC_R8(val, 0);
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;
	}

	case 0xC7: /* RST 0x0000 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0000;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xC8: /* RET Z */
		if(gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED2
        // Optional 16-bit path
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
        gb->cpu_reg.sp.reg += 2;
#else
        gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
        gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
			inst_cycles += 12;
			opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		}
		else
		  opcode=(uint8_t)(oppair >> 8);
		break;

	case 0xC9: /* RET */
	{
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED2
    gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2;
#else
    gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;
	}

	case 0xCA: /* JP Z, imm */
		if(gb->cpu_reg.f.f_bits.z)
		{
        uint8_t c = (uint8_t)(oppair >> 8);
				gb->cpu_reg.pc.reg++;
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg += 2;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xCB: /* CB INST */
		inst_cycles = __gb_execute_cb(gb);
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); // can cb change pc??? * revise
		//opcode = (uint8_t)(oppair >> 8); // things stopped working for megaman, rtype dx, etc with chaining when I made this change unless cgabmaactive was checked and opcode reloaded with gb_read
		break;

	case 0xCC: /* CALL Z, imm */
		if(gb->cpu_reg.f.f_bits.z)
		{
			uint8_t c = (uint8_t)(oppair >> 8);
			gb->cpu_reg.pc.reg++;
			uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
			__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
			__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
			gb->cpu_reg.pc.bytes.c = c;
			gb->cpu_reg.pc.bytes.p = p;
			inst_cycles += 12;
		}
		else
			gb->cpu_reg.pc.reg += 2;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xCD: /* CALL imm */
	{
		uint8_t c = (uint8_t)(oppair >> 8);
		gb->cpu_reg.pc.reg++;
		uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.bytes.c = c;
		gb->cpu_reg.pc.bytes.p = p;
	}
	opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
	break;

	case 0xCE: /* ADC A, imm */
	{
		uint8_t val = (uint8_t)(oppair >> 8);
	  gb->cpu_reg.pc.reg++;
		WGB_INSTR_ADC_R8(val, gb->cpu_reg.f.f_bits.c);
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;
	}

	case 0xCF: /* RST 0x0008 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0008;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xD0: /* RET NC */
    if (!gb->cpu_reg.f.f_bits.c)
    {
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
        gb->cpu_reg.sp.reg += 2; // advance SP past the popped PC
#else
        gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
        gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
        inst_cycles += 12;
    }
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xD1: /* POP DE */
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED
    gb->cpu_reg.de.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2;
#else
    gb->cpu_reg.de.bytes.e = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.de.bytes.d = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		opcode = (uint8_t)(oppair >> 8); 
		break;

	case 0xD2: /* JP NC, imm */
		if(!gb->cpu_reg.f.f_bits.c)
		{
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg += 2;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xD4: /* CALL NC, imm */
		if(!gb->cpu_reg.f.f_bits.c)
		{
        uint8_t c =(uint8_t)(oppair >> 8);
				gb->cpu_reg.pc.reg++;
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED
				gb->cpu_reg.sp.reg-=2;
				__gb_write16(gb, gb->cpu_reg.sp.reg, gb->cpu_reg.pc.reg);
#else
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
#endif
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
				inst_cycles += 12;
		}
		else
			gb->cpu_reg.pc.reg += 2;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xD5: /* PUSH DE */
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED
		gb->cpu_reg.sp.reg-=2;
		__gb_write16(gb, gb->cpu_reg.sp.reg, gb->cpu_reg.de.reg);
#else
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.de.bytes.d);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.de.bytes.e);
#endif
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xD6: /* SUB imm */
	{
		uint8_t val =(uint8_t)(oppair >> 8);
		gb->cpu_reg.pc.reg++;
		uint16_t temp = gb->cpu_reg.a - val;
		gb->cpu_reg.f.f_bits.z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.f.f_bits.n = 1;
		gb->cpu_reg.f.f_bits.h =
			(gb->cpu_reg.a ^ val ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;
	}

	case 0xD7: /* RST 0x0010 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0010;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xD8: /* RET C */
		if(gb->cpu_reg.f.f_bits.c)
		{
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
        gb->cpu_reg.sp.reg += 2; // advance SP past the popped PC
#else
        gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
        gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
			inst_cycles += 12;
			opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		}
		else
			opcode = (uint8_t)(oppair >> 8);

		break;

	case 0xD9: /* RETI */
	{
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED
    gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2; // advance SP past the popped PC
#else
    gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		gb->gb_ime = true;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
	}
	break;

	case 0xDA: /* JP C, imm */
		if(gb->cpu_reg.f.f_bits.c)
		{
        uint8_t c = (uint8_t)(oppair >> 8);
				gb->cpu_reg.pc.reg++;
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
				inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg += 2;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); 
		break;

	case 0xDC: /* CALL C, imm */
		if(gb->cpu_reg.f.f_bits.c)
		{
        uint8_t c = (uint8_t)(oppair >> 8);
				gb->cpu_reg.pc.reg++;
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED
				gb->cpu_reg.sp.reg-=2;
				__gb_write16(gb, gb->cpu_reg.sp.reg, gb->cpu_reg.pc.reg);
#else
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
#endif
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
				inst_cycles += 12;
		}
		else
			gb->cpu_reg.pc.reg += 2;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg); 
		break;

	case 0xDE: /* SBC A, imm */
	{
		uint8_t val = (uint8_t)(oppair >> 8);
		gb->cpu_reg.pc.reg++;
		WGB_INSTR_SBC_R8(val, gb->cpu_reg.f.f_bits.c);
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;
	}
	case 0xDF: /* RST 0x0018 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0018;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xE0: /* LD (0xFF00+imm), A */
		__gb_write(gb, 0xFF00 | (uint8_t)(oppair >> 8),
			   gb->cpu_reg.a);
		gb->cpu_reg.pc.reg++;
	  opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xE1: /* POP HL */
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED_DISABLED 
    gb->cpu_reg.hl.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2; // advance SP past the popped value
#else
    gb->cpu_reg.hl.bytes.l = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.hl.bytes.h = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xE2: /* LD (C), A */
		__gb_write(gb, 0xFF00 | gb->cpu_reg.bc.bytes.c, gb->cpu_reg.a);
#if WALNUT_GB_SAFE_DUALFETCH_OPCODES_DISABLED
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
#else
		opcode = (uint8_t)(oppair >> 8);
#endif
		break;

	case 0xE5: /* PUSH HL */
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED
		gb->cpu_reg.sp.reg-=2;
		__gb_write16(gb, gb->cpu_reg.sp.reg, gb->cpu_reg.hl.reg);
#else
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.hl.bytes.h);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.hl.bytes.l);
#endif
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xE6: /* AND imm */
	{
		uint8_t temp = (uint8_t)(oppair >> 8);
		gb->cpu_reg.pc.reg++;
		WGB_INSTR_AND_R8(temp);
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;
	}

	case 0xE7: /* RST 0x0020 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0020;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xE8: /* ADD SP, imm */
	{
		int8_t offset = (int8_t)(oppair >> 8);
		gb->cpu_reg.pc.reg++;
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.h = ((gb->cpu_reg.sp.reg & 0xF) + (offset & 0xF) > 0xF) ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = ((gb->cpu_reg.sp.reg & 0xFF) + (offset & 0xFF) > 0xFF);
		gb->cpu_reg.sp.reg += offset;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;
	}

	case 0xE9: /* JP (HL) */
		gb->cpu_reg.pc.reg = gb->cpu_reg.hl.reg;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xEA: /* LD (imm), A */
	{
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED
    uint8_t l = (uint8_t)(oppair >> 8);
		gb->cpu_reg.pc.reg++;
		oppair = __gb_read16(gb, gb->cpu_reg.pc.reg++);
    uint8_t h = oppair; // autotruncate
    uint16_t addr = WALNUT_GB_U8_TO_U16(h, l);
		__gb_write(gb, addr, gb->cpu_reg.a);
		opcode = oppair >> 8;	
#else
    uint8_t l = (uint8_t)(oppair >> 8);
		gb->cpu_reg.pc.reg++;
    uint8_t h = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint16_t addr = WALNUT_GB_U8_TO_U16(h, l);
		__gb_write(gb, addr, gb->cpu_reg.a);
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);		
#endif
		break;
	}

	case 0xEE: /* XOR imm */
		WGB_INSTR_XOR_R8((uint8_t)(oppair >> 8));
		gb->cpu_reg.pc.reg++;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xEF: /* RST 0x0028 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0028;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xF0: /* LD A, (0xFF00+imm) */
		gb->cpu_reg.a =
			__gb_read(gb, 0xFF00 | (uint8_t)(oppair >> 8));
			gb->cpu_reg.pc.reg++;
			opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xF1: /* POP AF */
	{
		uint8_t temp_8 = __gb_read(gb, gb->cpu_reg.sp.reg++);
		gb->cpu_reg.f.f_bits.z = (temp_8 >> 7) & 1;
		gb->cpu_reg.f.f_bits.n = (temp_8 >> 6) & 1;
		gb->cpu_reg.f.f_bits.h = (temp_8 >> 5) & 1;
		gb->cpu_reg.f.f_bits.c = (temp_8 >> 4) & 1;
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.sp.reg++);
		opcode = (uint8_t)(oppair >> 8);
		break;
	}

	case 0xF2: /* LD A, (C) */
		gb->cpu_reg.a = __gb_read(gb, 0xFF00 | gb->cpu_reg.bc.bytes.c);
#if WALNUT_GB_SAFE_DUALFETCH_OPCODES
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
#else
	opcode = (uint8_t)(oppair >> 8);
#endif
		break;

	case 0xF3: /* DI */
		gb->gb_ime = false;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xF5: /* PUSH AF */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.a);
		__gb_write(gb, --gb->cpu_reg.sp.reg,
			   gb->cpu_reg.f.f_bits.z << 7 | gb->cpu_reg.f.f_bits.n << 6 |
			   gb->cpu_reg.f.f_bits.h << 5 | gb->cpu_reg.f.f_bits.c << 4);
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xF6: /* OR imm */
		WGB_INSTR_OR_R8((uint8_t)(oppair >> 8));
		gb->cpu_reg.pc.reg++;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xF7: /* PUSH AF */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0030;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	case 0xF8: /* LD HL, SP+/-imm */
	{
		/* Taken from SameBoy, which is released under MIT Licence. */
		int8_t offset = (int8_t) (oppair >> 8);
		gb->cpu_reg.pc.reg++;
		gb->cpu_reg.hl.reg = gb->cpu_reg.sp.reg + offset;
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.h = ((gb->cpu_reg.sp.reg & 0xF) + (offset & 0xF) > 0xF) ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = ((gb->cpu_reg.sp.reg & 0xFF) + (offset & 0xFF) > 0xFF) ? 1 : 0;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;
	}

	case 0xF9: /* LD SP, HL */
		gb->cpu_reg.sp.reg = gb->cpu_reg.hl.reg;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xFA: /* LD A, (imm) */
	{
#if WALNUT_GB_16_BIT_OPS_DUALFETCH_DISABLED
    uint8_t l = (uint8_t)(oppair >> 8); // we dont increment pc because a interrupt might change it an invalidate this byte, we increment if no interrupt between 1st and 2nd opcode handlers
		gb->cpu_reg.pc.reg++;
		oppair = __gb_read16(gb, gb->cpu_reg.pc.reg++);
    uint8_t h = oppair; // auto-truncate
    uint16_t addr = WALNUT_GB_U8_TO_U16(h, l);
		gb->cpu_reg.a = __gb_read(gb, addr);
		opcode = oppair >> 8;
#else
    uint8_t l = (uint8_t)(oppair >> 8); // we dont increment pc because a interrupt might change it an invalidate this byte, we increment if no interrupt between 1st and 2nd opcode handlers
		gb->cpu_reg.pc.reg++;
    uint8_t h = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint16_t addr = WALNUT_GB_U8_TO_U16(h, l);
		gb->cpu_reg.a = __gb_read(gb, addr);
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
#endif
		break;
	}

	case 0xFB: /* EI */
		gb->gb_ime = true;
		opcode = (uint8_t)(oppair >> 8);
		break;

	case 0xFE: /* CP imm */
	{
		uint8_t val = (uint8_t)(oppair >> 8);
		gb->cpu_reg.pc.reg++;
		WGB_INSTR_CP_R8(val);
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;
	}

	case 0xFF: /* RST 0x0038 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0038;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);
		break;

	default:
		/* Return address where invalid opcode that was read. */
		(gb->gb_error)(gb, GB_INVALID_OPCODE, gb->cpu_reg.pc.reg - 1);
		WGB_UNREACHABLE();
		return;
	}

	do
	{
		/* DIV register timing */
		gb->counter.div_count += inst_cycles;
		while(gb->counter.div_count >= DIV_CYCLES)
		{
			gb->hram_io[IO_DIV]++;
			gb->counter.div_count -= DIV_CYCLES;
		}

		/* Check for RTC tick. */
		if(gb->mbc == 3 && (gb->rtc_real.reg.high & 0x40) == 0)
		{
			gb->counter.rtc_count += inst_cycles;
			while(WGB_UNLIKELY(gb->counter.rtc_count >= RTC_CYCLES))
			{
				gb->counter.rtc_count -= RTC_CYCLES;

				/* Detect invalid rollover. */
				if(WGB_UNLIKELY(gb->rtc_real.reg.sec == 63))
				{
					gb->rtc_real.reg.sec = 0;
					continue;
				}

				if(++gb->rtc_real.reg.sec != 60)
					continue;

				gb->rtc_real.reg.sec = 0;
				if(gb->rtc_real.reg.min == 63)
				{
					gb->rtc_real.reg.min = 0;
					continue;
				}
				if(++gb->rtc_real.reg.min != 60)
					continue;

				gb->rtc_real.reg.min = 0;
				if(gb->rtc_real.reg.hour == 31)
				{
					gb->rtc_real.reg.hour = 0;
					continue;
				}
				if(++gb->rtc_real.reg.hour != 24)
					continue;

				gb->rtc_real.reg.hour = 0;
				if(++gb->rtc_real.reg.yday != 0)
					continue;

				if(gb->rtc_real.reg.high & 1)  /* Bit 8 of days*/
					gb->rtc_real.reg.high |= 0x80; /* Overflow bit */

				gb->rtc_real.reg.high ^= 1;
			}
		}

		/* Check serial transmission. */
		if(gb->hram_io[IO_SC] & SERIAL_SC_TX_START)
		{
			unsigned int serial_cycles = SERIAL_CYCLES_1KB;

			/* If new transfer, call TX function. */
			if(gb->counter.serial_count == 0 &&
				gb->gb_serial_tx != NULL)
				(gb->gb_serial_tx)(gb, gb->hram_io[IO_SB]);

#if WALNUT_FULL_GBC_SUPPORT
			if(gb->hram_io[IO_SC] & 0x3)
				serial_cycles = SERIAL_CYCLES_32KB;
#endif

			gb->counter.serial_count += inst_cycles;

			/* If it's time to receive byte, call RX function. */
			if(gb->counter.serial_count >= serial_cycles)
			{
				/* If RX can be done, do it. */
				/* If RX failed, do not change SB if using external
				 * clock, or set to 0xFF if using internal clock. */
				uint8_t rx;

				if(gb->gb_serial_rx != NULL &&
					(gb->gb_serial_rx(gb, &rx) ==
						GB_SERIAL_RX_SUCCESS))
				{
					gb->hram_io[IO_SB] = rx;

					/* Inform game of serial TX/RX completion. */
					gb->hram_io[IO_SC] &= 0x01;
					gb->hram_io[IO_IF] |= SERIAL_INTR;
				}
				else if(gb->hram_io[IO_SC] & SERIAL_SC_CLOCK_SRC)
				{
					/* If using internal clock, and console is not
					 * attached to any external peripheral, shifted
					 * bits are replaced with logic 1. */
					gb->hram_io[IO_SB] = 0xFF;

					/* Inform game of serial TX/RX completion. */
					gb->hram_io[IO_SC] &= 0x01;
					gb->hram_io[IO_IF] |= SERIAL_INTR;
				}
				else
				{
					/* If using external clock, and console is not
					 * attached to any external peripheral, bits are
					 * not shifted, so SB is not modified. */
				}

				gb->counter.serial_count = 0;
			}
		}

		/* TIMA register timing */
		/* TODO: Change tac_enable to struct of TAC timer control bits. */
		if(gb->hram_io[IO_TAC] & IO_TAC_ENABLE_MASK)
		{
			gb->counter.tima_count += inst_cycles;

			while(gb->counter.tima_count >=
				TAC_CYCLES[gb->hram_io[IO_TAC] & IO_TAC_RATE_MASK])
			{
				gb->counter.tima_count -=
					TAC_CYCLES[gb->hram_io[IO_TAC] & IO_TAC_RATE_MASK];

				if(++gb->hram_io[IO_TIMA] == 0)
				{
					gb->hram_io[IO_IF] |= TIMER_INTR;
					/* On overflow, set TMA to TIMA. */
					gb->hram_io[IO_TIMA] = gb->hram_io[IO_TMA];
				}
			}
		}

		/* If LCD is off, don't update LCD state or increase the LCD
		 * ticks. Instead, keep track of the amount of time that is
		 * being passed. */
		if(!(gb->hram_io[IO_LCDC] & LCDC_ENABLE))
		{
			gb->counter.lcd_off_count += inst_cycles;
			if(gb->counter.lcd_off_count >= LCD_FRAME_CYCLES)
			{
				gb->counter.lcd_off_count -= LCD_FRAME_CYCLES;
				gb->gb_frame = true;
			}
			continue;
		}

		/* LCD Timing */
#if WALNUT_FULL_GBC_SUPPORT
        if (inst_cycles > 1) {
            gb->counter.lcd_count += (inst_cycles >> gb->cgb.doubleSpeed);
        } else {
#endif
		gb->counter.lcd_count += inst_cycles;
#if WALNUT_FULL_GBC_SUPPORT
	}
#endif

		/* New Scanline. HBlank -> VBlank or OAM Scan */
		if(gb->counter.lcd_count >= LCD_LINE_CYCLES)
		{
			gb->counter.lcd_count -= LCD_LINE_CYCLES;

			/* Next line */
			gb->hram_io[IO_LY] = gb->hram_io[IO_LY] + 1;
			if (gb->hram_io[IO_LY] == LCD_VERT_LINES)
				gb->hram_io[IO_LY] = 0;

			/* LYC Update */
			if(gb->hram_io[IO_LY] == gb->hram_io[IO_LYC])
			{
				gb->hram_io[IO_STAT] |= STAT_LYC_COINC;

				if(gb->hram_io[IO_STAT] & STAT_LYC_INTR)
					gb->hram_io[IO_IF] |= LCDC_INTR;
			}
			else
				gb->hram_io[IO_STAT] &= 0xFB;

			/* Check if LCD should be in Mode 1 (VBLANK) state */
			if(gb->hram_io[IO_LY] == LCD_HEIGHT)
			{
				gb->hram_io[IO_STAT] =
					(gb->hram_io[IO_STAT] & ~STAT_MODE) | IO_STAT_MODE_VBLANK;
				gb->gb_frame = true;
				gb->hram_io[IO_IF] |= VBLANK_INTR;
				gb->lcd_blank = false;

				if(gb->hram_io[IO_STAT] & STAT_MODE_1_INTR)
					gb->hram_io[IO_IF] |= LCDC_INTR;

#if ENABLE_LCD
				/* If frame skip is activated, check if we need to draw
				 * the frame or skip it. */
				if(gb->direct.frame_skip)
				{
					gb->display.frame_skip_count =
						!gb->display.frame_skip_count;
				}

				/* If interlaced is activated, change which lines get
				 * updated. Also, only update lines on frames that are
				 * actually drawn when frame skip is enabled. */
				if(gb->direct.interlace &&
						(!gb->direct.frame_skip ||
						 gb->display.frame_skip_count))
				{
					gb->display.interlace_count =
						!gb->display.interlace_count;
				}
#endif
                                /* If halted forever, then return on VBLANK. */
                                if(gb->gb_halt && !gb->hram_io[IO_IE])
					break;
			}
			/* Start of normal Line (not in VBLANK) */
			else if(gb->hram_io[IO_LY] < LCD_HEIGHT)
			{
				if(gb->hram_io[IO_LY] == 0)
				{
					/* Clear Screen */
					gb->display.WY = gb->hram_io[IO_WY];
					gb->display.window_clear = 0;
				}

				/* OAM Search occurs at the start of the line. */
				gb->hram_io[IO_STAT] = (gb->hram_io[IO_STAT] & ~STAT_MODE) | IO_STAT_MODE_OAM_SCAN;
				gb->counter.lcd_count = 0;

#if WALNUT_FULL_GBC_SUPPORT
				//DMA GBC
				if(gb->cgb.cgbMode && !gb->cgb.dmaActive && gb->cgb.dmaMode)
				{
#if WALNUT_GB_32BIT_DMA
					// Optimized 16-bit path
					for (uint8_t i = 0; i < 0x10; i += 4)
					{
							uint32_t val = __gb_read32(gb, (gb->cgb.dmaSource & 0xFFF0) + i);
							__gb_write32(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i, val);
							// 8-bit logic if there is some cause to fall back
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i, val);
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i + 1, val >> 8);
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i + 2, val >> 16);
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i + 3, val >> 24);
					}
#elif WALNUT_GB_16BIT_DMA
					// Optimized 16-bit path
					for (uint8_t i = 0; i < 0x10; i += 2)
					{
							uint16_t val = __gb_read16(gb, (gb->cgb.dmaSource & 0xFFF0) + i);
							__gb_write16(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i, val);
							// 8-bit logic if there is some cause to fall back
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i, val & 0xFF);
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i + 1, val >> 8);
					}
#else
			    // Original 8-bit path
					for (uint8_t i = 0; i < 0x10; i++)
					{
						__gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i,
										__gb_read(gb, (gb->cgb.dmaSource & 0xFFF0) + i));
					}
#endif

					gb->cgb.dmaSource += 0x10;
					gb->cgb.dmaDest += 0x10;
					if(!(--gb->cgb.dmaSize)) {gb->cgb.dmaActive = 1;
					}
#if WALNUT_GB_SAFE_DUALFETCH_DMA
					gb->prefetch_invalid=true;
#endif
				}
#endif
				if(gb->hram_io[IO_STAT] & STAT_MODE_2_INTR)
					gb->hram_io[IO_IF] |= LCDC_INTR;

				/* If halted immediately jump to next LCD mode.
				 * From OAM Search to LCD Draw. */
				//if(gb->counter.lcd_count < LCD_MODE2_OAM_SCAN_END)
				//	inst_cycles = LCD_MODE2_OAM_SCAN_END - gb->counter.lcd_count;
				inst_cycles = LCD_MODE2_OAM_SCAN_DURATION;
			}
		}
		/* Go from Mode 3 (LCD Draw) to Mode 0 (HBLANK). */
		else if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_LCD_DRAW &&
				gb->counter.lcd_count >= LCD_MODE3_LCD_DRAW_END)
		{
			gb->hram_io[IO_STAT] = (gb->hram_io[IO_STAT] & ~STAT_MODE) | IO_STAT_MODE_HBLANK;

			if(gb->hram_io[IO_STAT] & STAT_MODE_0_INTR)
				gb->hram_io[IO_IF] |= LCDC_INTR;

			/* If halted immediately, jump from OAM Scan to LCD Draw. */
			if (gb->counter.lcd_count < LCD_MODE0_HBLANK_MAX_DRUATION)
				inst_cycles = LCD_MODE0_HBLANK_MAX_DRUATION - gb->counter.lcd_count;
		}
		/* Go from Mode 2 (OAM Scan) to Mode 3 (LCD Draw). */
		else if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_OAM_SCAN &&
				gb->counter.lcd_count >= LCD_MODE2_OAM_SCAN_END)
		{
			gb->hram_io[IO_STAT] = (gb->hram_io[IO_STAT] & ~STAT_MODE) | IO_STAT_MODE_LCD_DRAW;
#if ENABLE_LCD
			if(!gb->lcd_blank)
				__gb_draw_line(gb);
#endif
			/* If halted immediately jump to next LCD mode. */
			if (gb->counter.lcd_count < LCD_MODE3_LCD_DRAW_MIN_DURATION)
				inst_cycles = LCD_MODE3_LCD_DRAW_MIN_DURATION - gb->counter.lcd_count;
		}
	} while(gb->gb_halt && (gb->hram_io[IO_IF] & gb->hram_io[IO_IE]) == 0);
	/* If halted, loop until an interrupt occurs. */

	// **** 2nd instruction processing ****
#if (WALNUT_GB_SAFE_DUALFETCH_DMA || WALNUT_GB_SAFE_DUALFETCH_MBC)
	if (gb->prefetch_invalid)
	{
		gb->prefetch_invalid=false;
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);// opcode was invalidated so we reload
	}
#endif
	/* Handle interrupts */
	/* If gb_halt is positive, then an interrupt must have occurred by the
	 * time we reach here, because on HALT, we jump to the next interrupt
	 * immediately. We also load opcode here because it may be replacing the previously loaded one from the dual fetch logic's first pass */
	while(gb->gb_halt || (gb->gb_ime &&
			gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & ANY_INTR))
	{
		gb->gb_halt = false;

		if(!gb->gb_ime)
			break;

		/* Disable interrupts */
		gb->gb_ime = false;

		/* Push Program Counter */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);

		/* Call interrupt handler if required. */
		if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & VBLANK_INTR)
		{
			gb->cpu_reg.pc.reg = VBLANK_INTR_ADDR;
			gb->hram_io[IO_IF] ^= VBLANK_INTR;
		}
		else if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & LCDC_INTR)
		{
			gb->cpu_reg.pc.reg = LCDC_INTR_ADDR;
			gb->hram_io[IO_IF] ^= LCDC_INTR;
		}
		else if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & TIMER_INTR)
		{
			gb->cpu_reg.pc.reg = TIMER_INTR_ADDR;
			gb->hram_io[IO_IF] ^= TIMER_INTR;
		}
		else if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & SERIAL_INTR)
		{
			gb->cpu_reg.pc.reg = SERIAL_INTR_ADDR;
			gb->hram_io[IO_IF] ^= SERIAL_INTR;
		}
		else if(gb->hram_io[IO_IF] & gb->hram_io[IO_IE] & CONTROL_INTR)
		{
			gb->cpu_reg.pc.reg = CONTROL_INTR_ADDR;			
			gb->hram_io[IO_IF] ^= CONTROL_INTR;
		}
		
		opcode = __gb_read(gb, gb->cpu_reg.pc.reg);

		break;
	}
	// at this point opcode always has the next instruction but pc hasn't yet been incremented so we increment here collectively for all cases
	// that can lead us to this point.
	gb->cpu_reg.pc.reg++;
	inst_cycles = op_cycles[opcode];
	/* Execute opcode */
	switch(opcode)
	{
	case 0x00: /* NOP */
		break;

	case 0x01: /* LD BC, imm */
#if WALNUT_GB_16_BIT_OPS
    gb->cpu_reg.bc.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2;
#else
    gb->cpu_reg.bc.bytes.c = __gb_read(gb, gb->cpu_reg.pc.reg++);
    gb->cpu_reg.bc.bytes.b = __gb_read(gb, gb->cpu_reg.pc.reg++);
#endif
		break;
	case 0x02: /* LD (BC), A */
		__gb_write(gb, gb->cpu_reg.bc.reg, gb->cpu_reg.a);
		break;

	case 0x03: /* INC BC */
		gb->cpu_reg.bc.reg++;
		break;

	case 0x04: /* INC B */
		WGB_INSTR_INC_R8(gb->cpu_reg.bc.bytes.b);
		break;

	case 0x05: /* DEC B */
		WGB_INSTR_DEC_R8(gb->cpu_reg.bc.bytes.b);
		break;

	case 0x06: /* LD B, imm */
		gb->cpu_reg.bc.bytes.b = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x07: /* RLCA */
		gb->cpu_reg.a = (gb->cpu_reg.a << 1) | (gb->cpu_reg.a >> 7);
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.c = (gb->cpu_reg.a & 0x01);
		break;

	case 0x08: /* LD (imm), SP */
	{
#if WALNUT_GB_16_BIT_OPS
    uint16_t addr = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2;
    __gb_write16(gb, addr, gb->cpu_reg.sp.reg);
#else
    uint8_t l = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint8_t h = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint16_t addr = WALNUT_GB_U8_TO_U16(h, l);
    __gb_write(gb, addr++, gb->cpu_reg.sp.bytes.p);
    __gb_write(gb, addr, gb->cpu_reg.sp.bytes.s);
#endif
    break;
	}

	case 0x09: /* ADD HL, BC */
	{
		uint_fast32_t temp = gb->cpu_reg.hl.reg + gb->cpu_reg.bc.reg;
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h =
			(temp ^ gb->cpu_reg.hl.reg ^ gb->cpu_reg.bc.reg) & 0x1000 ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = (temp & 0xFFFF0000) ? 1 : 0;
		gb->cpu_reg.hl.reg = (temp & 0x0000FFFF);
		break;
	}

	case 0x0A: /* LD A, (BC) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.bc.reg);
		break;

	case 0x0B: /* DEC BC */
		gb->cpu_reg.bc.reg--;
		break;

	case 0x0C: /* INC C */
		WGB_INSTR_INC_R8(gb->cpu_reg.bc.bytes.c);
		break;

	case 0x0D: /* DEC C */
		WGB_INSTR_DEC_R8(gb->cpu_reg.bc.bytes.c);
		break;

	case 0x0E: /* LD C, imm */
		gb->cpu_reg.bc.bytes.c = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x0F: /* RRCA */
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.c = gb->cpu_reg.a & 0x01;
		gb->cpu_reg.a = (gb->cpu_reg.a >> 1) | (gb->cpu_reg.a << 7);
		break;

	case 0x10: /* STOP */
		//gb->gb_halt = true;
#if WALNUT_FULL_GBC_SUPPORT
		if(gb->cgb.cgbMode & gb->cgb.doubleSpeedPrep)
		{
			gb->cgb.doubleSpeedPrep = 0;
			gb->cgb.doubleSpeed ^= 1;
		}
#endif
		break;

	case 0x11: /* LD DE, imm */
#if WALNUT_GB_16_BIT_OPS
    gb->cpu_reg.de.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2;
#else
    gb->cpu_reg.de.bytes.e = __gb_read(gb, gb->cpu_reg.pc.reg++);
    gb->cpu_reg.de.bytes.d = __gb_read(gb, gb->cpu_reg.pc.reg++);
#endif
		break;

	case 0x12: /* LD (DE), A */
		__gb_write(gb, gb->cpu_reg.de.reg, gb->cpu_reg.a);
		break;

	case 0x13: /* INC DE */
		gb->cpu_reg.de.reg++;
		break;

	case 0x14: /* INC D */
		WGB_INSTR_INC_R8(gb->cpu_reg.de.bytes.d);
		break;

	case 0x15: /* DEC D */
		WGB_INSTR_DEC_R8(gb->cpu_reg.de.bytes.d);
		break;

	case 0x16: /* LD D, imm */
		gb->cpu_reg.de.bytes.d = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x17: /* RLA */
	{
		uint8_t temp = gb->cpu_reg.a;
		gb->cpu_reg.a = (gb->cpu_reg.a << 1) | gb->cpu_reg.f.f_bits.c;
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.c = (temp >> 7) & 0x01;
		break;
	}

	case 0x18: /* JR imm */
	{
		int8_t temp = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
		gb->cpu_reg.pc.reg += temp;
		break;
	}

	case 0x19: /* ADD HL, DE */
	{
		uint_fast32_t temp = gb->cpu_reg.hl.reg + gb->cpu_reg.de.reg;
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h =
			(temp ^ gb->cpu_reg.hl.reg ^ gb->cpu_reg.de.reg) & 0x1000 ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = (temp & 0xFFFF0000) ? 1 : 0;
		gb->cpu_reg.hl.reg = (temp & 0x0000FFFF);
		break;
	}

	case 0x1A: /* LD A, (DE) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.de.reg);
		break;

	case 0x1B: /* DEC DE */
		gb->cpu_reg.de.reg--;
		break;

	case 0x1C: /* INC E */
		WGB_INSTR_INC_R8(gb->cpu_reg.de.bytes.e);
		break;

	case 0x1D: /* DEC E */
		WGB_INSTR_DEC_R8(gb->cpu_reg.de.bytes.e);
		break;

	case 0x1E: /* LD E, imm */
		gb->cpu_reg.de.bytes.e = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x1F: /* RRA */
	{
		uint8_t temp = gb->cpu_reg.a;
		gb->cpu_reg.a = gb->cpu_reg.a >> 1 | (gb->cpu_reg.f.f_bits.c << 7);
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.c = temp & 0x1;
		break;
	}

	case 0x20: /* JR NZ, imm */
		if(!gb->cpu_reg.f.f_bits.z)
		{
			int8_t temp = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
			gb->cpu_reg.pc.reg += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg++;

		break;

	case 0x21: /* LD HL, imm */
#if WALNUT_GB_16_BIT_OPS
    gb->cpu_reg.hl.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2;
#else
    gb->cpu_reg.hl.bytes.l = __gb_read(gb, gb->cpu_reg.pc.reg++);
    gb->cpu_reg.hl.bytes.h = __gb_read(gb, gb->cpu_reg.pc.reg++);
#endif
		break;

	case 0x22: /* LDI (HL), A */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.a);
		gb->cpu_reg.hl.reg++;
		break;

	case 0x23: /* INC HL */
		gb->cpu_reg.hl.reg++;
		break;

	case 0x24: /* INC H */
		WGB_INSTR_INC_R8(gb->cpu_reg.hl.bytes.h);
		break;

	case 0x25: /* DEC H */
		WGB_INSTR_DEC_R8(gb->cpu_reg.hl.bytes.h);
		break;

	case 0x26: /* LD H, imm */
		gb->cpu_reg.hl.bytes.h = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x27: /* DAA */
	{
		/* The following is from SameBoy. MIT License. */
		int16_t a = gb->cpu_reg.a;

		if(gb->cpu_reg.f.f_bits.n)
		{
			if(gb->cpu_reg.f.f_bits.h)
				a = (a - 0x06) & 0xFF;

			if(gb->cpu_reg.f.f_bits.c)
				a -= 0x60;
		}
		else
		{
			if(gb->cpu_reg.f.f_bits.h || (a & 0x0F) > 9)
				a += 0x06;

			if(gb->cpu_reg.f.f_bits.c || a > 0x9F)
				a += 0x60;
		}

		if((a & 0x100) == 0x100)
			gb->cpu_reg.f.f_bits.c = 1;

		gb->cpu_reg.a = (uint8_t)a;
		gb->cpu_reg.f.f_bits.z = (gb->cpu_reg.a == 0);
		gb->cpu_reg.f.f_bits.h = 0;

		break;
	}

	case 0x28: /* JR Z, imm */
		if(gb->cpu_reg.f.f_bits.z)
		{
			int8_t temp = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
			gb->cpu_reg.pc.reg += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg++;

		break;

	case 0x29: /* ADD HL, HL */
	{
		gb->cpu_reg.f.f_bits.c = (gb->cpu_reg.hl.reg & 0x8000) > 0;
		gb->cpu_reg.hl.reg <<= 1;
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h = (gb->cpu_reg.hl.reg & 0x1000) > 0;
		break;
	}

	case 0x2A: /* LD A, (HL+) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.hl.reg++);
		break;

	case 0x2B: /* DEC HL */
		gb->cpu_reg.hl.reg--;
		break;

	case 0x2C: /* INC L */
		WGB_INSTR_INC_R8(gb->cpu_reg.hl.bytes.l);
		break;

	case 0x2D: /* DEC L */
		WGB_INSTR_DEC_R8(gb->cpu_reg.hl.bytes.l);
		break;

	case 0x2E: /* LD L, imm */
		gb->cpu_reg.hl.bytes.l = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x2F: /* CPL */
		gb->cpu_reg.a = ~gb->cpu_reg.a;
		gb->cpu_reg.f.f_bits.n = 1;
		gb->cpu_reg.f.f_bits.h = 1;
		break;

	case 0x30: /* JR NC, imm */
		if(!gb->cpu_reg.f.f_bits.c)
		{
			int8_t temp = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
			gb->cpu_reg.pc.reg += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg++;

		break;

	case 0x31: /* LD SP, imm */
#if WALNUT_GB_16_BIT_OPS
    gb->cpu_reg.sp.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2;
#else
    gb->cpu_reg.sp.bytes.p = __gb_read(gb, gb->cpu_reg.pc.reg++);
    gb->cpu_reg.sp.bytes.s = __gb_read(gb, gb->cpu_reg.pc.reg++);
#endif
		break;

	case 0x32: /* LD (HL), A */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.a);
		gb->cpu_reg.hl.reg--;
		break;

	case 0x33: /* INC SP */
		gb->cpu_reg.sp.reg++;
		break;

	case 0x34: /* INC (HL) */
	{
		uint8_t temp = __gb_read(gb, gb->cpu_reg.hl.reg);
		WGB_INSTR_INC_R8(temp);
		__gb_write(gb, gb->cpu_reg.hl.reg, temp);
		break;
	}

	case 0x35: /* DEC (HL) */
	{
		uint8_t temp = __gb_read(gb, gb->cpu_reg.hl.reg);
		WGB_INSTR_DEC_R8(temp);
		__gb_write(gb, gb->cpu_reg.hl.reg, temp);
		break;
	}

	case 0x36: /* LD (HL), imm */
		__gb_write(gb, gb->cpu_reg.hl.reg, __gb_read(gb, gb->cpu_reg.pc.reg++));
		break;

	case 0x37: /* SCF */
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h = 0;
		gb->cpu_reg.f.f_bits.c = 1;
		break;

	case 0x38: /* JR C, imm */
		if(gb->cpu_reg.f.f_bits.c)
		{
			int8_t temp = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
			gb->cpu_reg.pc.reg += temp;
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg++;

		break;

	case 0x39: /* ADD HL, SP */
	{
		uint_fast32_t temp = gb->cpu_reg.hl.reg + gb->cpu_reg.sp.reg;
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h =
			((gb->cpu_reg.hl.reg & 0xFFF) + (gb->cpu_reg.sp.reg & 0xFFF)) & 0x1000 ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = temp & 0x10000 ? 1 : 0;
		gb->cpu_reg.hl.reg = (uint16_t)temp;
		break;
	}

	case 0x3A: /* LD A, (HL) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.hl.reg--);
		break;

	case 0x3B: /* DEC SP */
		gb->cpu_reg.sp.reg--;
		break;

	case 0x3C: /* INC A */
		WGB_INSTR_INC_R8(gb->cpu_reg.a);
		break;

	case 0x3D: /* DEC A */
		WGB_INSTR_DEC_R8(gb->cpu_reg.a);
		break;

	case 0x3E: /* LD A, imm */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.pc.reg++);
		break;

	case 0x3F: /* CCF */
		gb->cpu_reg.f.f_bits.n = 0;
		gb->cpu_reg.f.f_bits.h = 0;
		gb->cpu_reg.f.f_bits.c = ~gb->cpu_reg.f.f_bits.c;
		break;

	case 0x40: /* LD B, B */
		break;

	case 0x41: /* LD B, C */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.bc.bytes.c;
		break;

	case 0x42: /* LD B, D */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.de.bytes.d;
		break;

	case 0x43: /* LD B, E */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.de.bytes.e;
		break;

	case 0x44: /* LD B, H */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.hl.bytes.h;
		break;

	case 0x45: /* LD B, L */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.hl.bytes.l;
		break;

	case 0x46: /* LD B, (HL) */
		gb->cpu_reg.bc.bytes.b = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x47: /* LD B, A */
		gb->cpu_reg.bc.bytes.b = gb->cpu_reg.a;
		break;

	case 0x48: /* LD C, B */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.bc.bytes.b;
		break;

	case 0x49: /* LD C, C */
		break;

	case 0x4A: /* LD C, D */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.de.bytes.d;
		break;

	case 0x4B: /* LD C, E */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.de.bytes.e;
		break;

	case 0x4C: /* LD C, H */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.hl.bytes.h;
		break;

	case 0x4D: /* LD C, L */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.hl.bytes.l;
		break;

	case 0x4E: /* LD C, (HL) */
		gb->cpu_reg.bc.bytes.c = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x4F: /* LD C, A */
		gb->cpu_reg.bc.bytes.c = gb->cpu_reg.a;
		break;

	case 0x50: /* LD D, B */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.bc.bytes.b;
		break;

	case 0x51: /* LD D, C */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.bc.bytes.c;
		break;

	case 0x52: /* LD D, D */
		break;

	case 0x53: /* LD D, E */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.de.bytes.e;
		break;

	case 0x54: /* LD D, H */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.hl.bytes.h;
		break;

	case 0x55: /* LD D, L */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.hl.bytes.l;
		break;

	case 0x56: /* LD D, (HL) */
		gb->cpu_reg.de.bytes.d = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x57: /* LD D, A */
		gb->cpu_reg.de.bytes.d = gb->cpu_reg.a;
		break;

	case 0x58: /* LD E, B */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.bc.bytes.b;
		break;

	case 0x59: /* LD E, C */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.bc.bytes.c;
		break;

	case 0x5A: /* LD E, D */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.de.bytes.d;
		break;

	case 0x5B: /* LD E, E */
		break;

	case 0x5C: /* LD E, H */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.hl.bytes.h;
		break;

	case 0x5D: /* LD E, L */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.hl.bytes.l;
		break;

	case 0x5E: /* LD E, (HL) */
		gb->cpu_reg.de.bytes.e = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x5F: /* LD E, A */
		gb->cpu_reg.de.bytes.e = gb->cpu_reg.a;
		break;

	case 0x60: /* LD H, B */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.bc.bytes.b;
		break;

	case 0x61: /* LD H, C */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.bc.bytes.c;
		break;

	case 0x62: /* LD H, D */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.de.bytes.d;
		break;

	case 0x63: /* LD H, E */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.de.bytes.e;
		break;

	case 0x64: /* LD H, H */
		break;

	case 0x65: /* LD H, L */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.hl.bytes.l;
		break;

	case 0x66: /* LD H, (HL) */
		gb->cpu_reg.hl.bytes.h = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x67: /* LD H, A */
		gb->cpu_reg.hl.bytes.h = gb->cpu_reg.a;
		break;

	case 0x68: /* LD L, B */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.bc.bytes.b;
		break;

	case 0x69: /* LD L, C */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.bc.bytes.c;
		break;

	case 0x6A: /* LD L, D */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.de.bytes.d;
		break;

	case 0x6B: /* LD L, E */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.de.bytes.e;
		break;

	case 0x6C: /* LD L, H */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.hl.bytes.h;
		break;

	case 0x6D: /* LD L, L */
		break;

	case 0x6E: /* LD L, (HL) */
		gb->cpu_reg.hl.bytes.l = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x6F: /* LD L, A */
		gb->cpu_reg.hl.bytes.l = gb->cpu_reg.a;
		break;

	case 0x70: /* LD (HL), B */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.bc.bytes.b);
		break;

	case 0x71: /* LD (HL), C */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.bc.bytes.c);
		break;

	case 0x72: /* LD (HL), D */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.de.bytes.d);
		break;

	case 0x73: /* LD (HL), E */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.de.bytes.e);
		break;

	case 0x74: /* LD (HL), H */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.hl.bytes.h);
		break;

	case 0x75: /* LD (HL), L */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.hl.bytes.l);
		break;

	case 0x76: /* HALT */
	{
		int_fast16_t halt_cycles = INT_FAST16_MAX;

		/* TODO: Emulate HALT bug? */
		gb->gb_halt = true;

		if(gb->hram_io[IO_SC] & SERIAL_SC_TX_START)
		{
			int serial_cycles = SERIAL_CYCLES -
				gb->counter.serial_count;

			if(serial_cycles < halt_cycles)
				halt_cycles = serial_cycles;
		}

		if(gb->hram_io[IO_TAC] & IO_TAC_ENABLE_MASK)
		{
			int tac_cycles = TAC_CYCLES[gb->hram_io[IO_TAC] & IO_TAC_RATE_MASK] -
				gb->counter.tima_count;

			if(tac_cycles < halt_cycles)
				halt_cycles = tac_cycles;
		}

		if((gb->hram_io[IO_LCDC] & LCDC_ENABLE))
		{
			int lcd_cycles;

			/* If LCD is in HBlank, calculate the number of cycles
			 * until the end of HBlank and the start of mode 2 or
			 * mode 1. */
			if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_HBLANK)
			{
				lcd_cycles = LCD_MODE0_HBLANK_MAX_DRUATION - gb->counter.lcd_count;
			}
			else if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_OAM_SCAN)
			{
				lcd_cycles = LCD_MODE3_LCD_DRAW_MIN_DURATION - gb->counter.lcd_count;
			}
			else if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_LCD_DRAW)
			{
				lcd_cycles = LCD_MODE0_HBLANK_MAX_DRUATION - gb->counter.lcd_count;
			}
			else
			{
				/* VBlank */
				lcd_cycles = LCD_LINE_CYCLES - gb->counter.lcd_count;
			}

			if(lcd_cycles < halt_cycles)
				halt_cycles = lcd_cycles;
		}

		/* Some halt cycles may already be very high, so make sure we
		 * don't underflow here. */
		if(halt_cycles <= 0)
			halt_cycles = 4;

		inst_cycles = (uint_fast16_t)halt_cycles;
		break;
	}

	case 0x77: /* LD (HL), A */
		__gb_write(gb, gb->cpu_reg.hl.reg, gb->cpu_reg.a);
		break;

	case 0x78: /* LD A, B */
		gb->cpu_reg.a = gb->cpu_reg.bc.bytes.b;
		break;

	case 0x79: /* LD A, C */
		gb->cpu_reg.a = gb->cpu_reg.bc.bytes.c;
		break;

	case 0x7A: /* LD A, D */
		gb->cpu_reg.a = gb->cpu_reg.de.bytes.d;
		break;

	case 0x7B: /* LD A, E */
		gb->cpu_reg.a = gb->cpu_reg.de.bytes.e;
		break;

	case 0x7C: /* LD A, H */
		gb->cpu_reg.a = gb->cpu_reg.hl.bytes.h;
		break;

	case 0x7D: /* LD A, L */
		gb->cpu_reg.a = gb->cpu_reg.hl.bytes.l;
		break;

	case 0x7E: /* LD A, (HL) */
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.hl.reg);
		break;

	case 0x7F: /* LD A, A */
		break;

	case 0x80: /* ADD A, B */
		WGB_INSTR_ADC_R8(gb->cpu_reg.bc.bytes.b, 0);
		break;

	case 0x81: /* ADD A, C */
		WGB_INSTR_ADC_R8(gb->cpu_reg.bc.bytes.c, 0);
		break;

	case 0x82: /* ADD A, D */
		WGB_INSTR_ADC_R8(gb->cpu_reg.de.bytes.d, 0);
		break;

	case 0x83: /* ADD A, E */
		WGB_INSTR_ADC_R8(gb->cpu_reg.de.bytes.e, 0);
		break;

	case 0x84: /* ADD A, H */
		WGB_INSTR_ADC_R8(gb->cpu_reg.hl.bytes.h, 0);
		break;

	case 0x85: /* ADD A, L */
		WGB_INSTR_ADC_R8(gb->cpu_reg.hl.bytes.l, 0);
		break;

	case 0x86: /* ADD A, (HL) */
		WGB_INSTR_ADC_R8(__gb_read(gb, gb->cpu_reg.hl.reg), 0);
		break;

	case 0x87: /* ADD A, A */
		WGB_INSTR_ADC_R8(gb->cpu_reg.a, 0);
		break;

	case 0x88: /* ADC A, B */
		WGB_INSTR_ADC_R8(gb->cpu_reg.bc.bytes.b, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x89: /* ADC A, C */
		WGB_INSTR_ADC_R8(gb->cpu_reg.bc.bytes.c, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x8A: /* ADC A, D */
		WGB_INSTR_ADC_R8(gb->cpu_reg.de.bytes.d, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x8B: /* ADC A, E */
		WGB_INSTR_ADC_R8(gb->cpu_reg.de.bytes.e, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x8C: /* ADC A, H */
		WGB_INSTR_ADC_R8(gb->cpu_reg.hl.bytes.h, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x8D: /* ADC A, L */
		WGB_INSTR_ADC_R8(gb->cpu_reg.hl.bytes.l, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x8E: /* ADC A, (HL) */
		WGB_INSTR_ADC_R8(__gb_read(gb, gb->cpu_reg.hl.reg), gb->cpu_reg.f.f_bits.c);
		break;

	case 0x8F: /* ADC A, A */
		WGB_INSTR_ADC_R8(gb->cpu_reg.a, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x90: /* SUB B */
		WGB_INSTR_SBC_R8(gb->cpu_reg.bc.bytes.b, 0);
		break;

	case 0x91: /* SUB C */
		WGB_INSTR_SBC_R8(gb->cpu_reg.bc.bytes.c, 0);
		break;

	case 0x92: /* SUB D */
		WGB_INSTR_SBC_R8(gb->cpu_reg.de.bytes.d, 0);
		break;

	case 0x93: /* SUB E */
		WGB_INSTR_SBC_R8(gb->cpu_reg.de.bytes.e, 0);
		break;

	case 0x94: /* SUB H */
		WGB_INSTR_SBC_R8(gb->cpu_reg.hl.bytes.h, 0);
		break;

	case 0x95: /* SUB L */
		WGB_INSTR_SBC_R8(gb->cpu_reg.hl.bytes.l, 0);
		break;

	case 0x96: /* SUB (HL) */
		WGB_INSTR_SBC_R8(__gb_read(gb, gb->cpu_reg.hl.reg), 0);
		break;

	case 0x97: /* SUB A */
		gb->cpu_reg.a = 0;
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.z = 1;
		gb->cpu_reg.f.f_bits.n = 1;
		break;

	case 0x98: /* SBC A, B */
		WGB_INSTR_SBC_R8(gb->cpu_reg.bc.bytes.b, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x99: /* SBC A, C */
		WGB_INSTR_SBC_R8(gb->cpu_reg.bc.bytes.c, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x9A: /* SBC A, D */
		WGB_INSTR_SBC_R8(gb->cpu_reg.de.bytes.d, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x9B: /* SBC A, E */
		WGB_INSTR_SBC_R8(gb->cpu_reg.de.bytes.e, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x9C: /* SBC A, H */
		WGB_INSTR_SBC_R8(gb->cpu_reg.hl.bytes.h, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x9D: /* SBC A, L */
		WGB_INSTR_SBC_R8(gb->cpu_reg.hl.bytes.l, gb->cpu_reg.f.f_bits.c);
		break;

	case 0x9E: /* SBC A, (HL) */
		WGB_INSTR_SBC_R8(__gb_read(gb, gb->cpu_reg.hl.reg), gb->cpu_reg.f.f_bits.c);
		break;

	case 0x9F: /* SBC A, A */
		gb->cpu_reg.a = gb->cpu_reg.f.f_bits.c ? 0xFF : 0x00;
		gb->cpu_reg.f.f_bits.z = !gb->cpu_reg.f.f_bits.c;
		gb->cpu_reg.f.f_bits.n = 1;
		gb->cpu_reg.f.f_bits.h = gb->cpu_reg.f.f_bits.c;
		break;

	case 0xA0: /* AND B */
		WGB_INSTR_AND_R8(gb->cpu_reg.bc.bytes.b);
		break;

	case 0xA1: /* AND C */
		WGB_INSTR_AND_R8(gb->cpu_reg.bc.bytes.c);
		break;

	case 0xA2: /* AND D */
		WGB_INSTR_AND_R8(gb->cpu_reg.de.bytes.d);
		break;

	case 0xA3: /* AND E */
		WGB_INSTR_AND_R8(gb->cpu_reg.de.bytes.e);
		break;

	case 0xA4: /* AND H */
		WGB_INSTR_AND_R8(gb->cpu_reg.hl.bytes.h);
		break;

	case 0xA5: /* AND L */
		WGB_INSTR_AND_R8(gb->cpu_reg.hl.bytes.l);
		break;

	case 0xA6: /* AND (HL) */
		WGB_INSTR_AND_R8(__gb_read(gb, gb->cpu_reg.hl.reg));
		break;

	case 0xA7: /* AND A */
		WGB_INSTR_AND_R8(gb->cpu_reg.a);
		break;

	case 0xA8: /* XOR B */
		WGB_INSTR_XOR_R8(gb->cpu_reg.bc.bytes.b);
		break;

	case 0xA9: /* XOR C */
		WGB_INSTR_XOR_R8(gb->cpu_reg.bc.bytes.c);
		break;

	case 0xAA: /* XOR D */
		WGB_INSTR_XOR_R8(gb->cpu_reg.de.bytes.d);
		break;

	case 0xAB: /* XOR E */
		WGB_INSTR_XOR_R8(gb->cpu_reg.de.bytes.e);
		break;

	case 0xAC: /* XOR H */
		WGB_INSTR_XOR_R8(gb->cpu_reg.hl.bytes.h);
		break;

	case 0xAD: /* XOR L */
		WGB_INSTR_XOR_R8(gb->cpu_reg.hl.bytes.l);
		break;

	case 0xAE: /* XOR (HL) */
		WGB_INSTR_XOR_R8(__gb_read(gb, gb->cpu_reg.hl.reg));
		break;

	case 0xAF: /* XOR A */
		WGB_INSTR_XOR_R8(gb->cpu_reg.a);
		break;

	case 0xB0: /* OR B */
		WGB_INSTR_OR_R8(gb->cpu_reg.bc.bytes.b);
		break;

	case 0xB1: /* OR C */
		WGB_INSTR_OR_R8(gb->cpu_reg.bc.bytes.c);
		break;

	case 0xB2: /* OR D */
		WGB_INSTR_OR_R8(gb->cpu_reg.de.bytes.d);
		break;

	case 0xB3: /* OR E */
		WGB_INSTR_OR_R8(gb->cpu_reg.de.bytes.e);
		break;

	case 0xB4: /* OR H */
		WGB_INSTR_OR_R8(gb->cpu_reg.hl.bytes.h);
		break;

	case 0xB5: /* OR L */
		WGB_INSTR_OR_R8(gb->cpu_reg.hl.bytes.l);
		break;

	case 0xB6: /* OR (HL) */
		WGB_INSTR_OR_R8(__gb_read(gb, gb->cpu_reg.hl.reg));
		break;

	case 0xB7: /* OR A */
		WGB_INSTR_OR_R8(gb->cpu_reg.a);
		break;

	case 0xB8: /* CP B */
		WGB_INSTR_CP_R8(gb->cpu_reg.bc.bytes.b);
		break;

	case 0xB9: /* CP C */
		WGB_INSTR_CP_R8(gb->cpu_reg.bc.bytes.c);
		break;

	case 0xBA: /* CP D */
		WGB_INSTR_CP_R8(gb->cpu_reg.de.bytes.d);
		break;

	case 0xBB: /* CP E */
		WGB_INSTR_CP_R8(gb->cpu_reg.de.bytes.e);
		break;

	case 0xBC: /* CP H */
		WGB_INSTR_CP_R8(gb->cpu_reg.hl.bytes.h);
		break;

	case 0xBD: /* CP L */
		WGB_INSTR_CP_R8(gb->cpu_reg.hl.bytes.l);
		break;

	case 0xBE: /* CP (HL) */
		WGB_INSTR_CP_R8(__gb_read(gb, gb->cpu_reg.hl.reg));
		break;

	case 0xBF: /* CP A */
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.z = 1;
		gb->cpu_reg.f.f_bits.n = 1;
		break;

	case 0xC0: /* RET NZ */
		if(!gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_OPS
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
        gb->cpu_reg.sp.reg += 2;
#else
        gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
        gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
        inst_cycles += 12;
		}

		break;

	case 0xC1: /* POP BC */
#if WALNUT_GB_16_BIT_OPS
    gb->cpu_reg.bc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2;
#else
    gb->cpu_reg.bc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.bc.bytes.b = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		break;

	case 0xC2: /* JP NZ, imm */
		if(!gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_OPS
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xC3: /* JP imm */
#if WALNUT_GB_16_BIT_OPS
    gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
#else
{
    uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
    gb->cpu_reg.pc.bytes.c = c;
    gb->cpu_reg.pc.bytes.p = p;
}
#endif
		break;

	case 0xC4: /* CALL NZ imm */
		if(!gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_OPS
			uint16_t addr = __gb_read16(gb, gb->cpu_reg.pc.reg);
			gb->cpu_reg.pc.reg += 2;
      gb->cpu_reg.sp.reg -= 2;
      __gb_write16(gb, gb->cpu_reg.sp.reg, gb->cpu_reg.pc.reg);
			gb->cpu_reg.pc.reg = addr;
#else
			uint8_t c, p;
			c = __gb_read(gb, gb->cpu_reg.pc.reg++);
			p = __gb_read(gb, gb->cpu_reg.pc.reg++);
			__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
			__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
			gb->cpu_reg.pc.bytes.c = c;
			gb->cpu_reg.pc.bytes.p = p;
#endif        
			inst_cycles += 12;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xC5: /* PUSH BC */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.bc.bytes.b);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.bc.bytes.c);
		break;

	case 0xC6: /* ADD A, imm */
	{
		uint8_t val = __gb_read(gb, gb->cpu_reg.pc.reg++);
		WGB_INSTR_ADC_R8(val, 0);
		break;
	}

	case 0xC7: /* RST 0x0000 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0000;
		break;

	case 0xC8: /* RET Z */
		if(gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_OPS
        // Optional 16-bit path if stack is aligned
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
        gb->cpu_reg.sp.reg += 2;
#else
        gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
        gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
			inst_cycles += 12;
		}
		break;

	case 0xC9: /* RET */
	{
#if WALNUT_GB_16_BIT_OPS
    gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2;
#else
    gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		break;
	}

	case 0xCA: /* JP Z, imm */
		if(gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_OPS
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.pc.reg); // no need to advance pc because of jump
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xCB: /* CB INST */
		inst_cycles = __gb_execute_cb(gb);
		break;

	case 0xCC: /* CALL Z, imm */
		if(gb->cpu_reg.f.f_bits.z)
		{
#if WALNUT_GB_16_BIT_OPS
        uint16_t addr = __gb_read16(gb, gb->cpu_reg.pc.reg);
        gb->cpu_reg.pc.reg += 2; // advance past immediate

        // push old PC
				gb->cpu_reg.sp.reg-=2;
        __gb_write16(gb, gb->cpu_reg.sp.reg, gb->cpu_reg.pc.reg);

        gb->cpu_reg.pc.reg = addr;
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 12;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xCD: /* CALL imm */
#if WALNUT_GB_16_BIT_OPS
{
    uint16_t addr = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2; // advance past immediate
		gb->cpu_reg.sp.reg-=2;
		__gb_write16(gb, gb->cpu_reg.sp.reg, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg = addr;
}
#else
{
    uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
    __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
    __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
    gb->cpu_reg.pc.bytes.c = c;
    gb->cpu_reg.pc.bytes.p = p;
}
#endif
	break;

	case 0xCE: /* ADC A, imm */
	{
		uint8_t val = __gb_read(gb, gb->cpu_reg.pc.reg++);
		WGB_INSTR_ADC_R8(val, gb->cpu_reg.f.f_bits.c);
		break;
	}

	case 0xCF: /* RST 0x0008 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0008;
		break;

	case 0xD0: /* RET NC */

    if (!gb->cpu_reg.f.f_bits.c)
    {
#if WALNUT_GB_16_BIT_OPS
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
        gb->cpu_reg.sp.reg += 2; // advance SP past the popped PC
#else
        gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
        gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
        inst_cycles += 12;
    }

		break;

	case 0xD1: /* POP DE */
#if WALNUT_GB_16_BIT_OPS
    gb->cpu_reg.de.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2;
#else
    gb->cpu_reg.de.bytes.e = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.de.bytes.d = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		break;

	case 0xD2: /* JP NC, imm */
		if(!gb->cpu_reg.f.f_bits.c)
		{
#if WALNUT_GB_16_BIT_OPS
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.pc.reg); // jump so no need to advance pc
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xD4: /* CALL NC, imm */
		if(!gb->cpu_reg.f.f_bits.c)
		{
#if WALNUT_GB_16_BIT_OPS
        uint16_t addr = __gb_read16(gb, gb->cpu_reg.pc.reg);
        gb->cpu_reg.pc.reg += 2; // advance PC past immediate
				gb->cpu_reg.sp.reg-=2;
        __gb_write16(gb, gb->cpu_reg.sp.reg, gb->cpu_reg.pc.reg);
        gb->cpu_reg.pc.reg = addr;
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 12;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xD5: /* PUSH DE */
	// this was revised but working backwards from this 16-bit op to find the one thats broken
#if WALNUT_GB_16_BIT_OPS
		gb->cpu_reg.sp.reg-=2;
		__gb_write16(gb,gb->cpu_reg.sp.reg,gb->cpu_reg.de.reg);
#else
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.de.bytes.d);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.de.bytes.e);
#endif
		break;

	case 0xD6: /* SUB imm */
	{
		uint8_t val = __gb_read(gb, gb->cpu_reg.pc.reg++);
		uint16_t temp = gb->cpu_reg.a - val;
		gb->cpu_reg.f.f_bits.z = ((temp & 0xFF) == 0x00);
		gb->cpu_reg.f.f_bits.n = 1;
		gb->cpu_reg.f.f_bits.h =
			(gb->cpu_reg.a ^ val ^ temp) & 0x10 ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = (temp & 0xFF00) ? 1 : 0;
		gb->cpu_reg.a = (temp & 0xFF);
		break;
	}

	case 0xD7: /* RST 0x0010 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0010;
		break;

	case 0xD8: /* RET C */
		if(gb->cpu_reg.f.f_bits.c)
		{
#if WALNUT_GB_16_BIT
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
        gb->cpu_reg.sp.reg += 2; // advance SP past the popped PC
#else
        gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
        gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
			inst_cycles += 12;
		}

		break;

	case 0xD9: /* RETI */
	{
#if WALNUT_GB_16_BIT
    gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2; // advance SP past the popped PC
#else
    gb->cpu_reg.pc.bytes.c = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.pc.bytes.p = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		gb->gb_ime = true;
	}
	break;

	case 0xDA: /* JP C, imm */
		if(gb->cpu_reg.f.f_bits.c)
		{
#if WALNUT_GB_16_BIT
        gb->cpu_reg.pc.reg = __gb_read16(gb, gb->cpu_reg.pc.reg);
        gb->cpu_reg.pc.reg += 2; // advance PC past the immediate word
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 4;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xDC: /* CALL C, imm */
		if(gb->cpu_reg.f.f_bits.c)
		{
#if WALNUT_GB_16_BIT
        uint16_t target = __gb_read16(gb, gb->cpu_reg.pc.reg);
        gb->cpu_reg.pc.reg += 2; // advance PC past the immediate word
        // push current PC onto stack
        gb->cpu_reg.sp.reg -= 2;
				__gb_write16(gb, gb->cpu_reg.sp.reg, gb->cpu_reg.pc.reg)
        gb->cpu_reg.pc.reg = target;
#else
        uint8_t c = __gb_read(gb, gb->cpu_reg.pc.reg++);
        uint8_t p = __gb_read(gb, gb->cpu_reg.pc.reg++);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
        __gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
        gb->cpu_reg.pc.bytes.c = c;
        gb->cpu_reg.pc.bytes.p = p;
#endif
			inst_cycles += 12;
		}
		else
			gb->cpu_reg.pc.reg += 2;

		break;

	case 0xDE: /* SBC A, imm */
	{
		uint8_t val = __gb_read(gb, gb->cpu_reg.pc.reg++);
		WGB_INSTR_SBC_R8(val, gb->cpu_reg.f.f_bits.c);
		break;
	}

	case 0xDF: /* RST 0x0018 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0018;
		break;

	case 0xE0: /* LD (0xFF00+imm), A */
		__gb_write(gb, 0xFF00 | __gb_read(gb, gb->cpu_reg.pc.reg++),
			   gb->cpu_reg.a);
		break;

	case 0xE1: /* POP HL */
#if WALNUT_GB_16_BIT
    gb->cpu_reg.hl.reg = __gb_read16(gb, gb->cpu_reg.sp.reg);
    gb->cpu_reg.sp.reg += 2; // advance SP past the popped value
#else
    gb->cpu_reg.hl.bytes.l = __gb_read(gb, gb->cpu_reg.sp.reg++);
    gb->cpu_reg.hl.bytes.h = __gb_read(gb, gb->cpu_reg.sp.reg++);
#endif
		break;

	case 0xE2: /* LD (C), A */
		__gb_write(gb, 0xFF00 | gb->cpu_reg.bc.bytes.c, gb->cpu_reg.a);
		break;

	case 0xE5: /* PUSH HL */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.hl.bytes.h);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.hl.bytes.l);
		break;

	case 0xE6: /* AND imm */
	{
		uint8_t temp = __gb_read(gb, gb->cpu_reg.pc.reg++);
		WGB_INSTR_AND_R8(temp);
		break;
	}

	case 0xE7: /* RST 0x0020 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0020;
		break;

	case 0xE8: /* ADD SP, imm */
	{
		int8_t offset = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.h = ((gb->cpu_reg.sp.reg & 0xF) + (offset & 0xF) > 0xF) ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = ((gb->cpu_reg.sp.reg & 0xFF) + (offset & 0xFF) > 0xFF);
		gb->cpu_reg.sp.reg += offset;
		break;
	}

	case 0xE9: /* JP (HL) */
		gb->cpu_reg.pc.reg = gb->cpu_reg.hl.reg;
		break;

	case 0xEA: /* LD (imm), A */
	{
#if WALNUT_GB_16_BIT
    uint16_t addr = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2; // advance past the immediate
#else
    uint8_t l = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint8_t h = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint16_t addr = WALNUT_GB_U8_TO_U16(h, l);
#endif
		__gb_write(gb, addr, gb->cpu_reg.a);
		break;
	}

	case 0xEE: /* XOR imm */
		WGB_INSTR_XOR_R8(__gb_read(gb, gb->cpu_reg.pc.reg++));
		break;

	case 0xEF: /* RST 0x0028 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0028;
		break;

	case 0xF0: /* LD A, (0xFF00+imm) */
		gb->cpu_reg.a =
			__gb_read(gb, 0xFF00 | __gb_read(gb, gb->cpu_reg.pc.reg++));
		break;

	case 0xF1: /* POP AF */
	{
		uint8_t temp_8 = __gb_read(gb, gb->cpu_reg.sp.reg++);
		gb->cpu_reg.f.f_bits.z = (temp_8 >> 7) & 1;
		gb->cpu_reg.f.f_bits.n = (temp_8 >> 6) & 1;
		gb->cpu_reg.f.f_bits.h = (temp_8 >> 5) & 1;
		gb->cpu_reg.f.f_bits.c = (temp_8 >> 4) & 1;
		gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg.sp.reg++);
		break;
	}

	case 0xF2: /* LD A, (C) */
		gb->cpu_reg.a = __gb_read(gb, 0xFF00 | gb->cpu_reg.bc.bytes.c);
		break;

	case 0xF3: /* DI */
		gb->gb_ime = false;
		break;

	case 0xF5: /* PUSH AF */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.a);
		__gb_write(gb, --gb->cpu_reg.sp.reg,
			   gb->cpu_reg.f.f_bits.z << 7 | gb->cpu_reg.f.f_bits.n << 6 |
			   gb->cpu_reg.f.f_bits.h << 5 | gb->cpu_reg.f.f_bits.c << 4);
		break;

	case 0xF6: /* OR imm */
		WGB_INSTR_OR_R8(__gb_read(gb, gb->cpu_reg.pc.reg++));
		break;

	case 0xF7: /* PUSH AF */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0030;
		break;

	case 0xF8: /* LD HL, SP+/-imm */
	{
		/* Taken from SameBoy, which is released under MIT Licence. */
		int8_t offset = (int8_t) __gb_read(gb, gb->cpu_reg.pc.reg++);
		gb->cpu_reg.hl.reg = gb->cpu_reg.sp.reg + offset;
		gb->cpu_reg.f.reg = 0;
		gb->cpu_reg.f.f_bits.h = ((gb->cpu_reg.sp.reg & 0xF) + (offset & 0xF) > 0xF) ? 1 : 0;
		gb->cpu_reg.f.f_bits.c = ((gb->cpu_reg.sp.reg & 0xFF) + (offset & 0xFF) > 0xFF) ? 1 : 0;
		break;
	}

	case 0xF9: /* LD SP, HL */
		gb->cpu_reg.sp.reg = gb->cpu_reg.hl.reg;
		break;

	case 0xFA: /* LD A, (imm) */
	{
#if WALNUT_GB_16_BIT
    uint16_t addr = __gb_read16(gb, gb->cpu_reg.pc.reg);
    gb->cpu_reg.pc.reg += 2; // advance past the immediate
#else
    uint8_t l = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint8_t h = __gb_read(gb, gb->cpu_reg.pc.reg++);
    uint16_t addr = WALNUT_GB_U8_TO_U16(h, l);
#endif
		gb->cpu_reg.a = __gb_read(gb, addr);
		break;
	}

	case 0xFB: /* EI */
		gb->gb_ime = true;
		break;

	case 0xFE: /* CP imm */
	{
		uint8_t val = __gb_read(gb, gb->cpu_reg.pc.reg++);
		WGB_INSTR_CP_R8(val);
		break;
	}

	case 0xFF: /* RST 0x0038 */
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.p);
		__gb_write(gb, --gb->cpu_reg.sp.reg, gb->cpu_reg.pc.bytes.c);
		gb->cpu_reg.pc.reg = 0x0038;
		break;

	default:
		/* Return address where invalid opcode that was read. */
		(gb->gb_error)(gb, GB_INVALID_OPCODE, gb->cpu_reg.pc.reg - 1);
		WGB_UNREACHABLE();
	}

	// *** post opcode handling ***

	do
	{
		/* DIV register timing */
		gb->counter.div_count += inst_cycles;
		while(gb->counter.div_count >= DIV_CYCLES)
		{
			gb->hram_io[IO_DIV]++;
			gb->counter.div_count -= DIV_CYCLES;
		}

		/* Check for RTC tick. */
		if(gb->mbc == 3 && (gb->rtc_real.reg.high & 0x40) == 0)
		{
			gb->counter.rtc_count += inst_cycles;
			while(WGB_UNLIKELY(gb->counter.rtc_count >= RTC_CYCLES))
			{
				gb->counter.rtc_count -= RTC_CYCLES;

				/* Detect invalid rollover. */
				if(WGB_UNLIKELY(gb->rtc_real.reg.sec == 63))
				{
					gb->rtc_real.reg.sec = 0;
					continue;
				}

				if(++gb->rtc_real.reg.sec != 60)
					continue;

				gb->rtc_real.reg.sec = 0;
				if(gb->rtc_real.reg.min == 63)
				{
					gb->rtc_real.reg.min = 0;
					continue;
				}
				if(++gb->rtc_real.reg.min != 60)
					continue;

				gb->rtc_real.reg.min = 0;
				if(gb->rtc_real.reg.hour == 31)
				{
					gb->rtc_real.reg.hour = 0;
					continue;
				}
				if(++gb->rtc_real.reg.hour != 24)
					continue;

				gb->rtc_real.reg.hour = 0;
				if(++gb->rtc_real.reg.yday != 0)
					continue;

				if(gb->rtc_real.reg.high & 1)  /* Bit 8 of days*/
					gb->rtc_real.reg.high |= 0x80; /* Overflow bit */

				gb->rtc_real.reg.high ^= 1;
			}
		}

		/* Check serial transmission. */
		if(gb->hram_io[IO_SC] & SERIAL_SC_TX_START)
		{
			unsigned int serial_cycles = SERIAL_CYCLES_1KB;

			/* If new transfer, call TX function. */
			if(gb->counter.serial_count == 0 &&
				gb->gb_serial_tx != NULL)
				(gb->gb_serial_tx)(gb, gb->hram_io[IO_SB]);

#if WALNUT_FULL_GBC_SUPPORT
			if(gb->hram_io[IO_SC] & 0x3)
				serial_cycles = SERIAL_CYCLES_32KB;
#endif

			gb->counter.serial_count += inst_cycles;

			/* If it's time to receive byte, call RX function. */
			if(gb->counter.serial_count >= serial_cycles)
			{
				/* If RX can be done, do it. */
				/* If RX failed, do not change SB if using external
				 * clock, or set to 0xFF if using internal clock. */
				uint8_t rx;

				if(gb->gb_serial_rx != NULL &&
					(gb->gb_serial_rx(gb, &rx) ==
						GB_SERIAL_RX_SUCCESS))
				{
					gb->hram_io[IO_SB] = rx;

					/* Inform game of serial TX/RX completion. */
					gb->hram_io[IO_SC] &= 0x01;
					gb->hram_io[IO_IF] |= SERIAL_INTR;
				}
				else if(gb->hram_io[IO_SC] & SERIAL_SC_CLOCK_SRC)
				{
					/* If using internal clock, and console is not
					 * attached to any external peripheral, shifted
					 * bits are replaced with logic 1. */
					gb->hram_io[IO_SB] = 0xFF;

					/* Inform game of serial TX/RX completion. */
					gb->hram_io[IO_SC] &= 0x01;
					gb->hram_io[IO_IF] |= SERIAL_INTR;
				}
				else
				{
					/* If using external clock, and console is not
					 * attached to any external peripheral, bits are
					 * not shifted, so SB is not modified. */
				}

				gb->counter.serial_count = 0;
			}
		}

		/* TIMA register timing */
		/* TODO: Change tac_enable to struct of TAC timer control bits. */
		if(gb->hram_io[IO_TAC] & IO_TAC_ENABLE_MASK)
		{
			gb->counter.tima_count += inst_cycles;

			while(gb->counter.tima_count >=
				TAC_CYCLES[gb->hram_io[IO_TAC] & IO_TAC_RATE_MASK])
			{
				gb->counter.tima_count -=
					TAC_CYCLES[gb->hram_io[IO_TAC] & IO_TAC_RATE_MASK];

				if(++gb->hram_io[IO_TIMA] == 0)
				{
					gb->hram_io[IO_IF] |= TIMER_INTR;
					/* On overflow, set TMA to TIMA. */
					gb->hram_io[IO_TIMA] = gb->hram_io[IO_TMA];
				}
			}
		}

		/* If LCD is off, don't update LCD state or increase the LCD
		 * ticks. Instead, keep track of the amount of time that is
		 * being passed. */
		if(!(gb->hram_io[IO_LCDC] & LCDC_ENABLE))
		{
			gb->counter.lcd_off_count += inst_cycles;
			if(gb->counter.lcd_off_count >= LCD_FRAME_CYCLES)
			{
				gb->counter.lcd_off_count -= LCD_FRAME_CYCLES;
				gb->gb_frame = true;
			}
			continue;
		}

		/* LCD Timing */
#if WALNUT_FULL_GBC_SUPPORT
        if (inst_cycles > 1) {
            gb->counter.lcd_count += (inst_cycles >> gb->cgb.doubleSpeed);
        } else {
#endif
		gb->counter.lcd_count += inst_cycles;
#if WALNUT_FULL_GBC_SUPPORT
	}
#endif

		/* New Scanline. HBlank -> VBlank or OAM Scan */
		if(gb->counter.lcd_count >= LCD_LINE_CYCLES)
		{
			gb->counter.lcd_count -= LCD_LINE_CYCLES;

			/* Next line */
			gb->hram_io[IO_LY] = gb->hram_io[IO_LY] + 1;
			if (gb->hram_io[IO_LY] == LCD_VERT_LINES)
				gb->hram_io[IO_LY] = 0;

			/* LYC Update */
			if(gb->hram_io[IO_LY] == gb->hram_io[IO_LYC])
			{
				gb->hram_io[IO_STAT] |= STAT_LYC_COINC;

				if(gb->hram_io[IO_STAT] & STAT_LYC_INTR)
					gb->hram_io[IO_IF] |= LCDC_INTR;
			}
			else
				gb->hram_io[IO_STAT] &= 0xFB;

			/* Check if LCD should be in Mode 1 (VBLANK) state */
			if(gb->hram_io[IO_LY] == LCD_HEIGHT)
			{
				gb->hram_io[IO_STAT] =
					(gb->hram_io[IO_STAT] & ~STAT_MODE) | IO_STAT_MODE_VBLANK;
				gb->gb_frame = true;
				gb->hram_io[IO_IF] |= VBLANK_INTR;
				gb->lcd_blank = false;

				if(gb->hram_io[IO_STAT] & STAT_MODE_1_INTR)
					gb->hram_io[IO_IF] |= LCDC_INTR;

#if ENABLE_LCD
				/* If frame skip is activated, check if we need to draw
				 * the frame or skip it. */
				if(gb->direct.frame_skip)
				{
					gb->display.frame_skip_count =
						!gb->display.frame_skip_count;
				}

				/* If interlaced is activated, change which lines get
				 * updated. Also, only update lines on frames that are
				 * actually drawn when frame skip is enabled. */
				if(gb->direct.interlace &&
						(!gb->direct.frame_skip ||
						 gb->display.frame_skip_count))
				{
					gb->display.interlace_count =
						!gb->display.interlace_count;
				}
#endif
                                /* If halted forever, then return on VBLANK. */
                                if(gb->gb_halt && !gb->hram_io[IO_IE])
					break;
			}
			/* Start of normal Line (not in VBLANK) */
			else if(gb->hram_io[IO_LY] < LCD_HEIGHT)
			{
				if(gb->hram_io[IO_LY] == 0)
				{
					/* Clear Screen */
					gb->display.WY = gb->hram_io[IO_WY];
					gb->display.window_clear = 0;
				}

				/* OAM Search occurs at the start of the line. */
				gb->hram_io[IO_STAT] = (gb->hram_io[IO_STAT] & ~STAT_MODE) | IO_STAT_MODE_OAM_SCAN;
				gb->counter.lcd_count = 0;


#if WALNUT_FULL_GBC_SUPPORT
				//DMA GBC
				if(gb->cgb.cgbMode && !gb->cgb.dmaActive && gb->cgb.dmaMode)
				{
#if WALNUT_GB_32BIT_DMA
					// Optimized 16-bit path
					for (uint8_t i = 0; i < 0x10; i += 4)
					{
							uint32_t val = __gb_read32(gb, (gb->cgb.dmaSource & 0xFFF0) + i);
							__gb_write32(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i, val);
							// 8-bit logic if there is some cause to fall back
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i, val);
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i + 1, val >> 8);
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i + 2, val >> 16);
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i + 3, val >> 24);
					}
#elif WALNUT_GB_16BIT_DMA
					// Optimized 16-bit path
					for (uint8_t i = 0; i < 0x10; i += 2)
					{
							uint16_t val = __gb_read16(gb, (gb->cgb.dmaSource & 0xFFF0) + i);
							__gb_write16(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i, val);
							// 8-bit logic if there is some cause to fall back
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i, val & 0xFF);
							// __gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i + 1, val >> 8);
					}
#else
			    // Original 8-bit path
					for (uint8_t i = 0; i < 0x10; i++)
					{
						__gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i,
										__gb_read(gb, (gb->cgb.dmaSource & 0xFFF0) + i));
					}
#endif

					gb->cgb.dmaSource += 0x10;
					gb->cgb.dmaDest += 0x10;
					if(!(--gb->cgb.dmaSize)) {gb->cgb.dmaActive = 1;
					}
				}
#endif
				if(gb->hram_io[IO_STAT] & STAT_MODE_2_INTR)
					gb->hram_io[IO_IF] |= LCDC_INTR;

				/* If halted immediately jump to next LCD mode.
				 * From OAM Search to LCD Draw. */
				//if(gb->counter.lcd_count < LCD_MODE2_OAM_SCAN_END)
				//	inst_cycles = LCD_MODE2_OAM_SCAN_END - gb->counter.lcd_count;
				inst_cycles = LCD_MODE2_OAM_SCAN_DURATION;
			}
		}
		/* Go from Mode 3 (LCD Draw) to Mode 0 (HBLANK). */
		else if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_LCD_DRAW &&
				gb->counter.lcd_count >= LCD_MODE3_LCD_DRAW_END)
		{
			gb->hram_io[IO_STAT] = (gb->hram_io[IO_STAT] & ~STAT_MODE) | IO_STAT_MODE_HBLANK;

			if(gb->hram_io[IO_STAT] & STAT_MODE_0_INTR)
				gb->hram_io[IO_IF] |= LCDC_INTR;

			/* If halted immediately, jump from OAM Scan to LCD Draw. */
			if (gb->counter.lcd_count < LCD_MODE0_HBLANK_MAX_DRUATION)
				inst_cycles = LCD_MODE0_HBLANK_MAX_DRUATION - gb->counter.lcd_count;
		}
		/* Go from Mode 2 (OAM Scan) to Mode 3 (LCD Draw). */
		else if((gb->hram_io[IO_STAT] & STAT_MODE) == IO_STAT_MODE_OAM_SCAN &&
				gb->counter.lcd_count >= LCD_MODE2_OAM_SCAN_END)
		{
			gb->hram_io[IO_STAT] = (gb->hram_io[IO_STAT] & ~STAT_MODE) | IO_STAT_MODE_LCD_DRAW;
#if ENABLE_LCD
			if(!gb->lcd_blank)
				__gb_draw_line(gb);
#endif
			/* If halted immediately jump to next LCD mode. */
			if (gb->counter.lcd_count < LCD_MODE3_LCD_DRAW_MIN_DURATION)
				inst_cycles = LCD_MODE3_LCD_DRAW_MIN_DURATION - gb->counter.lcd_count;
		}
	} while(gb->gb_halt && (gb->hram_io[IO_IF] & gb->hram_io[IO_IE]) == 0);
}

