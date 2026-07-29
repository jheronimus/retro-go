#include "shared.h"
#include "system.h"
#include "sms.h"
#include "loadrom.h"

// system_manage_sram needed by sms
void system_manage_sram(uint8_t *sram, uint8_t slot_number, uint8_t mode)
{
    (void)sram; (void)slot_number; (void)mode;
}

// option var needed by sms
t_config option = { 0 };
static uint16_t sms_bitmap_buf[256 * 240];

static rg_app_t *app;
static rg_surface_t *updates[2];
static rg_surface_t *currentUpdate;

static void event_handler(int event, void *arg) {
    if (event == RG_EVENT_REDRAW) rg_display_submit(currentUpdate, 0);
}
static bool screenshot_handler(const char *filename, int width, int height) { return false; }
static bool save_state_handler(const char *filename) { return true; }
static bool load_state_handler(const char *filename) { return true; }
static bool reset_handler(bool hard) { system_reset(); return true; }
static void options_handler(rg_gui_option_t *dest) { *dest = (rg_gui_option_t)RG_DIALOG_END; }

void sms_main(void)
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

    updates[0] = rg_surface_create(256, 192, RG_PIXEL_565_LE, MEM_FAST);
    updates[1] = rg_surface_create(256, 192, RG_PIXEL_565_LE, MEM_FAST);
    currentUpdate = updates[0];

    memset(&option, 0, sizeof(option));
    option.fullscreen = 1;
    option.fm = 1;
    option.nosound = 1;

    bitmap.width = 256;
    bitmap.height = 192;
    bitmap.depth = 16;
    bitmap.data = (uint8_t *)sms_bitmap_buf;
    bitmap.pitch = 256 * sizeof(uint16_t);

    if (!load_rom((char*)app->romPath))
        RG_PANIC("ROM load failed.");

    system_poweron();
    rg_system_set_tick_rate(60);

    while (true)
    {
        uint32_t joystick = rg_input_read_gamepad();
        if (joystick & (RG_KEY_MENU|RG_KEY_OPTION)) rg_gui_game_menu();

        int64_t startTime = rg_system_timer();

        input.pad[0] = 0;
        if (joystick & RG_KEY_UP)     input.pad[0] |= INPUT_UP;
        if (joystick & RG_KEY_DOWN)   input.pad[0] |= INPUT_DOWN;
        if (joystick & RG_KEY_LEFT)   input.pad[0] |= INPUT_LEFT;
        if (joystick & RG_KEY_RIGHT)  input.pad[0] |= INPUT_RIGHT;
        if (joystick & RG_KEY_A)      input.pad[0] |= INPUT_BUTTON1;
        if (joystick & RG_KEY_B)      input.pad[0] |= INPUT_BUTTON2;

        input.system = 0;
        if (joystick & RG_KEY_START)  input.system |= INPUT_START;

        system_frame(0);

        // Copy frame buffer
        uint16_t *src = sms_bitmap_buf;
        uint16_t *dst = (uint16_t *)currentUpdate->data;
        for (int i = 0; i < 256 * 192; i++) dst[i] = src[i];

        rg_display_submit(currentUpdate, 0);
        currentUpdate = updates[currentUpdate == updates[0]];

        rg_system_tick(rg_system_timer() - startTime);

        int frameTime = 1000000 / 60;
        int elapsed = rg_system_timer() - startTime;
        if (elapsed < frameTime) rg_task_delay((frameTime - elapsed) / 1000);
    }
}
