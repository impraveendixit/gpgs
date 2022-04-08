#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "map.h"
#include "svgalib.h"
#include "transform.h"
#include "debug.h"
#include "config.h"
#include "flight.h"
#include "course.h"
#include "keyboard.h"
#include "internals.h"
#include "geometry.h"

#define NR_TRAILS	10000

struct map_context {
	struct gl_frame frame;
	const struct course *course_ptr;
	const struct flight_data *flt;
	struct transform transform;
	struct point ref_point;
	struct point trails[NR_TRAILS];
	unsigned int trails_nr;
	unsigned int autozoom : 1;
};


void map_context_adjust_scale(struct gl_frame *frm, map_scale_t scale)
{
	struct map_context *ctx = (struct map_context *)frm;

	if (ctx == NULL)
		return;

	switch (scale) {
	case MAP_SCALE_UP:
		transform_set_scale(&ctx->transform,
				    transform_get_scale(&ctx->transform) * 1.2);
		ctx->autozoom = 0;
		break;
	case MAP_SCALE_DN:
		transform_set_scale(&ctx->transform,
				    transform_get_scale(&ctx->transform) / 1.2);
		ctx->autozoom = 0;
		break;
	case MAP_SCALE_AUTO:
		ctx->autozoom = 1;
		break;
	}
}

static void auto_zoom(struct map_context *ctx, const struct course *cp)
{
	struct gl_frame *frm = &ctx->frame;
	double diag = ((double)frm->height / frm->width) * (turn_radius << 1);
	double zdist = cp->distance_to_go;

	if ((diag > cp->distance_to_go) || cp->on_line)
		zdist = diag;

	zdist = 10 / zdist;
	if (ctx->autozoom)
		transform_set_scale(&ctx->transform,
			transform_get_scale(&ctx->transform) * 0.95 + zdist);
}

static void __plot_line(const struct map_context *ctx,
			const struct line *l, int color)
{
	struct line l_temp, l_clip;
	const struct gl_frame *frm = &ctx->frame;
	struct boundary boundary = {
		.north	= frm->yb + 2,
		.west	= frm->xb + 2,
		.south	= frm->yb + frm->height - 2,
		.east	= frm->xb + frm->width - 2,
	};

	l_temp.bpos = do_transform(&ctx->transform, l->bpos);
	l_temp.epos = do_transform(&ctx->transform, l->epos);

	if (!line_clip(&boundary, &l_temp, &l_clip))
		svgalib_draw_thick_line(l_clip.bpos.x, l_clip.bpos.y,
					l_clip.epos.x, l_clip.epos.y, color);
}

static void plot_curr_line(const struct map_context *ctx,
			   const struct line *l, int color)
{
	struct line l_temp;
	struct point v = make_vector(&l->bpos, &l->epos);
	v = get_unit_vector(&v);

	/* extend unit vector of line to 500m for line entry */
	v.x *= 500.0;
	v.y *= 500.0;

	__plot_line(ctx, l, color);

	/* extend begin point of line */
	color = svgalib_get_color(10, 10, 10);
	l_temp.bpos = l->bpos;
	l_temp.epos.x = l->bpos.x + v.x;
	l_temp.epos.y = l->bpos.y + v.y;
	__plot_line(ctx, &l_temp, color);

	/* extend end point of line */
	l_temp.bpos = l->epos;
	l_temp.epos.x = l->epos.x - v.x;
	l_temp.epos.y = l->epos.y - v.y;
	__plot_line(ctx, &l_temp, color);
}

static void __plot_corner_point(const struct course *cp,
				const void *data, void *userdata)
{
	struct map_context *ctx = (struct map_context *)userdata;
	int color = svgalib_get_color(10, 10, 10);	/* light grey color */
	static struct line l;
	static int init = 1;
	const struct corner_point *curr = (const struct corner_point *)data;

	if (init) {
		l.bpos = curr->pos;
		l.epos = curr->pos;
		init = 0;
	} else {
		l.epos = curr->pos;
		__plot_line(ctx, &l, color);
		l.bpos = l.epos;
	}
}

static inline void plot_corner_points(struct map_context *ctx,
				      const struct course *cp)
{
	course_for_each(cp, COURSE_CORNER_POINT, __plot_corner_point, ctx);
}

static void __plot_flightline(const struct course *cp,
			      const void *data, void *userdata)
{
	struct map_context *ctx = (struct map_context *)userdata;
	const struct flt_path *flt = (const struct flt_path *)data;

	if (course_curr_type(cp) == COURSE_FLT_LINE &&
	    course_curr_entry(cp) == flt) {
		plot_curr_line(ctx, &flt->line, svgalib_get_color(31, 31, 31));
	} else {
		int color = svgalib_get_color(0, 31, 0);
		if (flt->status)
			color = svgalib_get_color(20, 0, 0);
		if (course_next_type(cp) == COURSE_FLT_LINE &&
		    course_next_entry(cp) == flt)
			color = svgalib_get_color(0, 0, 31);

		__plot_line(ctx, &flt->line, color);
	}
}

static void __plot_tieline(const struct course *cp,
			   const void *data, void *userdata)
{
	struct map_context *ctx = (struct map_context *)userdata;
	const struct flt_path *tie = (const struct flt_path *)data;

	if (course_curr_type(cp) == COURSE_TIE_LINE &&
	    course_curr_entry(cp) == tie) {
		plot_curr_line(ctx, &tie->line, svgalib_get_color(31, 31, 31));
	} else {
		int color = svgalib_get_color(0, 31, 0);
		if (tie->status)
		color = svgalib_get_color(20, 0, 0);
		if (course_next_type(cp) == COURSE_TIE_LINE &&
		course_next_entry(cp) == tie)
		color = svgalib_get_color(0, 0, 31);

		__plot_line(ctx, &tie->line, color);
	}
}

static inline void plot_flight_lines(struct map_context *ctx,
				     const struct course *cp)
{
	course_for_each(cp, COURSE_FLT_LINE, __plot_flightline, ctx);
}

static inline void plot_tie_lines(struct map_context *ctx,
				  const struct course *cp)
{
	course_for_each(cp, COURSE_TIE_LINE, __plot_tieline, ctx);
}

static void __plot_waypoint(const struct map_context *ctx,
			    const struct way_point *wp, int color)
{
	char tmp[10] = "";
	icon_t icon;
	const struct gl_frame *frm = &ctx->frame;
	struct boundary boundary = {
		.north	= frm->yb + 2,
		.west	= frm->xb + 2,
		.south	= frm->yb + frm->height - 2,
		.east	= frm->xb + frm->width - 2,
	};
	struct point pos = do_transform(&ctx->transform, wp->pos);

	switch (wp->type) {
	default:
	case WAYPOINT_FLAG:
		icon = ICON_FLAG;
		break;
	case WAYPOINT_HOME:
		icon = ICON_HOME;
		break;
	case WAYPOINT_CAMP:
		icon = ICON_CAMP;
		break;
	case WAYPOINT_BARREL:
		icon = ICON_BARREL;
		break;
	case WAYPOINT_POWERLINE:
		icon = ICON_POWERLINE;
		break;
	}

	/* Limit way point caption */
	strncpy(tmp, wp->caption, 10);
	tmp[9] = '\0';

	/* Limit boundary for icon */
	boundary.north += 14;
	boundary.south -= 14;
	boundary.west += 45;
	boundary.east -= 45;

	pos.y -= 10;
	icon_plot(&boundary, &pos, icon);

	pos.y += 10;
	if (is_point_inside_boundary(&boundary, &pos)) {
		if (color) {
			svgalib_draw_circle(pos.x, pos.y, 12, color);
			svgalib_display_text_wrapped(pos.x, pos.y + 8,
						     tmp, FONT_SUN8x16,
						     frm->color, color);
		} else {
			svgalib_display_text_wrapped(pos.x, pos.y + 8, tmp,
						     FONT_ACORN8x8, frm->color,
						     svgalib_get_color(0, 31, 0));
		}
	}
}

static void plot_waypoint(const struct course *cp,
			  const void *data, void *userdata)
{
	struct map_context *ctx = (struct map_context *)userdata;
	const struct way_point *wp = (const struct way_point *)data;
	int color;

	if (course_next_type(cp) == COURSE_WAY_POINT &&
		course_next_entry(cp) == wp) {
		color = svgalib_get_color(0, 0, 31);
	} else if (course_curr_type(cp) == COURSE_WAY_POINT &&
		course_curr_entry(cp) == wp) {
		color = svgalib_get_color(31, 31, 31);
	} else {
		color = svgalib_get_color(0, 0, 0);
	}
	__plot_waypoint(ctx, wp, color);
}

static inline void plot_way_points(struct map_context *ctx,
				   const struct course *cp)
{
	course_for_each(cp, COURSE_WAY_POINT, plot_waypoint, ctx);
}

static void __plot_trails(const struct boundary *b,
			  const struct point *pos, int color)
{
	if (is_point_inside_boundary(b, pos)) {
		svgalib_set_pixel(pos->x, pos->y, color);
		svgalib_set_pixel(pos->x + 1, pos->y, color);
		svgalib_set_pixel(pos->x - 1, pos->y, color);
		svgalib_set_pixel(pos->x, pos->y + 1, color);
		svgalib_set_pixel(pos->x, pos->y - 1, color);
	}
}

static void plot_aircraft_trails(struct map_context *ctx,
				 const struct point *pos)
{
	register unsigned int i;
	const struct gl_frame *frm = &ctx->frame;
	struct boundary boundary = {
		.north	= frm->yb + 2,
		.west	= frm->xb + 2,
		.south	= frm->yb + frm->height - 2,
		.east	= frm->xb + frm->width - 2,
	};

	/* Aircraft positional history is saved in buffer to generate trails */
	ctx->trails[ctx->trails_nr].x = pos->x;
	ctx->trails[ctx->trails_nr].y = pos->y;
	ctx->trails_nr = (ctx->trails_nr >= NR_TRAILS - 1) ? 0 : ctx->trails_nr + 1;

	/* plot of trailing points */
	for (i = NR_TRAILS - 1; i > 0; i--) {
		struct point p = ctx->trails[(ctx->trails_nr + i) % NR_TRAILS];
		if (p.x == 0.0 && p.y == 0.0)
			break;

		p = do_transform(&ctx->transform, p);
		__plot_trails(&boundary, &p, svgalib_get_color(31, 0, 0));
	}
}

static void show_cross_track_error(const struct map_context *ctx,
				   const struct course *cp)
{
	const struct point *ref = &ctx->ref_point;
	const struct gl_frame *frm = &ctx->frame;
	char str[64] = "";

	switch (cp->cross_track_error_flag) {
	case CROSS_TRACK_ERROR_NONE:
		break;
	case CROSS_TRACK_ERROR_RIGHT:
		if (cp->cross_track_error > 0.9 &&
		    cp->cross_track_error <= 9999.9) {
			snprintf(str, 64, "<< %4.0lf", cp->cross_track_error);
			svgalib_display_text_wrapped(ref->x - 50, ref->y, str,
						     FONT_SUN12x22, frm->color,
						     svgalib_get_color(31, 31, 31));
		}
		break;
	case CROSS_TRACK_ERROR_LEFT:
		if (cp->cross_track_error > 0.9 &&
		    cp->cross_track_error <= 9999.9) {
			snprintf(str, 64, "%-4.0lf >>", cp->cross_track_error);
			svgalib_display_text_wrapped(ref->x + 50, ref->y, str,
						     FONT_SUN12x22, frm->color,
						     svgalib_get_color(31, 31, 31));
		}
		break;
	}
}

static void plot_aircraft_motion(struct map_context *ctx, const struct point *pos)
{
	/* Aircraft icon at reference position in context */
	const struct point *ref = &ctx->ref_point;
	struct gl_frame *frm = &ctx->frame;
	struct boundary boundary = {
		.north	= frm->yb + 2,
		.west	= frm->xb + 2,
		.south	= frm->yb + frm->height - 2,
		.east	= frm->xb + frm->width - 2,
	};
	plot_aircraft_trails(ctx, pos);
	icon_plot(&boundary, ref, ICON_AIRCRAFT);
	//svgalib_draw_line(ref->x, ref->y, ref->x, ref->y - 120,
	//					svgalib_get_color(31, 31, 0));
}

static void plot_dest_point(const struct map_context *ctx, struct point dest)
{
	const struct gl_frame *frm = &ctx->frame;
	struct boundary boundary = {
		.north	= frm->yb + 2,
		.west	= frm->xb + 2,
		.south	= frm->yb + frm->height - 2,
		.east	= frm->xb + frm->width - 2,
	};

	dest = do_transform(&ctx->transform, dest);
	if (is_point_inside_boundary(&boundary, &dest)) {
		int color = svgalib_get_color(31, 31, 31);
		svgalib_draw_circle(dest.x, dest.y, 3, color);
		svgalib_draw_circle(dest.x, dest.y, 2, color);
	}
}

static void map_context_adjust(struct map_context *ctx, struct gl_frame *frm)
{
	ctx->ref_point.x = frm->xb + (frm->width >> 1);
	ctx->ref_point.y = frm->yb + (frm->height >> 2) * 3;
	transform_set_ref_point(&ctx->transform, &ctx->ref_point);
}

static void map_context_destroy(struct gl_frame *frm)
{
	struct map_context *ctx = (struct map_context *)frm;
	if (ctx)
		free(ctx);
	ctx = NULL;
}

static void map_context_draw(struct gl_frame *frm)
{
	struct map_context *ctx = (struct map_context *)frm;
	const struct course *cp = ctx->course_ptr;
	const struct flight_data *flt = ctx->flt;

	transform_set_angle(&ctx->transform, flt->sinfi, flt->cosfi);
	transform_set_curr_position(&ctx->transform, &flt->position);
	if (ctx->autozoom)
		auto_zoom(ctx, cp);

	plot_corner_points(ctx, cp);
	plot_flight_lines(ctx, cp);
	plot_tie_lines(ctx, cp);
	plot_way_points(ctx, cp);
	plot_dest_point(ctx, cp->dest_point);

	/* aircraft movement clutters screen while panning */
	if (!flt->panning) {
		plot_aircraft_motion(ctx, &flt->position);
		if (cp->on_line)
			show_cross_track_error(ctx, cp);
	}
}

int map_context_create(struct gl_frame **out, int x, int y, int w, int h,
		       int color, const struct course *cp,
		       const struct flight_data *flt)
{
	struct map_context *ctx = malloc(sizeof(struct map_context));
	if (ctx == NULL) {
		ERROR("Out of memory.");
		return -1;
	}

	gl_frame_init(&ctx->frame, x, y, w, h, color);
	ctx->frame.draw = map_context_draw;
	ctx->frame.destroy = map_context_destroy;

	transform_init(&ctx->transform, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0);
	memset(ctx->trails, 0, sizeof(struct point));
	memset(&ctx->ref_point, 0, sizeof(struct point));
	ctx->trails_nr = 0;
	ctx->autozoom = 1;
	ctx->course_ptr = cp;
	ctx->flt = flt;
	map_context_adjust(ctx, &ctx->frame);

	*out = GL_FRAME(ctx);
	return 0;
}
