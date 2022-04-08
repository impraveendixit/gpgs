#include <stdlib.h>

#include "clock.h"
#include "svgalib-private.h"
#include "debug.h"

static void gl_clock_adjust_positions(struct gl_clock *clk,
				      const struct gl_frame *frm)
{
	int r = (frm->width >= frm->height) ? frm->height : frm->width;

	clk->xb = frm->xb + (frm->width >> 1);
	clk->yb = frm->yb + (frm->height >> 1);
	clk->radius = (r >> 1) - 2;
}

static void gl_clock_draw(struct gl_frame *frm)
{
	struct gl_clock *clk = GL_CLOCK(frm);

	svgalib_draw_circle_filled(clk->xb, clk->yb,
				   clk->radius, clk->bg_color);
	svgalib_display_text(clk->xb - 24, clk->yb - 8, clk->text, FONT_10x18,
			     clk->bg_color, svgalib_get_color(31, 31, 31));
}

static void gl_clock_destroy(struct gl_frame *frm)
{
	struct gl_clock *clk = GL_CLOCK(frm);
	if (clk)
		free(clk);
	clk = NULL;
}

int gl_clock_create(struct gl_frame **out,
                int x, int y, int w, int h, int color)
{
	struct gl_clock *clk = malloc(sizeof(struct gl_clock));
	if (clk == NULL) {
		SYSERR("Failed to allocate clock structure.");
		return -1;
	}

	gl_frame_init(&clk->frame, x, y, w, h, color);
	clk->frame.draw = gl_clock_draw;
	clk->frame.destroy = gl_clock_destroy;

	gl_clock_adjust_positions(clk, &clk->frame);
	clk->bg_color = svgalib_get_color(0, 0, 0);

	*out = GL_FRAME(clk);
	return 0;
}

void gl_clock_set_text(struct gl_clock *clk, const char *text, int size)
{
	if (size > 6)
		strcpy(clk->text, "---");
	else
		strcpy(clk->text, text);
}
