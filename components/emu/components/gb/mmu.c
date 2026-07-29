#include "gb.h"
#if WALNUT_GB_16BIT_ALIGNED
uint16_t __gb_read16(struct gb_s *gb, uint16_t addr)
{
    switch (WALNUT_GB_GET_MSN16(addr))
    {
        // --- Boot ROM / Fixed ROM 0
        case 0x0:
            if (gb->hram_io[IO_BOOT] == 0 && addr < 0x0100)
                return (uint16_t)gb->gb_bootrom_read(gb, addr)
                     | ((uint16_t)gb->gb_bootrom_read(gb, addr + 1) << 8);
#if WALNUT_FULL_GBC_SUPPORT
            else if (gb->cgb.cgbMode && gb->hram_io[IO_BOOT] == 0 && addr >= 0x0200 && addr < 0x0900)
                return (uint16_t)gb->gb_bootrom_read(gb, addr)
                     | ((uint16_t)gb->gb_bootrom_read(gb, addr + 1) << 8);
#endif
            /* fallthrough */
        case 0x1:
        case 0x2:
        case 0x3:
            return gb->gb_rom_read_16bit(gb, addr);

        // --- Switchable ROM banks (0x4000–0x7FFF)
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
        {
            uint32_t bank_offset = (gb->selected_rom_bank - 1) * ROM_BANK_SIZE;
            return gb->gb_rom_read_16bit(gb, addr + bank_offset);
        }

        // --- VRAM (0x8000–0x9FFF)
        case 0x8:
        case 0x9:
        {
            uint8_t *vram_base =
#if WALNUT_FULL_GBC_SUPPORT
                &gb->vram[addr - gb->cgb.vramBankOffset];
#else
                &gb->vram[addr - VRAM_ADDR];
#endif
            if (addr + 1 < 0xA000)
            {
                if (((uintptr_t)vram_base & 1) == 0)
                    return *(uint16_t *)vram_base;
                else
                    return (uint16_t)vram_base[0] | ((uint16_t)vram_base[1] << 8);
            }
            return vram_base[0]; // last byte fallback
        }

        // --- External RAM / RTC (0xA000–0xBFFF)
        case 0xA:
        case 0xB:
            if (gb->mbc == 3 && gb->cart_ram_bank >= 0x08)
                return gb->rtc_latched.bytes[gb->cart_ram_bank - 0x08]
                     | ((uint16_t)gb->rtc_latched.bytes[gb->cart_ram_bank - 0x08 + 1] << 8);
            else if (gb->cart_ram && gb->enable_cart_ram)
            {
                uint16_t offset;
                if (gb->mbc == 2)
                    offset = addr & 0x1FF;
                else if ((gb->cart_mode_select || gb->mbc != 1) && gb->cart_ram_bank < gb->num_ram_banks)
                    offset = addr - CART_RAM_ADDR + (gb->cart_ram_bank * CRAM_BANK_SIZE);
                else
                    offset = addr - CART_RAM_ADDR;

                return gb->gb_cart_ram_read(gb, offset)
                     | ((uint16_t)gb->gb_cart_ram_read(gb, offset + 1) << 8);
            }
            return 0xFFFF;

        // --- Work RAM (0xC000–0xDFFF)
        case 0xC:
        case 0xD:
        {
            uint8_t *wram_ptr =
#if WALNUT_FULL_GBC_SUPPORT
                (gb->cgb.cgbMode && addr >= WRAM_1_ADDR)
                    ? &gb->wram[addr - gb->cgb.wramBankOffset]
                    : &gb->wram[addr - WRAM_0_ADDR];
#else
                &gb->wram[addr - WRAM_0_ADDR];
#endif
            if (((uintptr_t)wram_ptr & 1) == 0)
                return *(uint16_t *)wram_ptr;
            else
                return (uint16_t)wram_ptr[0] | ((uint16_t)wram_ptr[1] << 8);
        }

        // --- Echo RAM (0xE000–0xFDFF)
        case 0xE:
        {
            uint8_t *echo_ptr = &gb->wram[addr - ECHO_ADDR];
            if (((uintptr_t)echo_ptr & 1) == 0)
                return *(uint16_t *)echo_ptr;
            else
                return (uint16_t)echo_ptr[0] | ((uint16_t)echo_ptr[1] << 8);
        }

        // --- OAM / HRAM / IO (0xFE00–0xFFFF)
        case 0xF:
            if (addr < 0xFEA0)
            {
                uint8_t *oam_ptr = &gb->oam[addr - OAM_ADDR];
                if (((uintptr_t)oam_ptr & 1) == 0)
                    return *(uint16_t *)oam_ptr;
                else
                    return (uint16_t)oam_ptr[0] | ((uint16_t)oam_ptr[1] << 8);
            }
            else if (addr >= IO_ADDR)
            {
                // Some special registers require manual byte combine
#if ENABLE_SOUND
                if (addr >= 0xFF10 && addr <= 0xFF3F)
                {
                    uint8_t lo = audio_read(addr);
                    uint8_t hi = audio_read(addr + 1);
                    return lo | ((uint16_t)hi << 8);
                }
#endif
                return (uint16_t)gb->hram_io[addr - IO_ADDR]
                     | ((uint16_t)gb->hram_io[addr + 1 - IO_ADDR] << 8);
            }
            else
                return 0xFFFF;
    }

    (gb->gb_error)(gb, GB_INVALID_READ, addr);
    WGB_UNREACHABLE();
}
#else
uint16_t __gb_read16(struct gb_s *gb, uint16_t addr)
{
    switch(WALNUT_GB_GET_MSN16(addr))
    {
        // --- Boot ROM / Fixed ROM 0
        case 0x0:
            if(gb->hram_io[IO_BOOT] == 0 && addr < 0x0100)
                return (uint16_t)gb->gb_bootrom_read(gb, addr)
                     | ((uint16_t)gb->gb_bootrom_read(gb, addr + 1) << 8);
#if WALNUT_FULL_GBC_SUPPORT
            else if(gb->cgb.cgbMode && gb->hram_io[IO_BOOT] == 0 && addr >= 0x0200 && addr < 0x0900)
                return (uint16_t)gb->gb_bootrom_read(gb, addr)
                     | ((uint16_t)gb->gb_bootrom_read(gb, addr + 1) << 8);
#endif
            /* fallthrough */
        case 0x1:
        case 0x2:
        case 0x3:
            return gb->gb_rom_read_16bit(gb,addr);

        // --- Switchable ROM banks (0x4000–0x7FFF)
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
        {
            uint32_t bank_offset = (gb->selected_rom_bank - 1) * ROM_BANK_SIZE;
            return gb->gb_rom_read_16bit(gb,addr + bank_offset);
        }

        // --- VRAM (0x8000–0x9FFF)
        case 0x8:
        case 0x9:
#if WALNUT_FULL_GBC_SUPPORT
            if (addr + 1 < 0xA000)
                return *(uint16_t *)&gb->vram[addr - gb->cgb.vramBankOffset];
            else
                return gb->vram[addr - gb->cgb.vramBankOffset]; // last byte fallback
#else
            if (addr + 1 < 0xA000)
                return *(uint16_t *)&gb->vram[addr - VRAM_ADDR];
            else
                return gb->vram[addr - VRAM_ADDR];
#endif

        // --- External RAM / RTC (0xA000–0xBFFF)
        case 0xA:
        case 0xB:
            if (gb->mbc == 3 && gb->cart_ram_bank >= 0x08)
            {
                // RTC bytes are separate; combine manually
                return gb->rtc_latched.bytes[gb->cart_ram_bank - 0x08]
                     | ((uint16_t)gb->rtc_latched.bytes[gb->cart_ram_bank - 0x08 + 1] << 8);
            }
            else if (gb->cart_ram && gb->enable_cart_ram)
            {
                uint16_t offset;
                if (gb->mbc == 2)
                {
                    addr &= 0x1FF;
                    offset = addr;
                }
                else if ((gb->cart_mode_select || gb->mbc != 1) && gb->cart_ram_bank < gb->num_ram_banks)
                {
                    offset = addr - CART_RAM_ADDR + (gb->cart_ram_bank * CRAM_BANK_SIZE);
                }
                else
                {
                    offset = addr - CART_RAM_ADDR;
                }
                return gb->gb_cart_ram_read(gb, offset) | ((uint16_t)gb->gb_cart_ram_read(gb, offset + 1) << 8);
            }
            return 0xFFFF;

        // --- Work RAM (0xC000–0xDFFF)
        case 0xC:
        case 0xD:
#if WALNUT_FULL_GBC_SUPPORT
            if(gb->cgb.cgbMode && addr >= WRAM_1_ADDR)
                return *(uint16_t *)&gb->wram[addr - gb->cgb.wramBankOffset];
#endif
            return *(uint16_t *)&gb->wram[addr - WRAM_0_ADDR];

        // --- Echo RAM (0xE000–0xFDFF)
        case 0xE:
            return *(uint16_t *)&gb->wram[addr - ECHO_ADDR];

        // --- OAM (0xFE00–0xFE9F)
        case 0xF:
            if (addr < 0xFEA0)
                return *(uint16_t *)&gb->oam[addr - OAM_ADDR];
            // --- HRAM / IO (0xFF00–0xFFFF)
            else if (addr >= IO_ADDR)
            {
                // Some registers are not contiguous, must combine manually
#if ENABLE_SOUND
                if (addr >= 0xFF10 && addr <= 0xFF3F)
                {
                    uint8_t lo = audio_read(addr);
                    uint8_t hi = audio_read(addr + 1);
                    return lo | ((uint16_t)hi << 8);
                }
#endif
#if WALNUT_FULL_GBC_SUPPORT
                switch (addr & 0xFF)
                {
                    case 0x4D: // Speed switch
                        return gb->cgb.doubleSpeed | ((uint16_t)gb->cgb.doubleSpeedPrep << 8);
                    case 0x4F: // VRAM bank
                        return gb->cgb.vramBank | 0xFE00;
                    case 0x51: case 0x52: case 0x53: case 0x54: case 0x55:
                        {
                            uint8_t lo = gb->hram_io[addr - IO_ADDR];
                            uint8_t hi = gb->hram_io[addr + 1 - IO_ADDR];
                            return lo | ((uint16_t)hi << 8);
                        }
                    case 0x56:
                        return gb->hram_io[0x56] | ((uint16_t)gb->hram_io[0x57] << 8);
                    case 0x68: case 0x69: case 0x6A: case 0x6B:
                        {
                            uint8_t lo = gb->hram_io[addr - IO_ADDR];
                            uint8_t hi = gb->hram_io[addr + 1 - IO_ADDR];
                            return lo | ((uint16_t)hi << 8);
                        }
                    case 0x70:
                        return gb->cgb.wramBank | 0x00; // upper byte not used
                    default:
                        return *(uint16_t *)&gb->hram_io[addr - IO_ADDR];
                }
#else
                return *(uint16_t *)&gb->hram_io[addr - IO_ADDR];
#endif
            }
            else
            {
                // Unusable memory or unknown region; return 0xFFFF
                return 0xFFFF;
            }
    }

    // Should never reach here
    (gb->gb_error)(gb, GB_INVALID_READ, addr);
    WGB_UNREACHABLE();
}
#endif

#if WALNUT_GB_32BIT_ALIGNED
uint32_t __gb_read32(struct gb_s *gb, uint16_t addr)
{
    switch (WALNUT_GB_GET_MSN16(addr))
    {
        // --- Boot ROM / Fixed ROM 0
        case 0x0:
            if (gb->hram_io[IO_BOOT] == 0 && addr < 0x0100)
                return (uint32_t)gb->gb_bootrom_read(gb, addr)
                     | ((uint32_t)gb->gb_bootrom_read(gb, addr + 1) << 8)
                     | ((uint32_t)gb->gb_bootrom_read(gb, addr + 2) << 16)
                     | ((uint32_t)gb->gb_bootrom_read(gb, addr + 3) << 24);
#if WALNUT_FULL_GBC_SUPPORT
            else if (gb->cgb.cgbMode && gb->hram_io[IO_BOOT] == 0 &&
                     addr >= 0x0200 && addr < 0x0900)
                return (uint32_t)gb->gb_bootrom_read(gb, addr)
                     | ((uint32_t)gb->gb_bootrom_read(gb, addr + 1) << 8)
                     | ((uint32_t)gb->gb_bootrom_read(gb, addr + 2) << 16)
                     | ((uint32_t)gb->gb_bootrom_read(gb, addr + 3) << 24);
#endif
            /* fallthrough */
        case 0x1:
        case 0x2:
        case 0x3:
            return gb->gb_rom_read_32bit(gb, addr);

        // --- Switchable ROM banks
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
        {
            uint32_t bank_offset = (gb->selected_rom_bank - 1) * ROM_BANK_SIZE;
            return gb->gb_rom_read_32bit(gb, addr + bank_offset);
        }

        // --- VRAM
        case 0x8:
        case 0x9:
        {
#if WALNUT_FULL_GBC_SUPPORT
            uint8_t *p = &gb->vram[addr - gb->cgb.vramBankOffset];
#else
            uint8_t *p = &gb->vram[addr - VRAM_ADDR];
#endif
            if (addr + 3 < 0xA000 && (((uintptr_t)p & 3) == 0))
                return *(uint32_t *)p;

            return (uint32_t)p[0]
                 | ((uint32_t)p[1] << 8)
                 | ((uint32_t)p[2] << 16)
                 | ((uint32_t)p[3] << 24);
        }

        // --- External RAM / RTC
        case 0xA:
        case 0xB:
            if (gb->mbc == 3 && gb->cart_ram_bank >= 0x08)
            {
                const uint8_t *p =
                    &gb->rtc_latched.bytes[gb->cart_ram_bank - 0x08];
                return (uint32_t)p[0]
                     | ((uint32_t)p[1] << 8)
                     | ((uint32_t)p[2] << 16)
                     | ((uint32_t)p[3] << 24);
            }
            else if (gb->cart_ram && gb->enable_cart_ram)
            {
                uint16_t offset;
                if (gb->mbc == 2)
                {
                    addr &= 0x1FF;
                    offset = addr;
                }
                else if ((gb->cart_mode_select || gb->mbc != 1) &&
                         gb->cart_ram_bank < gb->num_ram_banks)
                {
                    offset = addr - CART_RAM_ADDR +
                             (gb->cart_ram_bank * CRAM_BANK_SIZE);
                }
                else
                {
                    offset = addr - CART_RAM_ADDR;
                }

                return (uint32_t)gb->gb_cart_ram_read(gb, offset)
                     | ((uint32_t)gb->gb_cart_ram_read(gb, offset + 1) << 8)
                     | ((uint32_t)gb->gb_cart_ram_read(gb, offset + 2) << 16)
                     | ((uint32_t)gb->gb_cart_ram_read(gb, offset + 3) << 24);
            }
            return 0xFFFFFFFF;

        // --- WRAM
        case 0xC:
        case 0xD:
        {
#if WALNUT_FULL_GBC_SUPPORT
            if (gb->cgb.cgbMode && addr >= WRAM_1_ADDR)
            {
                uint8_t *p = &gb->wram[addr - gb->cgb.wramBankOffset];
                if (((uintptr_t)p & 3) == 0)
                    return *(uint32_t *)p;
                return (uint32_t)p[0]
                     | ((uint32_t)p[1] << 8)
                     | ((uint32_t)p[2] << 16)
                     | ((uint32_t)p[3] << 24);
            }
#endif
            uint8_t *p = &gb->wram[addr - WRAM_0_ADDR];
            if (((uintptr_t)p & 3) == 0)
                return *(uint32_t *)p;
            return (uint32_t)p[0]
                 | ((uint32_t)p[1] << 8)
                 | ((uint32_t)p[2] << 16)
                 | ((uint32_t)p[3] << 24);
        }

        // --- Echo RAM
        case 0xE:
        {
            uint8_t *p = &gb->wram[addr - ECHO_ADDR];
            if (((uintptr_t)p & 3) == 0)
                return *(uint32_t *)p;
            return (uint32_t)p[0]
                 | ((uint32_t)p[1] << 8)
                 | ((uint32_t)p[2] << 16)
                 | ((uint32_t)p[3] << 24);
        }

        // --- OAM / HRAM / IO
        case 0xF:
            if (addr < 0xFEA0)
            {
                uint8_t *p = &gb->oam[addr - OAM_ADDR];
                if (((uintptr_t)p & 3) == 0)
                    return *(uint32_t *)p;
                return (uint32_t)p[0]
                     | ((uint32_t)p[1] << 8)
                     | ((uint32_t)p[2] << 16)
                     | ((uint32_t)p[3] << 24);
            }
            else if (addr >= IO_ADDR)
            {
#if ENABLE_SOUND
                if (addr >= 0xFF10 && addr <= 0xFF3F)
                {
                    uint32_t v = 0;
                    for (int i = 0; i < 4; ++i)
                        v |= ((uint32_t)audio_read(addr + i)) << (8 * i);
                    return v;
                }
#endif
                uint8_t *p = &gb->hram_io[addr - IO_ADDR];
                if (((uintptr_t)p & 3) == 0)
                    return *(uint32_t *)p;
                return (uint32_t)p[0]
                     | ((uint32_t)p[1] << 8)
                     | ((uint32_t)p[2] << 16)
                     | ((uint32_t)p[3] << 24);
            }
            return 0xFFFFFFFF;
    }

    (gb->gb_error)(gb, GB_INVALID_READ, addr);
    WGB_UNREACHABLE();
}
#else
uint32_t __gb_read32(struct gb_s *gb, uint16_t addr)
{
    switch (WALNUT_GB_GET_MSN16(addr))
    {
        // --- Boot ROM / Fixed ROM 0
        case 0x0:
            if (gb->hram_io[IO_BOOT] == 0 && addr < 0x0100)
                return (uint32_t)gb->gb_bootrom_read(gb, addr)
                     | ((uint32_t)gb->gb_bootrom_read(gb, addr + 1) << 8)
                     | ((uint32_t)gb->gb_bootrom_read(gb, addr + 2) << 16)
                     | ((uint32_t)gb->gb_bootrom_read(gb, addr + 3) << 24);
#if WALNUT_FULL_GBC_SUPPORT
            else if (gb->cgb.cgbMode && gb->hram_io[IO_BOOT] == 0 &&
                     addr >= 0x0200 && addr < 0x0900)
                return (uint32_t)gb->gb_bootrom_read(gb, addr)
                     | ((uint32_t)gb->gb_bootrom_read(gb, addr + 1) << 8)
                     | ((uint32_t)gb->gb_bootrom_read(gb, addr + 2) << 16)
                     | ((uint32_t)gb->gb_bootrom_read(gb, addr + 3) << 24);
#endif
            /* fallthrough */
        case 0x1:
        case 0x2:
        case 0x3:
            return gb->gb_rom_read_32bit(gb,addr);

        // --- Switchable ROM banks (0x4000–0x7FFF)
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
        {
            uint32_t bank_offset = (gb->selected_rom_bank - 1) * ROM_BANK_SIZE;
            return gb->gb_rom_read_32bit(gb,addr + bank_offset);
        }

        // --- VRAM (0x8000–0x9FFF)
        case 0x8:
        case 0x9:
#if WALNUT_FULL_GBC_SUPPORT
            if (addr + 3 < 0xA000)
                return *(uint32_t *)&gb->vram[addr - gb->cgb.vramBankOffset];
            else
                return *(uint16_t *)&gb->vram[addr - gb->cgb.vramBankOffset]; // fallback
#else
            if (addr + 3 < 0xA000)
                return *(uint32_t *)&gb->vram[addr - VRAM_ADDR];
            else
                return *(uint16_t *)&gb->vram[addr - VRAM_ADDR];
#endif

        // --- External RAM / RTC (0xA000–0xBFFF)
        case 0xA:
        case 0xB:
            if (gb->mbc == 3 && gb->cart_ram_bank >= 0x08)
            {
                // RTC bytes; manual combination
                return (uint32_t)gb->rtc_latched.bytes[gb->cart_ram_bank - 0x08]
                     | ((uint32_t)gb->rtc_latched.bytes[gb->cart_ram_bank - 0x08 + 1] << 8)
                     | ((uint32_t)gb->rtc_latched.bytes[gb->cart_ram_bank - 0x08 + 2] << 16)
                     | ((uint32_t)gb->rtc_latched.bytes[gb->cart_ram_bank - 0x08 + 3] << 24);
            }
            else if (gb->cart_ram && gb->enable_cart_ram)
            {
                uint16_t offset;
                if (gb->mbc == 2)
                {
                    addr &= 0x1FF;
                    offset = addr;
                }
                else if ((gb->cart_mode_select || gb->mbc != 1) &&
                         gb->cart_ram_bank < gb->num_ram_banks)
                {
                    offset = addr - CART_RAM_ADDR +
                             (gb->cart_ram_bank * CRAM_BANK_SIZE);
                }
                else
                {
                    offset = addr - CART_RAM_ADDR;
                }
                return (uint32_t)gb->gb_cart_ram_read(gb, offset)
                     | ((uint32_t)gb->gb_cart_ram_read(gb, offset + 1) << 8)
                     | ((uint32_t)gb->gb_cart_ram_read(gb, offset + 2) << 16)
                     | ((uint32_t)gb->gb_cart_ram_read(gb, offset + 3) << 24);
            }
            return 0xFFFFFFFF;

        // --- Work RAM (0xC000–0xDFFF)
        case 0xC:
        case 0xD:
#if WALNUT_FULL_GBC_SUPPORT
            if (gb->cgb.cgbMode && addr >= WRAM_1_ADDR)
                return *(uint32_t *)&gb->wram[addr - gb->cgb.wramBankOffset];
#endif
            return *(uint32_t *)&gb->wram[addr - WRAM_0_ADDR];

        // --- Echo RAM (0xE000–0xFDFF)
        case 0xE:
            return *(uint32_t *)&gb->wram[addr - ECHO_ADDR];

        // --- OAM / HRAM / IO
        case 0xF:
            if (addr < 0xFEA0)
                return *(uint32_t *)&gb->oam[addr - OAM_ADDR];
            else if (addr >= IO_ADDR)
            {
#if ENABLE_SOUND
                if (addr >= 0xFF10 && addr <= 0xFF3F)
                {
                    uint32_t v = 0;
                    for (int i = 0; i < 4; ++i)
                        v |= ((uint32_t)audio_read(addr + i)) << (8 * i);
                    return v;
                }
#endif
#if WALNUT_FULL_GBC_SUPPORT
                switch (addr & 0xFF)
                {
                    case 0x4D:
                        return gb->cgb.doubleSpeed |
                               ((uint32_t)gb->cgb.doubleSpeedPrep << 8);
                    case 0x4F:
                        return gb->cgb.vramBank | 0xFE00;
                    case 0x51: case 0x52: case 0x53: case 0x54: case 0x55:
                    case 0x56: case 0x68: case 0x69: case 0x6A: case 0x6B:
                    case 0x70:
                        return *(uint32_t *)&gb->hram_io[addr - IO_ADDR];
                    default:
                        return *(uint32_t *)&gb->hram_io[addr - IO_ADDR];
                }
#else
                return *(uint32_t *)&gb->hram_io[addr - IO_ADDR];
#endif
            }
            return 0xFFFFFFFF;
    }

    (gb->gb_error)(gb, GB_INVALID_READ, addr);
    WGB_UNREACHABLE();
}
#endif


/**
 * Internal function used to read bytes.
 * addr is host platform endian.
 */
uint8_t __gb_read(struct gb_s *gb, uint16_t addr)
{
	switch(WALNUT_GB_GET_MSN16(addr))
	{
	case 0x0:
		/* IO_BOOT is only set to 1 if gb->gb_bootrom_read was not NULL
		 * on reset. */
		if(gb->hram_io[IO_BOOT] == 0 && addr < 0x0100)
		{
			return gb->gb_bootrom_read(gb, addr);
		}
#if WALNUT_FULL_GBC_SUPPORT
		else if (gb->cgb.cgbMode && gb->hram_io[IO_BOOT] == 0 && addr < 0x0900 && addr >= 0x0200)
		{
			return gb->gb_bootrom_read(gb, addr);
		}
#endif
		/* Fallthrough */
	case 0x1:
	case 0x2:
	case 0x3:
		return gb->gb_rom_read(gb, addr);

	case 0x4:
	case 0x5:
	case 0x6:
	case 0x7:
		if(gb->mbc == 1 && gb->cart_mode_select)
			return gb->gb_rom_read(gb,
					       addr + ((gb->selected_rom_bank & 0x1F) - 1) * ROM_BANK_SIZE);
		else
			return gb->gb_rom_read(gb, addr + (gb->selected_rom_bank - 1) * ROM_BANK_SIZE);

	case 0x8:
	case 0x9:
#if WALNUT_FULL_GBC_SUPPORT
		return gb->vram[addr - gb->cgb.vramBankOffset];
#else
		return gb->vram[addr - VRAM_ADDR];
#endif
	case 0xA:
	case 0xB:
		if(gb->mbc == 3 && gb->cart_ram_bank >= 0x08)
		{
			return gb->rtc_latched.bytes[gb->cart_ram_bank - 0x08];
		}
		else if(gb->cart_ram && gb->enable_cart_ram)
		{
			if(gb->mbc == 2)
			{
				/* Only 9 bits are available in address. */
				addr &= 0x1FF;
				return gb->gb_cart_ram_read(gb, addr);
			}
			else if((gb->cart_mode_select || gb->mbc != 1) &&
					gb->cart_ram_bank < gb->num_ram_banks)
			{
				return gb->gb_cart_ram_read(gb, addr - CART_RAM_ADDR +
							    (gb->cart_ram_bank * CRAM_BANK_SIZE));
			}
			else
				return gb->gb_cart_ram_read(gb, addr - CART_RAM_ADDR);
		}

		return 0xFF;

	case 0xC:
	case 0xD:
#if WALNUT_FULL_GBC_SUPPORT
	if(gb->cgb.cgbMode && addr >= WRAM_1_ADDR)
		return gb->wram[addr - gb->cgb.wramBankOffset];
#endif
		return gb->wram[addr - WRAM_0_ADDR];

	case 0xE:
		return gb->wram[addr - ECHO_ADDR];

	case 0xF:
		if(addr < OAM_ADDR)
#if WALNUT_FULL_GBC_SUPPORT
			return gb->wram[(addr - 0x2000) - gb->cgb.wramBankOffset];
#else
			return gb->wram[addr - ECHO_ADDR];
#endif

		if(addr < UNUSED_ADDR)
			return gb->oam[addr - OAM_ADDR];

		/* Unusable memory area. Reading from this area returns 0xFF.*/
		if(addr < IO_ADDR)
			return 0xFF;

		/* APU registers. */
		if((addr >= 0xFF10) && (addr <= 0xFF3F))
		{
#if ENABLE_SOUND
			return audio_read(addr);
#else
			static const uint8_t ortab[] = {
				0x80, 0x3f, 0x00, 0xff, 0xbf,
				0xff, 0x3f, 0x00, 0xff, 0xbf,
				0x7f, 0xff, 0x9f, 0xff, 0xbf,
				0xff, 0xff, 0x00, 0x00, 0xbf,
				0x00, 0x00, 0x70,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			};
			return gb->hram_io[addr - IO_ADDR] | ortab[addr - IO_ADDR];
#endif
		}

#if WALNUT_FULL_GBC_SUPPORT
		/* IO and Interrupts. */
		switch (addr & 0xFF)
		{
		/* Speed Switch*/
		case 0x4D:
			return (gb->cgb.doubleSpeed << 7) + gb->cgb.doubleSpeedPrep;
		/* CGB VRAM Bank*/
		case 0x4F:
			return gb->cgb.vramBank | 0xFE;
		/* CGB DMA*/
		case 0x51:
			return (gb->cgb.dmaSource >> 8);
		case 0x52:
			return (gb->cgb.dmaSource & 0xF0);
		case 0x53:
			return (gb->cgb.dmaDest >> 8);
		case 0x54:
			return (gb->cgb.dmaDest & 0xF0);
		case 0x55:
			return (gb->cgb.dmaActive << 7) | (gb->cgb.dmaSize - 1);
		/* IR Register*/
		case 0x56:
			return gb->hram_io[0x56];
		/* CGB BG Palette Index*/
		case 0x68:
			return (gb->cgb.BGPaletteID & 0x3F) + (gb->cgb.BGPaletteInc << 7);
		/* CGB BG Palette*/
		case 0x69:
			return gb->cgb.BGPalette[(gb->cgb.BGPaletteID & 0x3F)];
		/* CGB OAM Palette Index*/
		case 0x6A:
			return (gb->cgb.OAMPaletteID & 0x3F) + (gb->cgb.OAMPaletteInc << 7);
		/* CGB OAM Palette*/
		case 0x6B:
			return gb->cgb.OAMPalette[(gb->cgb.OAMPaletteID & 0x3F)];
		/* CGB WRAM Bank*/
		case 0x70:
			return gb->cgb.wramBank;
		default:
#endif
			/* HRAM */
			if(addr >= IO_ADDR)
				return gb->hram_io[addr - IO_ADDR];
#if WALNUT_FULL_GBC_SUPPORT
		}
#endif
	}


	/* Return address that caused read error. */
	(gb->gb_error)(gb, GB_INVALID_READ, addr);
	WGB_UNREACHABLE();
}



/**
 * Internal function used to write bytes.
 */
void __gb_write(struct gb_s *gb, uint_fast16_t addr, uint8_t val)
{
	switch(WALNUT_GB_GET_MSN16(addr))
	{
	case 0x0:
	case 0x1:
		/* Set RAM enable bit. MBC2 is handled in fall-through. */
		if (gb->mbc > 0 && gb->mbc != 2)
		{
			if (gb->cart_ram)
				gb->enable_cart_ram = ((val & 0x0F) == 0x0A);
			return;
		}

		/* Intentional fall through. */
	case 0x2:
		if (gb->mbc == 5)
		{
			gb->selected_rom_bank =
				(gb->selected_rom_bank & 0x100) | val;
			gb->selected_rom_bank =
				gb->selected_rom_bank & gb->num_rom_banks_mask;
#if WALNUT_GB_SAFE_DUALFETCH_MBC
			gb->prefetch_invalid=true;
#endif
			return;
		}

	/* Intentional fall through. */
	case 0x3:
		if(gb->mbc == 1)
		{
			//selected_rom_bank = val & 0x7;
			gb->selected_rom_bank = (val & 0x1F) | (gb->selected_rom_bank & 0x60);

			if((gb->selected_rom_bank & 0x1F) == 0x00)
				gb->selected_rom_bank++;
#if WALNUT_GB_SAFE_DUALFETCH_MBC
			gb->prefetch_invalid=true;
#endif
		}
		else if(gb->mbc == 2)
		{
			/* If bit 8 is 1, then set ROM bank number. */
			if(addr & 0x100)
			{
				gb->selected_rom_bank = val & 0x0F;
				/* Setting ROM bank to 0, sets it to 1. */
				if(!gb->selected_rom_bank)
					gb->selected_rom_bank++;
			}
			/* Otherwise set whether RAM is enabled or not. */
			else
			{
				gb->enable_cart_ram = ((val & 0x0F) == 0x0A);
				return;
			}
		}
		else if(gb->mbc == 3)
		{
			gb->selected_rom_bank = val;
			if(!gb->cart_is_mbc3O)
				gb->selected_rom_bank = val & 0x7F;

			if(!gb->selected_rom_bank)
				gb->selected_rom_bank++;
		}
		else if(gb->mbc == 5)
			gb->selected_rom_bank = (val & 0x01) << 8 | (gb->selected_rom_bank & 0xFF);

		gb->selected_rom_bank = gb->selected_rom_bank & gb->num_rom_banks_mask;
#if WALNUT_GB_SAFE_DUALFETCH_MBC
		gb->prefetch_invalid=true;
#endif
		return;

	case 0x4:
	case 0x5:
		if(gb->mbc == 1)
		{
			gb->cart_ram_bank = (val & 3);
			gb->selected_rom_bank = ((val & 3) << 5) | (gb->selected_rom_bank & 0x1F);
			gb->selected_rom_bank = gb->selected_rom_bank & gb->num_rom_banks_mask;
		}
		else if(gb->mbc == 3)
		{
			gb->cart_ram_bank = val;
			/* If not using MBC3, only the first 4 cart RAM banks are useable.
			 * If cart RAM bank 0x8-0xC are selected, then the corresponding
			 * RTC register is selected instead of cart RAM. */
			if(!gb->cart_is_mbc3O && gb->cart_ram_bank < 0x8)
				gb->cart_ram_bank &= 0x3;
		}

		else if(gb->mbc == 5)
			gb->cart_ram_bank = (val & 0x0F);
#if WALNUT_GB_SAFE_DUALFETCH_MBC
		gb->prefetch_invalid=true;
#endif
		return;

	case 0x6:
	case 0x7:
		val &= 1;
		if(gb->mbc == 3 && val && gb->cart_mode_select == 0)
			memcpy(&gb->rtc_latched.bytes, &gb->rtc_real.bytes, sizeof(gb->rtc_latched.bytes));

		/* Set banking mode select. */
		gb->cart_mode_select = val;
#if WALNUT_GB_SAFE_DUALFETCH_MBC
		gb->prefetch_invalid=true;
#endif
		return;

	case 0x8:
	case 0x9:
#if WALNUT_FULL_GBC_SUPPORT
		gb->vram[addr - gb->cgb.vramBankOffset] = val;
#else
		gb->vram[addr - VRAM_ADDR] = val;
#endif
		return;

	case 0xA:
	case 0xB:
		if(gb->mbc == 3 && gb->cart_ram_bank >= 0x08)
		{
			const uint8_t rtc_reg_mask[5] = {
				0x3F, 0x3F, 0x1F, 0xFF, 0xC1
			};
			uint8_t reg = gb->cart_ram_bank - 0x08;
			//if(reg == 0) gb->counter.rtc_count = 0;

			gb->rtc_real.bytes[reg] = val & rtc_reg_mask[reg];
		}
		/* Do not write to RAM if unavailable or disabled. */
		else if(gb->cart_ram && gb->enable_cart_ram)
		{
			if(gb->mbc == 2)
			{
				/* Only 9 bits are available in address. */
				addr &= 0x1FF;
				/* Data is only 4 bits wide in MBC2 RAM. */
				val &= 0x0F;
				/* Upper nibble is set to high. */
				val |= 0xF0;
				gb->gb_cart_ram_write(gb, addr, val);
			}
			/* If cart has RAM, use this. If MBC1, only the first
			 * RAM bank can be written to if the advanced banking
			 * mode is selected. */
			else if(((gb->mbc == 1 && gb->cart_mode_select) || gb->mbc != 1) &&
					gb->cart_ram_bank < gb->num_ram_banks)
			{
				gb->gb_cart_ram_write(gb,
					addr - CART_RAM_ADDR + (gb->cart_ram_bank * CRAM_BANK_SIZE), val);
			}
			else if(gb->num_ram_banks)
				gb->gb_cart_ram_write(gb, addr - CART_RAM_ADDR, val);
		}

		return;

	case 0xC:
		gb->wram[addr - WRAM_0_ADDR] = val;
		return;

	case 0xD:
#if WALNUT_FULL_GBC_SUPPORT
		gb->wram[addr - gb->cgb.wramBankOffset] = val;
#else
		gb->wram[addr - WRAM_1_ADDR + WRAM_BANK_SIZE] = val;
#endif
		return;

	case 0xE:
		gb->wram[addr - ECHO_ADDR] = val;
		return;

	case 0xF:
		if(addr < OAM_ADDR)
		{
#if WALNUT_FULL_GBC_SUPPORT
			gb->wram[(addr - 0x2000) - gb->cgb.wramBankOffset] = val;
#else
			gb->wram[addr - ECHO_ADDR] = val;
#endif
			return;
		}

		if(addr < UNUSED_ADDR)
		{
			gb->oam[addr - OAM_ADDR] = val;
			return;
		}

		/* Unusable memory area. */
		if(addr < IO_ADDR)
			return;

		if(HRAM_ADDR <= addr && addr < INTR_EN_ADDR)
		{
			gb->hram_io[addr - IO_ADDR] = val;
			return;
		}

		if((addr >= 0xFF10) && (addr <= 0xFF3F))
		{
#if ENABLE_SOUND
			audio_write(addr, val);
#else
			gb->hram_io[addr - IO_ADDR] = val;
#endif
			return;
		}

		/* IO and Interrupts. */
		switch(WALNUT_GB_GET_LSB16(addr))
		{
		/* Joypad */
		case 0x00:
			/* Only bits 5 and 4 are R/W.
			 * The lower bits are overwritten later, and the two most
			 * significant bits are unused. */
			gb->hram_io[IO_JOYP] = val;

			/* Direction keys selected */
			if((gb->hram_io[IO_JOYP] & 0x10) == 0)
				gb->hram_io[IO_JOYP] |= (gb->direct.joypad >> 4);
			/* Button keys selected */
			else
				gb->hram_io[IO_JOYP] |= (gb->direct.joypad & 0x0F);

			return;

		/* Serial */
		case 0x01:
			gb->hram_io[IO_SB] = val;
			return;

		case 0x02:
			gb->hram_io[IO_SC] = val;
			return;

		/* Timer Registers */
		case 0x04:
			gb->hram_io[IO_DIV] = 0x00;
			return;

		case 0x05:
			gb->hram_io[IO_TIMA] = val;
			return;

		case 0x06:
			gb->hram_io[IO_TMA] = val;
			return;

		case 0x07:
			gb->hram_io[IO_TAC] = val;
			return;

		/* Interrupt Flag Register */
		case 0x0F:
			gb->hram_io[IO_IF] = (val | 0xE0);
			return;

		/* LCD Registers */
		case 0x40:
		{
			uint8_t lcd_enabled;

			/* Check if LCD is already enabled. */
			lcd_enabled = (gb->hram_io[IO_LCDC] & LCDC_ENABLE);

			gb->hram_io[IO_LCDC] = val;

			/* Check if LCD is going to be switched on. */
			if (!lcd_enabled && (val & LCDC_ENABLE))
			{
				gb->lcd_blank = true;
			}
			/* Check if LCD is being switched off. */
			else if (lcd_enabled && !(val & LCDC_ENABLE))
			{
				/* Walnut-GB will happily turn off LCD outside
				 * of VBLANK even though this damages real
				 * hardware. */

				/* Set LCD to Mode 0. */
				gb->hram_io[IO_STAT] =
					(gb->hram_io[IO_STAT] & ~STAT_MODE) |
					IO_STAT_MODE_HBLANK;
				/* LY fixed to 0 when LCD turned off. */
				gb->hram_io[IO_LY] = 0;
				/* Keep track of lcd_count to correctly track
				 * passing time. */
				gb->counter.lcd_off_count += gb->counter.lcd_count;
				/* Reset LCD timer, since the LCD starts from
				 * the beginning on power on. */
				gb->counter.lcd_count = 0;
			}
			return;
		}

		case 0x41:
			gb->hram_io[IO_STAT] = (val & STAT_USER_BITS) | (gb->hram_io[IO_STAT] & STAT_MODE) | 0x80;
			return;

		case 0x42:
			gb->hram_io[IO_SCY] = val;
			return;

		case 0x43:
			gb->hram_io[IO_SCX] = val;
			return;

		/* LY (0xFF44) is read only. */
		case 0x45:
			gb->hram_io[IO_LYC] = val;
			return;

		/* DMA Register */
		case 0x46:
		{
			uint16_t dma_addr;
			uint16_t i;
#if WALNUT_FULL_GBC_SUPPORT
			dma_addr = (uint_fast16_t)(val % 0xF1) << 8;
			gb->hram_io[IO_DMA] = (val % 0xF1);
#else
			dma_addr = (uint_fast16_t)val << 8;
			gb->hram_io[IO_DMA] = val;
#endif
#if WALNUT_GB_32BIT_DMA
#if WALNUT_GB_32BIT_ALIGNED
		/* Alignment aware 32-bit read path: fetch four bytes at a time */
		uint8_t *oam = gb->oam;

		if (((uintptr_t)oam & 3))
				for (i = 0; i < OAM_SIZE; i += 4)
				{
						uint32_t v = __gb_read32(gb, dma_addr + i);
						oam[i+0] = v;
						oam[i+1] = v >> 8;
						oam[i+2] = v >> 16;
						oam[i+3] = v >> 24;
				}
		else
			for (i = 0; i < OAM_SIZE; i += 4)
			{
					uint32_t v = __gb_read32(gb, dma_addr + i);
					*(uint32_t *)(oam + i) = v;
			}
#else
    /* 32-bit read path: fetch four bytes at a time */
    for (i = 0; i < OAM_SIZE; i += 4)
    {
        uint32_t v = __gb_read32(gb, dma_addr + i);
				*((uint32_t *)(gb->oam + i)) = v;
    }
#endif
#elif WALNUT_GB_16BIT_DMA
    /* 16-bit read path: fetch two bytes at a time */
    for (i = 0; i < OAM_SIZE; i += 2)
    {
        uint16_t v = __gb_read16(gb, dma_addr + i);
        gb->oam[i]   = v & 0xFF;
        gb->oam[i+1] = v >> 8;
    }
#else
    /* Original 8-bit read path */
    for (i = 0; i < OAM_SIZE; i++)
        gb->oam[i] = __gb_read(gb, dma_addr + i);
#endif
#if WALNUT_GB_SAFE_DUALFETCH_DMA
		gb->prefetch_invalid=true;
#endif
			return;
		}

		/* DMG Palette Registers */
		case 0x47:
			gb->hram_io[IO_BGP] = val;
			gb->display.bg_palette[0] = (gb->hram_io[IO_BGP] & 0x03);
			gb->display.bg_palette[1] = (gb->hram_io[IO_BGP] >> 2) & 0x03;
			gb->display.bg_palette[2] = (gb->hram_io[IO_BGP] >> 4) & 0x03;
			gb->display.bg_palette[3] = (gb->hram_io[IO_BGP] >> 6) & 0x03;
			return;

		case 0x48:
			gb->hram_io[IO_OBP0] = val;
			gb->display.sp_palette[0] = (gb->hram_io[IO_OBP0] & 0x03);
			gb->display.sp_palette[1] = (gb->hram_io[IO_OBP0] >> 2) & 0x03;
			gb->display.sp_palette[2] = (gb->hram_io[IO_OBP0] >> 4) & 0x03;
			gb->display.sp_palette[3] = (gb->hram_io[IO_OBP0] >> 6) & 0x03;
			return;

		case 0x49:
			gb->hram_io[IO_OBP1] = val;
			gb->display.sp_palette[4] = (gb->hram_io[IO_OBP1] & 0x03);
			gb->display.sp_palette[5] = (gb->hram_io[IO_OBP1] >> 2) & 0x03;
			gb->display.sp_palette[6] = (gb->hram_io[IO_OBP1] >> 4) & 0x03;
			gb->display.sp_palette[7] = (gb->hram_io[IO_OBP1] >> 6) & 0x03;
			return;

		/* Window Position Registers */
		case 0x4A:
			gb->hram_io[IO_WY] = val;
			return;

		case 0x4B:
			gb->hram_io[IO_WX] = val;
			return;

#if WALNUT_FULL_GBC_SUPPORT
		/* Prepare Speed Switch*/
		case 0x4D:
			gb->cgb.doubleSpeedPrep = val & 1;
			return;

		/* CGB VRAM Bank*/
		case 0x4F:
			gb->cgb.vramBank = val & 0x01;
			if(gb->cgb.cgbMode) gb->cgb.vramBankOffset = VRAM_ADDR - (gb->cgb.vramBank << 13);
			return;
#endif
		/* Turn off boot ROM */
		case 0x50:
			gb->hram_io[IO_BOOT] = 0x01;
			return;
#if WALNUT_FULL_GBC_SUPPORT
		/* DMA Register */
		case 0x51:
			gb->cgb.dmaSource = (gb->cgb.dmaSource & 0xFF) + (val << 8);
			return;
		case 0x52:
			gb->cgb.dmaSource = (gb->cgb.dmaSource & 0xFF00) + val;
			return;
		case 0x53:
			gb->cgb.dmaDest = (gb->cgb.dmaDest & 0xFF) + (val << 8);
			return;
		case 0x54:
			gb->cgb.dmaDest = (gb->cgb.dmaDest & 0xFF00) + val;
			return;

		/* DMA Register*/
		case 0x55:
			gb->cgb.dmaSize = (val & 0x7F) + 1;
			gb->cgb.dmaMode = val >> 7;
			//DMA GBC
			if(gb->cgb.dmaActive)
			{  // Only transfer if dma is not active (=1) otherwise treat it as a termination
#if	WALNUT_GB_32BIT_DMA
	      if (gb->cgb.cgbMode && !gb->cgb.dmaMode)
        {
            uint16_t src = gb->cgb.dmaSource & 0xFFF0;
            uint16_t dst = (gb->cgb.dmaDest & 0x1FF0) | 0x8000;
            uint16_t count = gb->cgb.dmaSize << 4;

            for (uint16_t i = 0; i < count; i += 4)
            {
                // 32-bit read from source
                uint32_t val32 = __gb_read32(gb, src + i);
                // 8-bit writes to destination (4 bytes)
								__gb_write32(gb, dst + i, val32);
								//__gb_write16(gb, dst + i + 2, val32 >> 16);
                // __gb_write(gb, dst + i, val32 );
                // __gb_write(gb, dst + i + 1, val32 >> 8);
	              // __gb_write(gb, dst + i + 2, val32 >> 16);
                // __gb_write(gb, dst + i + 3, val32 >> 24);
            }

            gb->cgb.dmaSource += count;
            gb->cgb.dmaDest += count;
            gb->cgb.dmaSize = 0;
#if WALNUT_GB_SAFE_DUALFETCH_DMA
						gb->prefetch_invalid=true;
#endif
        }
#elif WALNUT_GB_16BIT_DMA
        if (gb->cgb.cgbMode && !gb->cgb.dmaMode)
        {
            uint16_t src = gb->cgb.dmaSource & 0xFFF0;
            uint16_t dst = (gb->cgb.dmaDest & 0x1FF0) | 0x8000;
            uint16_t count = gb->cgb.dmaSize << 4;

            for (uint16_t i = 0; i < count; i += 2)
            {
                // 16-bit read from source
                uint16_t val16 = __gb_read16(gb, src + i);
								__gb_write16(gb, dst + i, val16);
                // // 8-bit writes to destination (two bytes)
                // __gb_write(gb, dst + i, val16 & 0xFF);
                // __gb_write(gb, dst + i + 1, val16 >> 8);
            }

            gb->cgb.dmaSource += count;
            gb->cgb.dmaDest += count;
            gb->cgb.dmaSize = 0;
#if WALNUT_GB_SAFE_DUALFETCH_DMA
						gb->prefetch_invalid=true;
#endif
        }
#else
				if(gb->cgb.cgbMode && (!gb->cgb.dmaMode))
				{
					for (int i = 0; i < (gb->cgb.dmaSize << 4); i++)
					{
						__gb_write(gb, ((gb->cgb.dmaDest & 0x1FF0) | 0x8000) + i, __gb_read(gb, (gb->cgb.dmaSource & 0xFFF0) + i));
					}
					gb->cgb.dmaSource += (gb->cgb.dmaSize << 4);
					gb->cgb.dmaDest += (gb->cgb.dmaSize << 4);
					gb->cgb.dmaSize = 0;
#if WALNUT_GB_SAFE_DUALFETCH_DMA
					gb->prefetch_invalid=true;
#endif
				}
#endif			
			}
			gb->cgb.dmaActive = gb->cgb.dmaMode ^ 1;  // set active if it's an HBlank DMA
			return;

		/* IR Register*/
		case 0x56:
			gb->hram_io[0x56] = val;
			return;

		/* CGB BG Palette Index*/
		case 0x68:
			gb->cgb.BGPaletteID = val & 0x3F;
			gb->cgb.BGPaletteInc = val >> 7;
			return;

		/* CGB BG Palette*/
		case 0x69:
			// native rgb565 version
	    gb->cgb.BGPalette[gb->cgb.BGPaletteID & 0x3F] = val;
#if WALNUT_GB_RGB565_BIGENDIAN
			gb->cgb.fixPalette[(gb->cgb.BGPaletteID & 0x3E) >> 1] = bgr555_to_rgb565BE_accurate((gb->cgb.BGPalette[(gb->cgb.BGPaletteID & 0x3E) + 1] << 8) | (gb->cgb.BGPalette[(gb->cgb.BGPaletteID & 0x3E)])); // convert native bgr 555 to rgb565 for native LCD panel rendering
#else
  	  gb->cgb.fixPalette[(gb->cgb.BGPaletteID & 0x3E) >> 1] = bgr555_to_rgb565_accurate((gb->cgb.BGPalette[(gb->cgb.BGPaletteID & 0x3E) + 1] << 8) | (gb->cgb.BGPalette[(gb->cgb.BGPaletteID & 0x3E)])); // convert native bgr 555 to rgb565 for native LCD panel rendering
#endif
			if(gb->cgb.BGPaletteInc) {
				gb->cgb.BGPaletteID++;
				gb->cgb.BGPaletteID = (gb->cgb.BGPaletteID) & 0x3F;
			}
    	return;		

		/* CGB OAM Palette Index*/
		case 0x6A:
			gb->cgb.OAMPaletteID = val & 0x3F;
			gb->cgb.OAMPaletteInc = val >> 7;
			return;

		/* CGB OAM Palette*/
		case 0x6B:
			gb->cgb.OAMPalette[(gb->cgb.OAMPaletteID & 0x3F)] = val;
#if WALNUT_GB_RGB565_BIGENDIAN
			gb->cgb.fixPalette[0x20 + ((gb->cgb.OAMPaletteID & 0x3E) >> 1)] = bgr555_to_rgb565BE_accurate((gb->cgb.OAMPalette[(gb->cgb.OAMPaletteID & 0x3E) + 1] << 8) + (gb->cgb.OAMPalette[(gb->cgb.OAMPaletteID & 0x3E)]));
#else
			gb->cgb.fixPalette[0x20 + ((gb->cgb.OAMPaletteID & 0x3E) >> 1)] = bgr555_to_rgb565_accurate((gb->cgb.OAMPalette[(gb->cgb.OAMPaletteID & 0x3E) + 1] << 8) + (gb->cgb.OAMPalette[(gb->cgb.OAMPaletteID & 0x3E)]));
#endif
			if(gb->cgb.OAMPaletteInc) {
				gb->cgb.OAMPaletteID++;
				gb->cgb.OAMPaletteID = (gb->cgb.OAMPaletteID) & 0x3F;
			}		
			return;

		/* CGB WRAM Bank*/
		case 0x70:
			gb->cgb.wramBank = val;
			gb->cgb.wramBankOffset = WRAM_1_ADDR - (1 << 12);
			if(gb->cgb.cgbMode && (gb->cgb.wramBank & 7) > 0) gb->cgb.wramBankOffset = WRAM_1_ADDR - ((gb->cgb.wramBank & 7) << 12);
			return;
#endif

		/* Interrupt Enable Register */
		case 0xFF:
			gb->hram_io[IO_IE] = val;
			return;
		}
	}

	/* Invalid writes are ignored. */
	return;
}

#if WALNUT_GB_32BIT_ALIGNED
void __gb_write32(struct gb_s *gb, uint16_t addr, uint32_t val) {
    uint8_t *dst = NULL;

    switch (WALNUT_GB_GET_MSN16(addr)) {
        case 0x8: // VRAM
        case 0x9:
#if WALNUT_FULL_GBC_SUPPORT
            dst = &gb->vram[addr - gb->cgb.vramBankOffset];
#else
            dst = &gb->vram[addr - VRAM_ADDR];
#endif
            break;

        case 0xC: // WRAM bank 0
        case 0xD:
#if WALNUT_FULL_GBC_SUPPORT
            dst = &gb->wram[addr - gb->cgb.wramBankOffset];
#else
            dst = &gb->wram[addr - WRAM_1_ADDR + WRAM_BANK_SIZE];
#endif
            break;

        case 0xE: // Echo RAM
            dst = &gb->wram[addr - ECHO_ADDR];
            break;

        case 0xF: // HRAM / OAM
            if (addr < OAM_ADDR) {
#if WALNUT_FULL_GBC_SUPPORT
                dst = &gb->wram[(addr - 0x2000) - gb->cgb.wramBankOffset];
#else
                dst = &gb->wram[addr - ECHO_ADDR];
#endif
            } else if (addr < UNUSED_ADDR) {
                dst = &gb->oam[addr - OAM_ADDR];
            }
            break;

        default:
            break;
    }

    if (dst) {
        if (((uintptr_t)dst & 3) == 0) {
            /* Aligned 32-bit store */
            *(uint32_t *)dst = val;
        } else {
            /* Fallback: byte-wise store */
            dst[0] = (uint8_t)(val);
            dst[1] = (uint8_t)(val >> 8);
            dst[2] = (uint8_t)(val >> 16);
            dst[3] = (uint8_t)(val >> 24);
        }
        return;
    }

    /* Fallback for special addresses or side-effectful regions */
    __gb_write(gb, addr + 0, (uint8_t)(val));
    __gb_write(gb, addr + 1, (uint8_t)(val >> 8));
    __gb_write(gb, addr + 2, (uint8_t)(val >> 16));
    __gb_write(gb, addr + 3, (uint8_t)(val >> 24));
}
#else
void __gb_write32(struct gb_s *gb, uint16_t addr, uint32_t val) {
    switch (WALNUT_GB_GET_MSN16(addr)) {
        case 0x8: // VRAM
        case 0x9:
#if WALNUT_FULL_GBC_SUPPORT
            *(uint32_t*)&gb->vram[addr - gb->cgb.vramBankOffset] = val;
#else
            *(uint32_t*)&gb->vram[addr - VRAM_ADDR] = val;
#endif
            return;

        case 0xC: // WRAM bank 0
        case 0xD:
#if WALNUT_FULL_GBC_SUPPORT
            *(uint32_t*)&gb->wram[addr - gb->cgb.wramBankOffset] = val;
#else
            *(uint32_t*)&gb->wram[addr - WRAM_1_ADDR + WRAM_BANK_SIZE] = val;
#endif
            return;

        case 0xE: // Echo RAM
            *(uint32_t*)&gb->wram[addr - ECHO_ADDR] = val;
            return;

        case 0xF: // HRAM / OAM
            if(addr < OAM_ADDR) {
#if WALNUT_FULL_GBC_SUPPORT
                *(uint32_t*)&gb->wram[(addr - 0x2000) - gb->cgb.wramBankOffset] = val;
#else
                *(uint32_t*)&gb->wram[addr - ECHO_ADDR] = val;
#endif
                return;
            }
            if(addr < UNUSED_ADDR) {
                *(uint32_t*)&gb->oam[addr - OAM_ADDR] = val;
                return;
            }
            break;

        default:
            break;
    }

    // fallback for special addresses or side-effectful regions
    __gb_write(gb, addr, val & 0xFF);
    __gb_write(gb, addr + 1, (val >> 8) & 0xFF);
    __gb_write(gb, addr + 2, (val >> 16) & 0xFF);
    __gb_write(gb, addr + 3, (val >> 24) & 0xFF);
}
#endif

// The 16-bit write function is mainly for DMA transfers so focuses on accesible memory regions and falls back to the 8-bit version for all other cases.
void __gb_write16(struct gb_s *gb, uint_fast16_t addr, uint16_t val)
{
    switch(WALNUT_GB_GET_MSN16(addr))
    {
        case 0x8: // VRAM
        case 0x9:
#if WALNUT_FULL_GBC_SUPPORT
            *((uint16_t *)(gb->vram + (addr - gb->cgb.vramBankOffset))) = val;
#else
            *((uint16_t *)(gb->vram + (addr - VRAM_ADDR))) = val;
#endif
            return;

        case 0xA: // Cartridge RAM / RTC
        case 0xB:
            if(gb->mbc == 3 && gb->cart_ram_bank >= 0x08)
            {
                // RTC: must go through __gb_write for side effects
                __gb_write(gb, addr, val & 0xFF);
                __gb_write(gb, addr + 1, val >> 8);
            }
            else if(gb->cart_ram && gb->enable_cart_ram)
            {
                gb->gb_cart_ram_write(gb, addr - CART_RAM_ADDR + (gb->cart_ram_bank * CRAM_BANK_SIZE), val & 0xFF);
                gb->gb_cart_ram_write(gb, addr - CART_RAM_ADDR + 1 + (gb->cart_ram_bank * CRAM_BANK_SIZE), val >> 8);
            }
            return;

        case 0xC: // WRAM bank 0
#if WALNUT_FULL_GBC_SUPPORT
        case 0xD: // WRAM bank 1..N
            *((uint16_t *)(gb->wram + (addr - gb->cgb.wramBankOffset))) = val;
#else
        case 0xD:
            *((uint16_t *)(gb->wram + (addr - WRAM_1_ADDR + WRAM_BANK_SIZE))) = val;
#endif
            return;

        case 0xE: // Echo RAM
            *((uint16_t *)(gb->wram + (addr - ECHO_ADDR))) = val;
            return;

        case 0xF:
            if(addr < OAM_ADDR) // Echo / WRAM mirrored
            {
#if WALNUT_FULL_GBC_SUPPORT
                *((uint16_t *)(gb->wram + (addr - 0x2000 - gb->cgb.wramBankOffset))) = val;
#else
                *((uint16_t *)(gb->wram + (addr - ECHO_ADDR))) = val;
#endif
            }
            else if(addr < UNUSED_ADDR || (addr >= HRAM_ADDR && addr < INTR_EN_ADDR))
            {
                // Special regions must fall back
                __gb_write(gb, addr, val & 0xFF);
                __gb_write(gb, addr + 1, val >> 8);
            }
            else
            {
                // Any other IO / DMA / palette / timer regions
                __gb_write(gb, addr, val & 0xFF);
                __gb_write(gb, addr + 1, val >> 8);
            }
            return;

        default:
            // Safety fallback for anything not covered
            __gb_write(gb, addr, val & 0xFF);
            __gb_write(gb, addr + 1, val >> 8);
            return;
    }
}

