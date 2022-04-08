#include <stdlib.h>

#include "profile-view.h"
#include "svgalib-private.h"
#include "frame.h"
#include "debug.h"


#define	PROFILE_SCALES_NR		18
#define	PROFILE_DEFAULT_SCALE_INDEX	8
#define	PROFILE_GRID_SIZE		20

static const float profile_scale[PROFILE_SCALES_NR] = {
	2.0e4, 1.0e4, 5.0e3, 2.0e3,
	1.0e3, 5.0e2, 2.0e2, 1.0e2,
	5.0e1, 2.0e1, 1.0e1, 5.0e0,
	2.0e0, 1.0e0, 5.0e-1, 2.0e-1,
	1.0e-1, 5.0e-2
};

struct profile_attribute {
	int scale_index; /* current scale number */
};

struct profile_view_entry {
	double *sample_list;
	int sample_index;
	int color;
};

struct gl_profile_view {
	struct gl_frame frame;
	struct profile_view_entry **entry_list;
	struct profile_attribute attr;
	int nr_entries;
	int sample_size;
	int marker_color;
	int grid_color;
};

#define PROFILE_VIEW(frame) ((struct gl_profile_view *)frame)

static struct profile_view_entry *profile_view_entry_create(int sample_size)
{
	struct profile_view_entry *en = NULL;

	en = calloc(1, sizeof(struct profile_view_entry));
	if (en == NULL) {
		ERROR("Out of memory.");
		goto exit;
	}
	en->sample_list = calloc(sample_size, sizeof(double));
	if (en->sample_list == NULL) {
		ERROR("Out of memory.");
		goto exit_free;
	}
	en->sample_index = -1;
	en->color = svgalib_get_color(10, 10, 10);
	return en;

 exit_free:
	free(en);
 exit:
	return NULL;
}

static void profile_view_entry_destroy(struct profile_view_entry *en)
{
	if (en) {
		free(en->sample_list);
		free(en);
	}
	en = NULL;
}

void gl_profile_view_add_sample(struct gl_profile_view *pv,
				int pos, double sample)
{
	struct profile_view_entry *en = pv->entry_list[pos];
	if (en != NULL) {
		en->sample_index = (en->sample_index + 1) % pv->sample_size;
		en->sample_list[en->sample_index] = sample;
	}
}

void gl_profile_view_set_color(struct gl_profile_view *pv, int pos, int color)
{
	struct profile_view_entry *en = pv->entry_list[pos];
	if (en != NULL)
		en->color = color;
}

static void profile_view_destroy(struct gl_frame *frm)
{
	struct gl_profile_view *pv = PROFILE_VIEW(frm);
	if (pv) {
		register int i;

		for (i = 0; i < pv->nr_entries; i++) {
			struct profile_view_entry *en = pv->entry_list[i];
			if (en == NULL)
				continue;
			profile_view_entry_destroy(en);
		}
		free(pv->entry_list);
		free(pv);
	}
	pv = NULL;
}

static void profile_view_draw(struct gl_frame *frm)
{
	struct gl_profile_view *pv = PROFILE_VIEW(frm);
	register int i, j, k;
	int e, s;
	float scale = gl_profile_view_get_scale(pv);
	char txt[64] = "";

	e = frm->xb + frm->width;
	s = frm->yb + frm->height;

	/* Border */
	svgalib_draw_grid(frm->xb, frm->yb, frm->width, frm->height,
			  PROFILE_GRID_SIZE, pv->grid_color);

	for (k = 0; k < pv->nr_entries; k++) {
		struct profile_view_entry *en = pv->entry_list[k];
		if (en == NULL)
			continue;

		/* Draw all samples of each entry */
		for (i = frm->xb + 2, j = 1;
		     (j < pv->sample_size) && (i < e); i++, j++) {
			double y1 = en->sample_list[j - 1] / scale;
			double y2 = en->sample_list[j] / scale;
			int y3 = ((int)y1) % frm->height;
			int y4 = ((int)y2) % frm->height;
			y1 = y1 / frm->height;
			y2 = y2 / frm->height;

			if (y1 < frm->yb || y1 > s)
				y1 = s - y3;
			else
				y1 = s - y1;

			if (y2 < frm->yb || y2 > s)
				y2 = s - y4;
			else
				y2 = s - y2;

			svgalib_draw_line(i - 1, y1, i, y2, en->color);

			if (j == en->sample_index)
				svgalib_draw_thick_line(i, frm->yb, i,
						  s, pv->marker_color);
		}
	}
	snprintf(txt, 64, "%.1f pT/div", scale * PROFILE_GRID_SIZE);
	svgalib_display_text(frm->xb + 5, frm->yb + 5, txt, FONT_7x14,
				frm->color, svgalib_get_color(0, 31, 0));
}

int gl_profile_view_create(struct gl_frame **out, int x, int y, int w, int h,
			   int frm_color, int nr_entries, int sample_size,
			   int marker_color, int grid_color)
{
	struct gl_profile_view *pv = NULL;
	register int i;

	pv = malloc(sizeof(struct gl_profile_view));
	if (pv == NULL) {
		ERROR("Out of memory");
		goto exit;
	}

	pv->entry_list = calloc(nr_entries, sizeof(struct profile_view_entry *));
	if (pv->entry_list == NULL) {
		ERROR("Out of memory");
		goto exit_free;
	}

	pv->sample_size = sample_size;
	for (i = 0; i < nr_entries; i++) {
		pv->entry_list[i] = profile_view_entry_create(pv->sample_size);
		if (pv->entry_list[i] == NULL)
			DEBUG("profile_view_entry_create() failed");
	}

	pv->attr.scale_index = PROFILE_DEFAULT_SCALE_INDEX;
	pv->nr_entries = nr_entries;
	pv->marker_color = marker_color;
	pv->grid_color = grid_color;
	gl_frame_init(&pv->frame, x, y, w, h, frm_color);
	pv->frame.draw = profile_view_draw;
	pv->frame.destroy = profile_view_destroy;
	*out = GL_FRAME(pv);
	return 0;

 exit_free:
	free(pv);
 exit:
	return -1;
}

void gl_profile_view_scale_decrement(struct gl_profile_view *pv)
{
	if (pv->attr.scale_index < PROFILE_SCALES_NR - 1)
		pv->attr.scale_index++;
}

void gl_profile_view_scale_increment(struct gl_profile_view *pv)
{
	if (pv->attr.scale_index > 0)
		pv->attr.scale_index--;
}

void gl_profile_view_scale_reset(struct gl_profile_view *pv)
{
	pv->attr.scale_index = PROFILE_DEFAULT_SCALE_INDEX;
}

float gl_profile_view_get_scale(const struct gl_profile_view *pv)
{
	return profile_scale[pv->attr.scale_index];
}
