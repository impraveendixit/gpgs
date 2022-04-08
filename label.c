#include <stdlib.h>
#include <stdio.h>

#include "label.h"
#include "debug.h"
#include "svgalib-private.h"


static void gl_label_destroy(struct gl_frame *frm)
{
	struct gl_label *label = GL_LABEL(frm);
	if (label)
		free(label);
	label = NULL;
}

static void gl_label_show(struct gl_frame *frm)
{
	struct gl_label *label = GL_LABEL(frm);
	svgalib_display_text(frm->xb + label->border,
			     frm->yb + label->border,
			     label->text, label->font,
			     frm->color, label->color);
}

int gl_label_create(struct gl_frame **out,
		    int x, int y, int w, int h, int color)
{
	struct gl_label *label = malloc(sizeof(struct gl_label));
	if (label == NULL) {
		SYSERR("Failed to allocate for label");
		return -1;
	}
	label->font = FONT_NONE;
	label->color = svgalib_get_color(0, 0, 0);
	label->border = 0;
	label->text = NULL;

	gl_frame_init(&label->frame, x, y, w, h, color);
	label->frame.draw = gl_label_show;
	label->frame.destroy = gl_label_destroy;

	*out = GL_FRAME(label);
	return 0;
}
