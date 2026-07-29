#include "walnut_cgb.h"

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
void __gb_draw_line(struct gb_s *gb)
{
	uint8_t pixels[160] = {0};
	const uint8_t hram_io_ly = gb->hram_io[IO_LY];
#if WALNUT_FULL_GBC_SUPPORT
	const uint8_t cgbMode = gb->cgb.cgbMode;
#endif
	/* If LCD not initialised by front-end, don't render anything. */
	if(gb->display.lcd_draw_line == NULL)
		return;

	if(gb->direct.frame_skip && !gb->display.frame_skip_count)
		return;

#if WALNUT_FULL_GBC_SUPPORT
	uint8_t pixelsPrio[160] = {0};  //do these pixels have priority over OAM?
#endif
	/* If interlaced mode is activated, check if we need to draw the current
	 * line. */
	if(gb->direct.interlace)
	{
		if((!gb->display.interlace_count
				&& (hram_io_ly & 1) == 0)
				|| (gb->display.interlace_count
				    && (hram_io_ly & 1) == 1))
		{
			/* Compensate for missing window draw if required. */
			if(gb->hram_io[IO_LCDC] & LCDC_WINDOW_ENABLE
					&& hram_io_ly >= gb->display.WY
					&& gb->hram_io[IO_WX] <= 166)
				gb->display.window_clear++;

			return;
		}
	}

	/* If background is enabled, draw it. */
#if WALNUT_FULL_GBC_SUPPORT
	if(cgbMode || gb->hram_io[IO_LCDC] & LCDC_BG_ENABLE)
#else
	if(gb->hram_io[IO_LCDC] & LCDC_BG_ENABLE)
#endif
	{
		uint8_t bg_y, disp_x, bg_x, idx, py, px, t1, t2;
		uint16_t bg_map, tile;

		/* Calculate current background line to draw. Constant because
		 * this function draws only this one line each time it is
		 * called. */
		bg_y = hram_io_ly + gb->hram_io[IO_SCY];

		/* Get selected background map address for first tile
		 * corresponding to current line.
		 * 0x20 (32) is the width of a background tile, and the bit
		 * shift is to calculate the address. */
		bg_map =
			((gb->hram_io[IO_LCDC] & LCDC_BG_MAP) ?
			 VRAM_BMAP_2 : VRAM_BMAP_1)
			+ (bg_y >> 3) * 0x20;

		/* The displays (what the player sees) X coordinate, drawn right
		 * to left. */
		disp_x = LCD_WIDTH - 1;

		/* The X coordinate to begin drawing the background at. */
		bg_x = disp_x + gb->hram_io[IO_SCX];

		/* Get tile index for current background tile. */
		idx = gb->vram[bg_map + (bg_x >> 3)];
#if WALNUT_FULL_GBC_SUPPORT
		uint8_t idxAtt = gb->vram[bg_map + (bg_x >> 3) + 0x2000];
#endif
		/* Y coordinate of tile pixel to draw. */
		py = (bg_y & 0x07);
		/* X coordinate of tile pixel to draw. */
		px = 7 - (bg_x & 0x07);

		/* Select addressing mode. */
		if(gb->hram_io[IO_LCDC] & LCDC_TILE_SELECT)
			tile = VRAM_TILES_1 + idx * 0x10;
		else
			tile = VRAM_TILES_2 + ((idx + 0x80) % 0x100) * 0x10;

#if WALNUT_FULL_GBC_SUPPORT
		if(cgbMode)
		{
			if(idxAtt & 0x08) tile += 0x2000; //VRAM bank 2
			if(idxAtt & 0x40) tile += 2 * (7 - py);
		}
		if(!(idxAtt & 0x40))
		{
			tile += 2 * py;
		}

		/* fetch first tile */
		if(cgbMode && (idxAtt & 0x20))
		{  //Horizantal Flip
			t1 = gb->vram[tile] << px;
			t2 = gb->vram[tile + 1] << px;
		}
		else
		{
			t1 = gb->vram[tile] >> px;
			t2 = gb->vram[tile + 1] >> px;
		}
#else
		tile += 2 * py;

		/* fetch first tile */
		t1 = gb->vram[tile] >> px;
		t2 = gb->vram[tile + 1] >> px;
#endif
		
		for(; disp_x != 0xFF; disp_x--)
		{
			uint8_t c;

			if(px == 8)
			{
				/* fetch next tile */
				px = 0;
				bg_x = disp_x + gb->hram_io[IO_SCX];
				idx = gb->vram[bg_map + (bg_x >> 3)];
#if WALNUT_FULL_GBC_SUPPORT
				idxAtt = gb->vram[bg_map + (bg_x >> 3) + 0x2000];
#endif
				if(gb->hram_io[IO_LCDC] & LCDC_TILE_SELECT)
					tile = VRAM_TILES_1 + idx * 0x10;
				else
					tile = VRAM_TILES_2 + ((idx + 0x80) % 0x100) * 0x10;

#if WALNUT_FULL_GBC_SUPPORT
				if(cgbMode)
				{
					if(idxAtt & 0x08) tile += 0x2000; //VRAM bank 2
					if(idxAtt & 0x40) tile += 2 * (7 - py);
				}
				if(!(idxAtt & 0x40))
				{
					tile += 2 * py;
				}
#else
				tile += 2 * py;
#endif
				t1 = gb->vram[tile];
				t2 = gb->vram[tile + 1];
			}

			/* copy background */
#if WALNUT_FULL_GBC_SUPPORT
			if(cgbMode && (idxAtt & 0x20))
			{  //Horizantal Flip
				c = (((t1 & 0x80) >> 1) | (t2 & 0x80)) >> 6;
				pixels[disp_x] = ((idxAtt & 0x07) << 2) + c;
				pixelsPrio[disp_x] = (idxAtt >> 7);
				t1 = t1 << 1;
				t2 = t2 << 1;
			}
			else
			{
				c = (t1 & 0x1) | ((t2 & 0x1) << 1);
				if(cgbMode)
				{
					pixels[disp_x] = ((idxAtt & 0x07) << 2) + c;
					pixelsPrio[disp_x] = (idxAtt >> 7);
				}
				else
				{
					pixels[disp_x] = gb->display.bg_palette[c];
#if WALNUT_GB_12_COLOUR
					pixels[disp_x] |= LCD_PALETTE_BG;
#endif
				}
				t1 = t1 >> 1;
				t2 = t2 >> 1;
			}
#else
			c = (t1 & 0x1) | ((t2 & 0x1) << 1);
			pixels[disp_x] = gb->display.bg_palette[c];
#if WALNUT_GB_12_COLOUR
			pixels[disp_x] |= LCD_PALETTE_BG;
#endif
			t1 = t1 >> 1;
			t2 = t2 >> 1;
#endif
			px++;
		}
	}

	/* draw window */
	if(gb->hram_io[IO_LCDC] & LCDC_WINDOW_ENABLE
			&& hram_io_ly >= gb->display.WY
			&& gb->hram_io[IO_WX] <= 166)
	{
		uint16_t win_line, tile;
		uint8_t disp_x, win_x, py, px, idx, t1, t2, end;

		/* Calculate Window Map Address. */
		win_line = (gb->hram_io[IO_LCDC] & LCDC_WINDOW_MAP) ?
				    VRAM_BMAP_2 : VRAM_BMAP_1;
		win_line += (gb->display.window_clear >> 3) * 0x20;

		disp_x = LCD_WIDTH - 1;
		win_x = disp_x - gb->hram_io[IO_WX] + 7;

		// look up tile
		py = gb->display.window_clear & 0x07;
		px = 7 - (win_x & 0x07);
		idx = gb->vram[win_line + (win_x >> 3)];
#if WALNUT_FULL_GBC_SUPPORT
		uint8_t idxAtt = gb->vram[win_line + (win_x >> 3) + 0x2000];
#endif

		if(gb->hram_io[IO_LCDC] & LCDC_TILE_SELECT)
			tile = VRAM_TILES_1 + idx * 0x10;
		else
			tile = VRAM_TILES_2 + ((idx + 0x80) % 0x100) * 0x10;

#if WALNUT_FULL_GBC_SUPPORT
		if(cgbMode)
		{
			if(idxAtt & 0x08) tile += 0x2000; //VRAM bank 2
			if(idxAtt & 0x40) tile += 2 * (7 - py);
		}
		if(!(idxAtt & 0x40))
		{
			tile += 2 * py;
		}

		// fetch first tile
		if(cgbMode && (idxAtt & 0x20))
		{  //Horizantal Flip
			t1 = gb->vram[tile] << px;
			t2 = gb->vram[tile + 1] << px;
		}
		else
		{
			t1 = gb->vram[tile] >> px;
			t2 = gb->vram[tile + 1] >> px;
		}
#else
		tile += 2 * py;

		// fetch first tile
		t1 = gb->vram[tile] >> px;
		t2 = gb->vram[tile + 1] >> px;
#endif
		// loop & copy window
		end = (gb->hram_io[IO_WX] < 7 ? 0 : gb->hram_io[IO_WX] - 7) - 1;

		for(; disp_x != end; disp_x--)
		{
			uint8_t c;

			if(px == 8)
			{
				// fetch next tile
				px = 0;
				win_x = disp_x - gb->hram_io[IO_WX] + 7;
				idx = gb->vram[win_line + (win_x >> 3)];
#if WALNUT_FULL_GBC_SUPPORT
				idxAtt = gb->vram[win_line + (win_x >> 3) + 0x2000];
#endif

				if(gb->hram_io[IO_LCDC] & LCDC_TILE_SELECT)
					tile = VRAM_TILES_1 + idx * 0x10;
				else
					tile = VRAM_TILES_2 + ((idx + 0x80) % 0x100) * 0x10;

#if WALNUT_FULL_GBC_SUPPORT
				if(cgbMode)
				{
					if(idxAtt & 0x08) tile += 0x2000; //VRAM bank 2
					if(idxAtt & 0x40) tile += 2 * (7 - py);
				}
				if(!(idxAtt & 0x40))
				{
					tile += 2 * py;
				}
#else
				tile += 2 * py;
#endif
				t1 = gb->vram[tile];
				t2 = gb->vram[tile + 1];
			}

			// copy window
#if WALNUT_FULL_GBC_SUPPORT
			if(idxAtt & 0x20)
			{  //Horizantal Flip
				c = (((t1 & 0x80) >> 1) | (t2 & 0x80)) >> 6;
				pixels[disp_x] = ((idxAtt & 0x07) << 2) + c;
				pixelsPrio[disp_x] = (idxAtt >> 7);
				t1 = t1 << 1;
				t2 = t2 << 1;
			}
			else
			{
				c = (t1 & 0x1) | ((t2 & 0x1) << 1);
				if(cgbMode)
				{
					pixels[disp_x] = ((idxAtt & 0x07) << 2) + c;
					pixelsPrio[disp_x] = (idxAtt >> 7);
				}
				else
				{
					pixels[disp_x] = gb->display.bg_palette[c];
#if WALNUT_GB_12_COLOUR
					pixels[disp_x] |= LCD_PALETTE_BG;
#endif
				}
				t1 = t1 >> 1;
				t2 = t2 >> 1;
			}
#else
			c = (t1 & 0x1) | ((t2 & 0x1) << 1);
			pixels[disp_x] = gb->display.bg_palette[c];
#if WALNUT_GB_12_COLOUR
			pixels[disp_x] |= LCD_PALETTE_BG;
#endif
			t1 = t1 >> 1;
			t2 = t2 >> 1;
#endif
			px++;
		}

		gb->display.window_clear++; // advance window line
	}

	// draw sprites
	if(gb->hram_io[IO_LCDC] & LCDC_OBJ_ENABLE)
	{
		uint8_t sprite_number;
#if WALNUT_GB_HIGH_LCD_ACCURACY
		uint8_t number_of_sprites = 0;

		struct sprite_data sprites_to_render[MAX_SPRITES_LINE + 1]; // Requires one extra slot for sorting 

		/* Record number of sprites on the line being rendered, limited
		 * to the maximum number sprites that the Game Boy is able to
		 * render on each line (10 sprites). */
		for(sprite_number = 0;
				sprite_number < NUM_SPRITES;
				sprite_number++)
		{
			/* Sprite Y position. */
			uint8_t OY = gb->oam[4 * sprite_number + 0];
			/* Sprite X position. */
			uint8_t OX = gb->oam[4 * sprite_number + 1];

			/* If sprite isn't on this line, continue. */
			if(hram_io_ly +
				(gb->hram_io[IO_LCDC] & LCDC_OBJ_SIZE ? 0 : 8) >= OY
					|| hram_io_ly + 16 < OY)
				continue;

#if WALNUT_FULL_GBC_SUPPORT
			if (!cgbMode)
			{
#endif
			struct sprite_data current;

			current.sprite_number = sprite_number;
			current.x = OX;

			uint8_t place;
			for (place = number_of_sprites; place != 0; place--)
			{
				if(compare_sprites(&sprites_to_render[place - 1], &current) < 0)
					break;
			}
			if(place >= MAX_SPRITES_LINE)
				continue;
			for (uint8_t i = number_of_sprites; i > place; --i) {
				sprites_to_render[i] = sprites_to_render[i - 1];
			}
			if(number_of_sprites < MAX_SPRITES_LINE)
				number_of_sprites++;
			sprites_to_render[place] = current;
#if WALNUT_FULL_GBC_SUPPORT
			}
			else
			{
				// CGB does not care about the X coordinate of the sprite when it comes to render priority, only the OAM order.
				// Skip the reordering and just fill sprites_to_render until it is full.
				if (number_of_sprites >= MAX_SPRITES_LINE) continue;
				sprites_to_render[number_of_sprites].sprite_number = sprite_number;
				sprites_to_render[number_of_sprites].x = OX;
				number_of_sprites++;
			}
#endif
		}
#endif

		/* Render each sprite, from low priority to high priority. */
#if WALNUT_GB_HIGH_LCD_ACCURACY
		/* Render the top ten prioritised sprites on this scanline. */
		for(sprite_number = number_of_sprites - 1;
				sprite_number != 0xFF;
				sprite_number--)
		{
			uint8_t s = sprites_to_render[sprite_number].sprite_number;
#else
		for (sprite_number = NUM_SPRITES - 1;
			sprite_number != 0xFF;
			sprite_number--)
		{
			uint8_t s = sprite_number;
#endif
			uint8_t py, t1, t2, dir, start, end, shift, disp_x;
			/* Sprite Y position. */
			uint8_t OY = gb->oam[4 * s + 0];
			/* Sprite X position. */
			uint8_t OX = gb->oam[4 * s + 1];
			/* Sprite Tile/Pattern Number. */
			uint8_t OT = gb->oam[4 * s + 2]
				     & (gb->hram_io[IO_LCDC] & LCDC_OBJ_SIZE ? 0xFE : 0xFF);
			/* Additional attributes. */
			uint8_t OF = gb->oam[4 * s + 3];

#if !WALNUT_GB_HIGH_LCD_ACCURACY
			/* If sprite isn't on this line, continue. */
			if(hram_io_ly +
					(gb->hram_io[IO_LCDC] & LCDC_OBJ_SIZE ? 0 : 8) >= OY ||
					hram_io_ly + 16 < OY)
				continue;
#endif

			/* Continue if sprite not visible. */
			if(OX == 0 || OX >= 168)
				continue;

			// y flip
			py = hram_io_ly - OY + 16;

			if(OF & OBJ_FLIP_Y)
				py = (gb->hram_io[IO_LCDC] & LCDC_OBJ_SIZE ? 15 : 7) - py;

			// fetch the tile
#if WALNUT_FULL_GBC_SUPPORT
			if(cgbMode)
			{
				t1 = gb->vram[((OF & OBJ_BANK) << 10) + VRAM_TILES_1 + OT * 0x10 + 2 * py];
				t2 = gb->vram[((OF & OBJ_BANK) << 10) + VRAM_TILES_1 + OT * 0x10 + 2 * py + 1];
			}
			else
#endif
			{
				t1 = gb->vram[VRAM_TILES_1 + OT * 0x10 + 2 * py];
				t2 = gb->vram[VRAM_TILES_1 + OT * 0x10 + 2 * py + 1];
			}

			// handle x flip
			if(OF & OBJ_FLIP_X)
			{
				dir = 1;
				start = (OX < 8 ? 0 : OX - 8);
				end = MIN(OX, LCD_WIDTH);
				shift = 8 - OX + start;
			}
			else
			{
				dir = (uint8_t)-1;
				start = MIN(OX, LCD_WIDTH) - 1;
				end = (OX < 8 ? 0 : OX - 8) - 1;
				shift = OX - (start + 1);
			}

			// copy tile
			t1 >>= shift;
			t2 >>= shift;

			/* TODO: Put for loop within the to if statements
			 * because the BG priority bit will be the same for
			 * all the pixels in the tile. */
			for(disp_x = start; disp_x != end; disp_x += dir)
			{
				uint8_t c = (t1 & 0x1) | ((t2 & 0x1) << 1);
				// check transparency / sprite overlap / background overlap
#if WALNUT_FULL_GBC_SUPPORT
				if(cgbMode)
				{
					uint8_t isBackgroundDisabled = c && !(gb->hram_io[IO_LCDC] & LCDC_BG_ENABLE);
					uint8_t isPixelPriorityNonConflicting = c &&
															!(pixelsPrio[disp_x] && (pixels[disp_x] & 0x3)) &&
															!((OF & OBJ_PRIORITY) && (pixels[disp_x] & 0x3));

					if(isBackgroundDisabled || isPixelPriorityNonConflicting)
					{
						/* Set pixel colour. */
						pixels[disp_x] = ((OF & OBJ_CGB_PALETTE) << 2) + c + 0x20;  // add 0x20 to differentiate from BG
					}
				}
				else
#endif
				if(c && !(OF & OBJ_PRIORITY && !((pixels[disp_x] & 0x3) == gb->display.bg_palette[0])))
				{
					/* Set pixel colour. */
					pixels[disp_x] = (OF & OBJ_PALETTE)
						? gb->display.sp_palette[c + 4]
						: gb->display.sp_palette[c];
#if WALNUT_GB_12_COLOUR
					/* Set pixel palette (OBJ0 or OBJ1). */
					pixels[disp_x] |= (OF & OBJ_PALETTE);
#endif
#if WALNUT_FULL_GBC_SUPPORT
					/* Deselect BG palette. */
					pixels[disp_x] &= ~LCD_PALETTE_BG;
#endif
				}
				t1 = t1 >> 1;
				t2 = t2 >> 1;
			}
		}
	}
	
	gb->display.lcd_draw_line(gb, pixels, gb->hram_io[IO_LY]);
}

