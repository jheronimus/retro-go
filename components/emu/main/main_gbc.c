#include "shared.h"
#include <walnut_cgb.h>

static int skipFrames = 0;
static bool slowFrame = false;

static rg_app_t *app;
static rg_surface_t *updates[2];
static rg_surface_t *currentUpdate;
static struct gb_s gb_ctx;

struct priv_s {
    uint8_t *rom;
    size_t rom_size;
    uint8_t *sram;
    size_t sram_size;
};

static struct priv_s priv_data;

static uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr)
{
    const struct priv_s *p = (const struct priv_s *)gb->direct.priv;
    return addr < p->rom_size ? p->rom[addr] : 0xFF;
}

static uint16_t gb_rom_read16(struct gb_s *gb, const uint_fast32_t addr)
{
    const struct priv_s *p = (const struct priv_s *)gb->direct.priv;
    if (addr + 1 < p->rom_size) {
        return p->rom[addr] | (p->rom[addr+1] << 8);
    }
    return 0xFFFF;
}

static uint32_t gb_rom_read32(struct gb_s *gb, const uint_fast32_t addr)
{
    const struct priv_s *p = (const struct priv_s *)gb->direct.priv;
    if (addr + 3 < p->rom_size) {
        return p->rom[addr] | (p->rom[addr+1] << 8) | (p->rom[addr+2] << 16) | (p->rom[addr+3] << 24);
    }
    return 0xFFFFFFFF;
}

static uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr)
{
    const struct priv_s *p = (const struct priv_s *)gb->direct.priv;
    return addr < p->sram_size ? p->sram[addr] : 0xFF;
}

static void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr, const uint8_t val)
{
    struct priv_s *p = (struct priv_s *)gb->direct.priv;
    if (addr < p->sram_size) p->sram[addr] = val;
}

static void gb_error(struct gb_s *gb, const enum gb_error_e err, const uint16_t addr)
{
    RG_PANIC("GB Error!");
}

static void event_handler(int event, void *arg) { }
static bool screenshot_handler(const char *filename, int width, int height) { return false; }
static bool save_state_handler(const char *filename) { return true; }
static bool load_state_handler(const char *filename) { return true; }
static bool reset_handler(bool hard) { gb_reset(&gb_ctx); return true; }
static void options_handler(rg_gui_option_t *dest) { *dest = (rg_gui_option_t)RG_DIALOG_END; }

static void lcd_draw_line(struct gb_s *gb, const uint8_t *pixels, const uint_fast8_t line)
{
    uint16_t *dest = (uint16_t*)currentUpdate->data + line * 160;
    for (int i = 0; i < 160; i++)
    {
        uint8_t c = pixels[i];
        dest[i] = ((c & 3) * 0x1f) << 11; // dummy color
    }
}

void gbc_main(void) // entrypoint name might be gb_main or gnuboy_main, we'll keep gnuboy_main if CMake had gnuboy
{
    const rg_handlers_t handlers = {
        .loadState = &load_state_handler,
        .saveState = &save_state_handler,
        .reset = &reset_handler,
        .event = &event_handler,
        .screenshot = &screenshot_handler,
        .options = &options_handler,
    };
    app = rg_system_reinit(0, &handlers, NULL);

    updates[0] = rg_surface_create(160, 144, RG_PIXEL_565, MEM_FAST);
    updates[1] = rg_surface_create(160, 144, RG_PIXEL_565, MEM_FAST);
    currentUpdate = updates[0];

    priv_data.rom = rg_storage_read_file(app->romPath, &priv_data.rom_size);
    if (!priv_data.rom) RG_PANIC("ROM load failed.");

    priv_data.sram_size = 32768;
    priv_data.sram = malloc(priv_data.sram_size);

    if (gb_init(&gb_ctx, &gb_rom_read, &gb_rom_read16, &gb_rom_read32, &gb_cart_ram_read, &gb_cart_ram_write, &gb_error, &priv_data) != GB_INIT_NO_ERROR)
        RG_PANIC("GB init failed");

    gb_init_lcd(&gb_ctx, &lcd_draw_line);
    rg_system_set_tick_rate(60);

    while (true)
    {
        uint32_t joystick = rg_input_read_gamepad();
        if (joystick & (RG_KEY_MENU|RG_KEY_OPTION)) rg_gui_game_menu();

        int64_t startTime = rg_system_timer();
        
        gb_ctx.direct.joypad = 0xFF;
        if (joystick & RG_KEY_START)  gb_ctx.direct.joypad &= ~(1 << 7);
        if (joystick & RG_KEY_SELECT) gb_ctx.direct.joypad &= ~(1 << 6);
        if (joystick & RG_KEY_A)      gb_ctx.direct.joypad &= ~(1 << 4);
        if (joystick & RG_KEY_B)      gb_ctx.direct.joypad &= ~(1 << 5);
        if (joystick & RG_KEY_UP)     gb_ctx.direct.joypad &= ~(1 << 2);
        if (joystick & RG_KEY_DOWN)   gb_ctx.direct.joypad &= ~(1 << 3);
        if (joystick & RG_KEY_LEFT)   gb_ctx.direct.joypad &= ~(1 << 1);
        if (joystick & RG_KEY_RIGHT)  gb_ctx.direct.joypad &= ~(1 << 0);

        gb_run_frame(&gb_ctx);

        rg_display_submit(currentUpdate, 0);
        currentUpdate = updates[currentUpdate == updates[0]];

        rg_system_tick(rg_system_timer() - startTime);

        int frameTime = 1000000 / 60;
        int elapsed = rg_system_timer() - startTime;
        if (elapsed < frameTime) rg_system_sleep((frameTime - elapsed) / 1000);
    }
}
