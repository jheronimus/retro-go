#include "shared.h"

uint8_t bg_name_dirty[0x200];
uint16_t bg_name_list[0x200];
uint16_t bg_list_index;
uint8_t *linebuf;
uint8_t sms_cram_expand_table[4];
uint8_t gg_cram_expand_table[16];

typedef struct {
    int32_t yrange;
    int32_t xpos;
    int32_t attr;
} object_info_t;

static object_info_t object_info[8];
static int32_t object_index_count = 0;

static void parse_satb(int32_t line)
{
	uint8_t *st = (uint8_t *)&vdp.vram[vdp.satb];
	int32_t i = 0;
	uint8_t yp;
	uint8_t height = 8;
	uint8_t zoomed = vdp.reg[1] & 0x01;
  
	if(vdp.reg[1] & 0x02) 
		height <<= 1;
	if(zoomed)
		height <<= 1;

	object_index_count = 0;

	for(i = 0; i < 64; i++)
	{
		yp = st[i];
		if(vdp.extended == 0 && yp == 208)
			return;
		if(yp > 240) yp -= 256;
		yp |= (zoomed);
 		yp = line - yp;
		if(yp < height)
		{
			if (object_index_count == 8)
			{
				if (line < vdp.height)
				    vdp.spr_ovr = 1;
				if (option.spritelimit)
					return;
			}
			object_info[object_index_count].yrange = yp;
			object_info[object_index_count].xpos = st[0x80 + (i << 1)];
			object_info[object_index_count].attr = st[0x81 + (i << 1)];
			vdp.status |= 0x20; ++object_index_count;
		}
	}
}

void render_init(void) {}
void render_shutdown(void) {}
void render_reset(void) {}
void render_line(int32_t line) {
    parse_satb(line);
}
void palette_sync(int32_t index) {}
