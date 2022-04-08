#include <stdlib.h>
#include <string.h>

#include "scale-bar.h"
#include "svgalib-private.h"
#include "debug.h"


static void gl_scale_bar_adjust_text_positions(struct gl_scale_bar *sbar,
                                                struct gl_frame *frm)
{
	switch (sbar->pointer) {
	case GL_SCALE_POINTER_LEFT:
		sbar->txt_xb = frm->xb + (frm->width >> 2);
		sbar->txt_yb = frm->yb + (frm->height >> 1) - (frm->width >> 3);
		break;
	case GL_SCALE_POINTER_RIGHT:
		sbar->txt_xb = frm->xb + (frm->width >> 3);
		sbar->txt_yb = frm->yb + (frm->height >> 1) - (frm->width >> 4);
		break;
	case GL_SCALE_POINTER_TOP:
		sbar->txt_xb = frm->xb + (frm->width >> 1) - (frm->height >> 2);
		sbar->txt_yb = frm->yb + (frm->height >> 1);
		break;
	case GL_SCALE_POINTER_BOTTOM:
		sbar->txt_xb = frm->xb + (frm->width >> 1) - (frm->height >> 2);
		sbar->txt_yb = frm->yb + (frm->height >> 1) - (frm->height >> 2);
		break;
	default:
		break;
	}
}

static int gl_scale_bar_color_callback_default(int curr, int mid,
					       const void *data)
{
	int scaled = (mid - curr) / 5;
	int color = svgalib_get_color(5, 5, 5);

	if (scaled % 10 == 0)
		color = svgalib_get_color(10, 10, 10);

	return color;
}

static void gl_scale_bar_draw(struct gl_frame *frm)
{
	struct gl_scale_bar *sbar = GL_SCALE_BAR(frm);
	int color, start, mid, end, pointer_width, i;

	if (sbar->pointer == GL_SCALE_POINTER_LEFT ||
	    sbar->pointer == GL_SCALE_POINTER_RIGHT) {
		start = frm->yb;
		mid = frm->yb + (frm->height >> 1);
		end = frm->yb + frm->height;
		pointer_width = frm->width >> 1;
	} else {
		start = frm->xb;
		mid = frm->xb + (frm->width >> 1);
		end = frm->xb + frm->width;
		pointer_width = frm->height >> 1;
	}

	for (i = start; i < end; ++i) {
		int width, abs_width, pointer_region;

		abs_width = abs(mid - i);
		if (abs_width <= pointer_width) {
			width = abs_width;
			pointer_region = 1;
		} else {
			width = pointer_width;
			pointer_region = 0;
		}

		color = sbar->callback(i, mid, sbar->callback_data);

		switch (sbar->pointer) {
		case GL_SCALE_POINTER_LEFT:
			if (pointer_region)
				svgalib_draw_hline(frm->xb, i,
						   frm->xb + frm->width,
						   sbar->pointer_color);
			svgalib_draw_hline(frm->xb, i, frm->xb + width, color);
			break;
		case GL_SCALE_POINTER_RIGHT:
			if (pointer_region)
				svgalib_draw_hline(frm->xb, i,
						   frm->xb + frm->width,
						   sbar->pointer_color);
			svgalib_draw_hline(frm->xb + frm->width - width, i,
					   frm->xb + frm->width, color);
			break;
		case GL_SCALE_POINTER_TOP:
			if (pointer_region)
				svgalib_draw_line(i, frm->yb, i,
						  frm->yb + frm->height,
						  sbar->pointer_color);
			svgalib_draw_line(i, frm->yb, i, frm->yb + width, color);
			break;
		case GL_SCALE_POINTER_BOTTOM:
			if (pointer_region)
				svgalib_draw_line(i, frm->yb, i,
						  frm->yb + frm->height,
						  sbar->pointer_color);
			svgalib_draw_line(i, frm->yb + frm->height - width,
					  i, frm->yb + frm->height, color);
			break;
		default:
			break;
		}
	}

	svgalib_display_text(sbar->txt_xb, sbar->txt_yb, sbar->pointer_text,
			     FONT_10x18, sbar->pointer_color, sbar->txt_color);
}

static void gl_scale_bar_destroy(struct gl_frame *frm)
{
	struct gl_scale_bar *sbar = GL_SCALE_BAR(frm);
	if (sbar)
		free(sbar);
	sbar = NULL;
}

int gl_scale_bar_create(struct gl_frame **out,
                        int x, int y, int w, int h, int color)
{
	struct gl_scale_bar *sbar = malloc(sizeof(struct gl_scale_bar));
	if (sbar == NULL) {
		SYSERR("Out of memory");
		return -1;
	}

	gl_frame_init(&sbar->frame, x, y, w, h, color);
	sbar->frame.draw = gl_scale_bar_draw;
	sbar->frame.destroy = gl_scale_bar_destroy;

	sbar->pointer = GL_SCALE_POINTER_NONE;
	sbar->pointer_color = svgalib_get_color(0, 0, 0);
	sbar->txt_color = svgalib_get_color(31, 31, 31);
	strcpy(sbar->pointer_text, "----");
	sbar->callback = gl_scale_bar_color_callback_default;
	sbar->callback_data = NULL;
	gl_scale_bar_adjust_text_positions(sbar, &sbar->frame);

	*out = GL_FRAME(sbar);
	return 0;
}

void gl_scale_bar_set_text(struct gl_scale_bar *sbar, const char *txt, int size)
{
	if (size > 4)
		strcpy(sbar->pointer_text, "----");
	else
		strcpy(sbar->pointer_text, txt);
}

void gl_scale_bar_set_pointer(struct gl_scale_bar *sbar,
			      gl_scale_pointer_t pointer)
{
	sbar->pointer = pointer;
	gl_scale_bar_adjust_text_positions(sbar, &sbar->frame);
}

void gl_scale_bar_set_color_callback(struct gl_scale_bar *sbar,
				     gl_scale_bar_color_callback_t callback,
				     const void *userdata)
{
	sbar->callback = callback;
	sbar->callback_data = userdata;
}
