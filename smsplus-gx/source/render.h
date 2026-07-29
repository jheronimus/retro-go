#ifndef _RENDER_H_
#define _RENDER_H_
extern uint8_t bg_name_dirty[0x200];
extern uint16_t bg_name_list[0x200];
extern uint16_t bg_list_index;
extern uint8_t *linebuf;
extern uint8_t sms_cram_expand_table[4];
extern uint8_t gg_cram_expand_table[16];

extern void render_init(void);
extern void render_shutdown(void);
extern void render_reset(void);
extern void render_line(int32_t line);
extern void palette_sync(int32_t index);
#endif
