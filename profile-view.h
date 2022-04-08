#ifndef GL_PROFILE_VIEW_H_INCLUDED
#define GL_PROFILE_VIEW_H_INCLUDED

struct gl_profile_view;
struct gl_frame;

extern int gl_profile_view_create(struct gl_frame **out, int x, int y, int w,
				  int h, int frm_color, int nr_entries,
				  int sample_size, int marker_color,
				  int grid_color);

void gl_profile_view_increment_marker_pos(struct gl_profile_view *pv);

void gl_profile_view_add_sample(struct gl_profile_view *pv,
				int pos, double sample);

void gl_profile_view_set_color(struct gl_profile_view *pv,
			       int pos, int color);

float gl_profile_view_get_scale(const struct gl_profile_view *pv);

void gl_profile_view_scale_reset(struct gl_profile_view *pv);

void gl_profile_view_scale_decrement(struct gl_profile_view *pv);

void gl_profile_view_scale_increment(struct gl_profile_view *pv);

#endif /* GL_PROFILE_VIEW_H_INCLUDED */
