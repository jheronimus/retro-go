#include "shared.h"
#include "main.h"
#include "screen.h"
#include "memory.h"
#include "event.h"
#include "io.h"
#include "video.h"
#include "common.h"
#include "audio.h"

static rg_app_t *app;
static rg_surface_t *updates[2];
static rg_surface_t *currentUpdate;

static void event_handler(int event, void *arg) {
    if (event == RG_EVENT_REDRAW) rg_display_submit(currentUpdate, 0);
}
static bool screenshot_handler(const char *filename, int width, int height) { return false; }
static bool save_state_handler(const char *filename) { return true; }
static bool load_state_handler(const char *filename) { return true; }
static bool reset_handler(bool hard) { reset_pce(); return true; }
static void options_handler(rg_gui_option_t *dest) { *dest = (rg_gui_option_t)RG_DIALOG_END; }

void pce_main(void)
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

    updates[0] = rg_surface_create(320, 240, RG_PIXEL_565, MEM_FAST);
    updates[1] = rg_surface_create(320, 240, RG_PIXEL_565, MEM_FAST);
    currentUpdate = updates[0];

    memset(&config, 0, sizeof(config));
    config.enable_sound = 0;
    
    initialize_pce();

    if (load_rom((char*)app->romPath) == -1)
        RG_PANIC("ROM load failed.");

    reset_pce();
    rg_system_set_tick_rate(60);

    while (true)
    {
        uint32_t joystick = rg_input_read_gamepad();
        if (joystick & (RG_KEY_MENU|RG_KEY_OPTION)) rg_gui_game_menu();

        int64_t startTime = rg_system_timer();

        io.button_status[0] = 0;
        if (joystick & RG_KEY_UP)     io.button_status[0] |= IO_BUTTON_UP;
        if (joystick & RG_KEY_DOWN)   io.button_status[0] |= IO_BUTTON_DOWN;
        if (joystick & RG_KEY_LEFT)   io.button_status[0] |= IO_BUTTON_LEFT;
        if (joystick & RG_KEY_RIGHT)  io.button_status[0] |= IO_BUTTON_RIGHT;
        if (joystick & RG_KEY_A)      io.button_status[0] |= IO_BUTTON_I;
        if (joystick & RG_KEY_B)      io.button_status[0] |= IO_BUTTON_II;
        if (joystick & RG_KEY_SELECT) io.button_status[0] |= IO_BUTTON_SELECT;
        if (joystick & RG_KEY_START)  io.button_status[0] |= IO_BUTTON_RUN;

        update_frame(0);

        uint16_t *src = (uint16_t*)get_screen_ptr();
        if (src) {
            uint16_t *dst = (uint16_t *)currentUpdate->data;
            for (int i = 0; i < 320 * 240; i++) dst[i] = src[i];
        }

        rg_display_submit(currentUpdate, 0);
        currentUpdate = updates[currentUpdate == updates[0]];

        rg_system_tick(rg_system_timer() - startTime);

        int frameTime = 1000000 / 60;
        int elapsed = rg_system_timer() - startTime;
        if (elapsed < frameTime) rg_system_sleep((frameTime - elapsed) / 1000);
    }
}
