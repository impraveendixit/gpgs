#ifndef SVGALIB_GRAPHICS_MODE_H_INCLUDED
#define SVGALIB_GRAPHICS_MODE_H_INCLUDED

#include "lib/vgagl.h"
#include "lib/vga.h"
#include "font/font.h"

extern int svgalib_init(int mode);

extern void svgalib_exit(void);

extern void svgalib_show_context(GraphicsContext *gc);

extern void svgalib_clear_screen(int color);

extern void svgalib_copy_box_to_screen(int x, int y, int w, int h);

extern void svgalib_draw_frame(int x, int y, int w, int h, int color);

extern void svgalib_display_text(int xb, int yb, const char txt[],
				 font_t type, int bgcolor, int txtcolor);

extern void svgalib_display_text_wrapped(int xb, int yb, const char txt[],
				 font_t type, int bgcolor, int txtcolor);

extern void svgalib_draw_thick_line(int xb, int yb, int xe, int ye, int color);

extern void svgalib_draw_grid(int x, int y, int w, int h, int step, int color);

extern GraphicsContext *svgalib_virtual_context_create(void);

static inline void svgalib_virtual_context_destroy(GraphicsContext *gc)
{
	gl_freecontext(gc);
}

static inline void svgalib_set_context(GraphicsContext *gc)
{
	gl_setcontext(gc);
}

static inline void svgalib_clear_context(GraphicsContext *gc, int color)
{
	gl_setcontext(gc);
	gl_clearscreen(color);
}

static inline void svgalib_draw_box_colored(int x, int y,
					    int w, int h, int color)
{
	gl_fillbox(x, y, w, h, color);
}

static inline int svgalib_get_color(int r, int g, int b)
{
	return ((b & 0x1F) + ((g & 0x1F) << 6) + ((r & 0x1F) << 11));
}

static inline void svgalib_draw_line(int xb, int yb, int xe, int ye, int color)
{
	gl_line(xb, yb, xe, ye, color);
}

static inline void svgalib_draw_hline(int xb, int yb, int xe, int color)
{
	gl_hline(xb, yb, xe, color);
}

static inline void svgalib_draw_circle(int xc, int yc, int r, int color)
{
	gl_circle(xc, yc, r, color);
}

static inline void svgalib_draw_circle_filled(int xc, int yc, int r, int color)
{
	gl_fillcircle(xc, yc, r, color);
}

static inline void svgalib_set_pixel(int x, int y, int color)
{
	gl_setpixel(x, y, color);
}

#endif	/* SVGALIB_GRAPHICS_MODE_H_INCLUDED */
