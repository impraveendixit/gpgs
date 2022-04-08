#include <stdlib.h>

#include "data-box.h"
#include "svgalib-private.h"
#include "debug.h"


static void data_box_destroy(struct gl_frame *frm)
{
	struct data_box *dbp = DATA_BOX(frm);
	if (dbp)
		free(dbp);
	dbp = NULL;
}

static void data_box_show(struct gl_frame *frm)
{
	struct data_box *dbp = DATA_BOX(frm);
	int x, y;

	switch (dbp->split) {
	case SPLIT_VERTICAL:
		y = frm->height * dbp->split_ratio;
		svgalib_display_text(frm->xb + 5, frm->yb + 10,
				     dbp->caption, FONT_7x14, 0,
				     svgalib_get_color(20, 20, 20));

		svgalib_draw_frame(frm->xb, frm->yb + y, frm->width,
				   frm->height - y, frm->color);
		svgalib_display_text(frm->xb + 10, frm->yb + y + 3,
				     dbp->text, FONT_SUN12x22,
				     frm->color, dbp->text_color);
		break;

	case SPLIT_HORIZONTAL:
		x = frm->width * dbp->split_ratio;
		svgalib_draw_frame(frm->xb, frm->yb, frm->width,
				   frm->height, frm->color);
		svgalib_display_text(frm->xb + 5, frm->yb + 5,
				     dbp->caption, FONT_7x14, frm->color,
				     svgalib_get_color(0, 31, 0));

		svgalib_display_text(frm->xb + x + 10, frm->yb + 5,
				     dbp->text, FONT_SUN12x22,
				     frm->color, dbp->text_color);
		break;
	default:
		break;
	}
	INFO("%s=%s", dbp->caption, dbp->text);
}

int data_box_create(struct gl_frame **out,
		    int x, int y, int w, int h, int color)
{
	struct data_box *dbp = calloc(1, sizeof(struct data_box));
	if (dbp == NULL) {
		SYSERR("Failed to allocate data box structure.");
		return -1;
	}
	dbp->text_color = svgalib_get_color(31, 31, 31);
	dbp->split = SPLIT_NONE;
	dbp->split_ratio = 0.0;
	gl_frame_init(&dbp->frame, x, y, w, h, color);
	dbp->frame.draw = data_box_show;
	dbp->frame.destroy = data_box_destroy;

	*out = GL_FRAME(dbp);
	return 0;
}

void data_box_set_text(struct data_box *dbp, const char *text, int size)
{
	if (size > 9)
		strcpy(dbp->text, "----");
	else
		strcpy(dbp->text, text);
}

void data_box_set_caption(struct data_box *dbp, const char *caption)
{
	snprintf(dbp->caption, 16, "%-15s", caption);
}
