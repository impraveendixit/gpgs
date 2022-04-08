#ifndef SVGALIB_TEXT_MODE_H_INCLUDED
#define SVGALIB_TEXT_MODE_H_INCLUDED

#include <stdlib.h>

#include "font/font.h"
#include "debug.h"

typedef struct {

} GraphicsContext;

static inline int svgalib_init(int mode)
{
	DEBUG("svgalib library initialized in %d mode", mode);
	return 0;
}

static inline void svgalib_exit(void)
{
	DEBUG("svgalib library exited");
}

static inline void svgalib_show_context(GraphicsContext *gc)
{
	DEBUG("Graphics context displayed.");
}

static inline void svgalib_clear_screen(int color)
{
	DEBUG("Graphics context cleared");
}

static inline void svgalib_copy_box_to_screen(int x, int y, int w, int h)
{
	DEBUG("Box of width=%d, height=%d, copied to current context at (%d, %d)",
	      w, h, x, y);
}

static inline void svgalib_draw_frame(int x, int y, int w, int h, int color)
{
	DEBUG("Frame of width=%d, height=%d drawn at (%d, %d) with color=%d",
	      w, h, x, y, color);
}

static inline void svgalib_display_text(int xb, int yb, const char txt[],
					font_t type, int bgcolor, int txtcolor)
{
	DEBUG("%s at (%d, %d) with font=%i", txt, xb, yb, type);
}

static inline void svgalib_display_text_wrapped(int xb, int yb,
						const char txt[],
						font_t type, int bgcolor,
						int txtcolor)
{
	DEBUG("%s at (%d, %d) with font=%i", txt, xb, yb, type);
}

static inline void svgalib_draw_thick_line(int xb, int yb, int xe,
					   int ye, int color)
{
	DEBUG("Thick line of color=%d from (%d, %d) to (%d, %d)",
	      color, xb, yb, xe, ye);
}

static inline void svgalib_draw_grid(int x, int y, int w, int h,
				     int step, int color)
{
	DEBUG("Grid drawn (%d, %d) to (%d, %d)", x, y, x + w, y + h);
}

static inline GraphicsContext *svgalib_virtual_context_create(void)
{
	DEBUG("Virtual context created.");
	return (GraphicsContext *)malloc(sizeof(GraphicsContext));
}

static inline void svgalib_virtual_context_destroy(GraphicsContext *gc)
{
	if (gc)
		free(gc);
	DEBUG("Virtual context destroyed.");
}

static inline void svgalib_set_context(GraphicsContext *gc)
{
	DEBUG("Graphics context set.");
}

static inline void svgalib_clear_context(GraphicsContext *gc, int color)
{
	DEBUG("Graphics context cleared.");
}

static inline void svgalib_draw_box_colored(int x, int y,
					    int w, int h, int color)
{
	DEBUG("Box of width=%d, height=%d drawn at (%d, %d) with color=%d",
	      w, h, x, y, color);
}

static inline int svgalib_get_color(int r, int g, int b)
{
	return ((b & 0x1F) + ((g & 0x1F) << 6) + ((r & 0x1F) << 11));
}

static inline void svgalib_draw_line(int xb, int yb, int xe, int ye, int color)
{
	DEBUG("Line of color=%d, from (%d, %d) to (%d, %d)",
	      color, xb, yb, xe, ye);
}

static inline void svgalib_draw_hline(int xb, int yb, int xe, int color)
{
	DEBUG("Horizontal Line  at height %d of color=%d, from (%d, %d)",
	      yb, color, xb, xe);
}

static inline void svgalib_draw_circle(int xc, int yc, int r, int color)
{
	DEBUG("Circle of radius=%d at (%d, %d)", r, xc, yc);
}

static inline void svgalib_draw_circle_filled(int xc, int yc, int r, int color)
{
	DEBUG("Circle filled with color=%d, of radius=%d at (%d, %d)",
	      color, r, xc, yc);
}

static inline void svgalib_set_pixel(int x, int y, int color)
{
	DEBUG("Pixel at (%d, %d) set with color=%d", x, y, color);
}

#endif	/* SVGALIB_TEXT_MODE_H_INCLUDED */
