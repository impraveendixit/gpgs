#include <stdlib.h>

#include "compass.h"
#include "svgalib-private.h"
#include "debug.h"


static void adjust_center(struct gl_compass *comp, struct gl_frame *frm)
{
	int r = (frm->width >= frm->height) ? frm->height : frm->width;

	comp->center_x = frm->xb + (frm->width >> 1);
	comp->center_y = frm->yb + (frm->height >> 1);
	comp->radius = (r >> 1) - 5;
}

static void gl_compass_destroy(struct gl_frame *frm)
{
	struct gl_compass *comp = GL_COMPASS(frm);
	if (comp)
		free(comp);
	comp = NULL;
}

static void draw_arrow(int width, int pgn[], int size, int color)
{
	register int i;
	int ix = 0, xx = 0, xy = 0;
	int in = 0, nx = width, ny = width;
	int dx = 0, dy = 0;

	for (i = 1; i < size; i += 2) {
		if (pgn[i] > xy) {
			xy = pgn[i];
			xx = pgn[i - 1];
			ix = i;
		}

		if (pgn[i] < ny) {
			ny = pgn[i];
			nx = pgn[i - 1];
			in = i;
		}
	}

	for (i = 1; i < size; i += 2) {
		if (i != ix && i != in) {
			dy = pgn[i];
			dx = pgn[i - 1];
		}
	}

	/* upper half of compass polygon */
	for (i = ny; i < dy; ++i)
		svgalib_draw_line(nx * (xy - i) / (xy - ny) + xx * (i - ny) / (xy - ny),
				  i,
				  (nx * (dy - i) / (dy - ny) + dx * (i - ny) / (dy - ny)),
				  i,
				  color);

	/* lower half of compass polygon */
	for (i = dy; i < xy; ++i)
		svgalib_draw_line(nx * (xy - i) / (xy - ny) + xx * (i - ny) / (xy - ny),
				  i,
				  (dx * (xy - i) / (xy - dy) + xx * (i - dy) / (xy - dy)),
				  i,
				  color);
}

static void gl_compass_draw(struct gl_frame *frm)
{
	struct gl_compass *comp = GL_COMPASS(frm);
	int color;
	int xc, yc;
	int pgn[6];

	svgalib_draw_circle_filled(comp->center_x, comp->center_y,
				   comp->radius, comp->bg_color);

	/**
	 * Interior angle at upper and lower vertex of compass..
	 * Governs width of the compass.
	 */
	xc = comp->cosfi * (comp->radius >> 2);
	yc = comp->sinfi * (comp->radius >> 2);

	pgn[0] = comp->center_x + xc;
	pgn[1] = comp->center_y + yc;
	pgn[4] = comp->center_x - xc;
	pgn[5] = comp->center_y - yc;

	/**
	 * Other angles of upper and lower triangle..
	 * Govern length of compass needle ..
	 */
	xc = comp->cosfi * (comp->radius - 2);
	yc = comp->sinfi * (comp->radius - 2);

	/* top half of compass pointer */
	color = svgalib_get_color(15, 8, 0);
	pgn[2] = comp->center_x + yc;
	pgn[3] = comp->center_y - xc;
	draw_arrow(2 * comp->radius, pgn, 6, color);

	/* bottom half of compass pointer */
	color = svgalib_get_color(0, 0, 15);
	pgn[2] = comp->center_x - yc;
	pgn[3] = comp->center_y + xc;
	draw_arrow(2 * comp->radius, pgn, 6, color);
}

int gl_compass_create(struct gl_frame **out,
                    int x, int y, int w, int h, int color)
{
	struct gl_compass *comp = malloc(sizeof(struct gl_compass));
	if (comp == NULL) {
		SYSERR("Failed to allocate compass structure.");
		return -1;
	}

	gl_frame_init(&comp->frame, x, y, w, h, color);
	comp->frame.draw = gl_compass_draw;
	comp->frame.destroy = gl_compass_destroy;

	adjust_center(comp, &comp->frame);
	comp->bg_color = svgalib_get_color(0, 0, 0);
	comp->sinfi = 0;
	comp->cosfi = 1;

	*out = GL_FRAME(comp);
	return 0;
}
