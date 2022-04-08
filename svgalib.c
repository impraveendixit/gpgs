#include <stdlib.h>

#include "svgalib-private.h"
#include "debug.h"

static GraphicsContext *physical_context = NULL;
static int vgamode;

int svgalib_init(int mode)
{
	if (vga_init() != 0) {
		DEBUG("vga_init() failed.");
		goto exit_restore;
	}

	if (!mode)
		vgamode = vga_getdefaultmode();
	else
		vgamode = mode;

	/* If mode not supported, terminate the program */
	if (!vga_hasmode(vgamode)) {
		DEBUG("Graphics Mode not supported.");
		goto exit_restore;
	}

	/* Set given mode in the physical screen */
	if (vga_setmode(vgamode) != 0) {
		DEBUG("Failed to create set given mode.");
		goto exit_restore;
	}

	/* Create a physical context for the given mode in memory */
	if (gl_setcontextvga(vgamode) != 0) {
		DEBUG("Failed to set given mode physical context.");
		goto exit_restore;
	}

	/* Allocate space to hold physical context */
	physical_context = gl_allocatecontext();
	if (physical_context == NULL) {
		DEBUG("Failed to allocate physical context.");
		goto exit_restore;
	}
	gl_getcontext(physical_context);
	gl_clearscreen(0);

	/* Setting default font */
	gl_setfont(8, 8, gl_font8x8);
	gl_setwritemode(FONT_COMPRESSED + WRITEMODE_OVERWRITE);
	gl_setfontcolors(0, vga_white());
	return 0;

 exit_restore:
	vga_setmode(TEXT);
	return -1;
}

void svgalib_exit(void)
{
	vga_setmode(TEXT);
	if (physical_context)
		gl_freecontext(physical_context);
}

GraphicsContext *svgalib_virtual_context_create(void)
{
	GraphicsContext saved, *vc = NULL;

	/* copy old context */
	gl_getcontext(&saved);

	if (gl_setcontextvgavirtual(vgamode) != 0) {
		DEBUG("gl_setcontextvgavirtual() failed.");
		goto exit_restore;
	}
	vc = gl_allocatecontext();
	if (vc == NULL) {
		DEBUG("gl_allocatecontext() failed.");
		goto exit_restore;
	}
	gl_getcontext(vc);

 exit_restore:
	gl_setcontext(&saved);
	return vc;
}

void svgalib_show_context(GraphicsContext *gc)
{
	gl_setcontext(gc);
	gl_copyscreen(physical_context);
}

void svgalib_clear_screen(int color)
{
	gl_setcontext(physical_context);
	gl_clearscreen(color);
}

void svgalib_copy_box_to_screen(int x, int y, int w, int h)
{
	gl_copyboxtocontext(x, y, w, h, physical_context, x, y);
}

void svgalib_draw_frame(int x, int y, int w, int h, int color)
{
	int e = x + w;
	int s = y + h;

	gl_fillbox(x, y, w, h, color);

	color = svgalib_get_color(20, 20, 20);
	gl_hline(x, y, e, color);
	gl_hline(x + 1, y + 1, e - 1, color);
	gl_line(x, y, x, s, color);
	gl_line(x + 1, y + 1, x + 1, s - 1, color);

	color = svgalib_get_color(10, 10, 10);
	gl_hline(x, s, e, color);
	gl_hline(x + 1, s - 1, e - 1, color);
	gl_line(e, y, e, s, color);
	gl_line(e - 1, y + 1, e - 1, s - 1, color);
}

void svgalib_display_text(int xb, int yb, const char txt[],
			  font_t type, int bgcolor, int txtcolor)
{
	const struct font *f = get_font(type);
	if (f != NULL) {
		gl_setfont(font_width(f), font_height(f), font_data(f));
		gl_setwritemode(FONT_COMPRESSED + WRITEMODE_OVERWRITE);
	}
	gl_setfontcolors(bgcolor, txtcolor);
	gl_write(xb, yb, (char *)txt);
}

void svgalib_display_text_wrapped(int xb, int yb, const char txt[],
				  font_t type, int bgcolor, int txtcolor)
{
	register int i;
	int len = strlen(txt);
	const struct font *f = get_font(type);

	if (f != NULL) {
		gl_setfont(font_width(f), font_height(f), font_data(f));
		gl_setwritemode(FONT_COMPRESSED + WRITEMODE_MASKED);
	}
	gl_setfontcolors(bgcolor, txtcolor);
	for (i = 0; i < len; ++i)
		gl_printf(xb - (len << 2) + (i << 3), yb - 8, "%c", txt[i]);
}

void svgalib_draw_thick_line(int xb, int yb, int xe, int ye, int color)
{
	gl_line(xb, yb, xe, ye, color);
	if (abs(xe - xb) <= abs(ye - yb)) {
		gl_line(xb - 1, yb, xe - 1, ye, color);
		gl_line(xb + 1, yb, xe + 1, ye, color);
	} else {
		gl_line(xb, yb - 1, xe, ye - 1, color);
		gl_line(xb, yb + 1, xe, ye + 1, color);
	}
}

void svgalib_draw_grid(int x, int y, int w, int h, int step, int color)
{
	register int i;

	if (!step) {
		DEBUG("Invalid grid step size");
		return;
	}

	svgalib_draw_frame(x, y, w, h, svgalib_get_color(0, 0, 0));

	for (i = x; i <= (x + w); i++) {
		if (!((i - x) % step))
			gl_line(i, y, i, y + h, color);
	}

	for (i = y; i <= (y + h); i++) {
		if (!((i - y) % step))
			gl_line(x, i, x + w, i, color);
	}
}
