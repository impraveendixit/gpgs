#include <stdlib.h>

#include "frame.h"
#include "svgalib-private.h"
#include "debug.h"

void gl_frame_init(struct gl_frame *frm, int x, int y, int w, int h, int color)
{
	frm->xb = x;
	frm->yb = y;
	frm->width = w;
	frm->height = h;
	frm->color = color;
	frm->callback_data = NULL;
	frm->callback = NULL;
}

void gl_frame_destroy(struct gl_frame *frm)
{
	if (frm)
		frm->destroy(frm);
}

void gl_frame_add_callback(struct gl_frame *frm, rc_t rc,
			   gl_frame_callback_t callback, const void *data)
{
	frm->callback_data = data;
	frm->callback = callback;
	frm->rc = rc;
}

void gl_frame_draw(struct gl_frame *frm, unsigned int no_border)
{
	if (frm == NULL)
		return;

	if (frm->callback)
		frm->callback(frm, frm->callback_data);

	if (!no_border)
		svgalib_draw_frame(frm->xb, frm->yb, frm->width,
				   frm->height, frm->color);
	frm->draw(frm);
}
