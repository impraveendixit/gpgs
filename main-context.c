#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "main-context.h"
#include "svgalib.h"
#include "config.h"
#include "gps.h"
#include "log.h"
#include "mag.h"
#include "debug.h"
#include "course.h"
#include "flight.h"
#include "keyboard.h"
#include "simulant.h"


typedef enum main_frame_view_t {
	VIEW_MAG_CONTEXT,
	VIEW_GPS_CONTEXT,
	VIEW_FILE_CONTEXT,
	VIEW_MAP_CONTEXT
} main_frame_view_t;

struct graphics_context {
	GraphicsContext *context;
	struct gl_frame *scale_bar_altitude;
	struct gl_frame *scale_bar_tracking;
	struct gl_frame *compass;
	struct gl_frame *clock;
	struct gl_frame *map_area;
	struct gl_frame *data_box_curr_target;
	struct gl_frame *data_box_next_target;
	struct gl_frame *data_box_heading;
	struct gl_frame *data_box_GS;
	struct gl_frame *data_box_DTG;
	struct gl_frame *data_box_gpsfix;
	struct gl_frame *data_box_gpsalt;
	struct gl_frame *data_box_gpslat;
	struct gl_frame *data_box_gpslon;
	struct gl_frame *data_box_mag;
	struct gl_frame *data_box_space;
	struct gl_frame *label_datum;
	struct gl_frame *gps_context;
	struct gl_frame *file_list;
	struct gl_frame *profile_frame;
	main_frame_view_t main_view;
	unsigned int mag_disable;
};

static const char *cmd_list[] = {
	"UNLOGALL THISPORT",
	"SBASCONTROL DISABLE",
	"ASSIGNOMNI USER 1539932500 1200",
	"RTKSOURCE OMNISTAR",
	"PSRDIFFSOURCE OMNISTAR",
	"LOG GPGGA ONTIME 0.2",
	NULL
};

static void gps_context_callback(struct gl_frame *frm, const void *data)
{
	static const char **pcmd = cmd_list;
	const struct gps_data *gps = (const struct gps_data *)data;

	gps_context_add_entry(frm, gps->nmea_string,
			      svgalib_get_color(0, 20, 0));
	if (*pcmd != NULL) {
		char buff[128] = "";

		snprintf(buff, 128, "%s\r\n", *pcmd);
		gps_device_write(buff, strlen(buff));
		gps_context_add_entry(frm, *pcmd, svgalib_get_color(15, 15, 0));
		if (*++pcmd == NULL)
		gps_context_change_header(frm, "GPS Configured :-) ",
					  svgalib_get_color(0, 31, 0));
	}
}

static int scale_bar_color_callback_AGL_altitude(int curr, int mid,
						 const void *data)
{
	int scaled = (mid - curr) / 5 + *(const double *)data;
	int color = svgalib_get_color(5, 5, 5);

	if ((scaled > survey_height_AGL && curr > mid) ||
	    (scaled <= survey_height_AGL && curr < mid))
		color = svgalib_get_color(0, 20, 0);

	if (abs(scaled - survey_height_AGL) <= 10)
		color = svgalib_get_color(0, 31, 0);

	if (scaled >= 0 && scaled <= warning_height_AGL)
		color = svgalib_get_color(31, 0, 0);

	if (scaled % 10 == 0)
		color = svgalib_get_color(10, 10, 10);

	return color;
}

static int scale_bar_color_callback_MSL_altitude(int curr, int mid,
						 const void *data)
{
	int scaled = (mid - curr) / 5 + *(const double *)data;
	int color = svgalib_get_color(5, 5, 5);

	if ((scaled > HAC_height_MSL && curr > mid) ||
	    (scaled <= HAC_height_MSL && curr < mid))
		color = svgalib_get_color(0, 20, 0);

	if (abs(scaled - HAC_height_MSL) <= 10)
		color = svgalib_get_color(0, 31, 0);

	if (scaled % 10 == 0)
		color = svgalib_get_color(10, 10, 10);

	return color;
}

static int scale_bar_color_callback_tracking(int curr, int mid,
					     const void *data)
{
	int scaled = (curr - mid) / 5 - (*(const double *)data) + 360;
	int color = svgalib_get_color(5, 5, 5);

	if ((scaled > 360 && curr < mid) || (scaled <= 360 && curr > mid))
		color = svgalib_get_color(0, 20, 0);

	if (abs(scaled - 360) <= 10)
		color = svgalib_get_color(0, 31, 0);

	if (scaled % 10 == 0)
		color = svgalib_get_color(10, 10, 10);

	return color;
}

static void scale_bar_tracking_callback(struct gl_frame *frm, const void *data)
{
	const struct course *cp = (const struct course *)data;
	struct gl_scale_bar *sbar = GL_SCALE_BAR(frm);
	static double tracking;
	char buff[8] = "";

	/* Reset tracking value when no map loaded */
	if (cp->map_loaded)
		tracking = 0.8 * tracking + 0.2 * cp->tracking;
	else
		tracking = 0.0;

	sprintf(buff, "%-+.0lf", tracking);
	gl_scale_bar_set_text(sbar, buff, strlen(buff));
	INFO("Tracking: %s", buff);
}

static void scale_bar_altitude_callback(struct gl_frame *frm, const void *data)
{
	const struct flight_data *flt = (const struct flight_data *)data;
	struct gl_scale_bar *sbar = GL_SCALE_BAR(frm);
	char buff[8] = "";

	snprintf(buff, 8, "%.0lf", flt->altitude);
	gl_scale_bar_set_text(sbar, buff, strlen(buff));
	INFO("Altitude: %s", buff);

	if (flt->at_AGL_height) {
		gl_scale_bar_set_pointer_color(sbar,
					       svgalib_get_color(0, 10, 0));
		gl_scale_bar_set_color_callback(sbar,
			scale_bar_color_callback_AGL_altitude, &flt->altitude);

	} else {
		gl_scale_bar_set_pointer_color(sbar,
					       svgalib_get_color(5, 15, 15));
		gl_scale_bar_set_color_callback(sbar,
			scale_bar_color_callback_MSL_altitude, &flt->altitude);
	}
}

static void profile_view_callback(struct gl_frame *frm, const void *data)
{
	const struct mag_data *mag = (const struct mag_data *)data;
	struct gl_profile_view *pv = (struct gl_profile_view *)frm;

	/* Convert into unit into picoTesla for clarity */
	gl_profile_view_add_sample(pv, 0, mag->field_value * 1000);
}

static void compass_callback(struct gl_frame *frm, const void *data)
{
	const struct flight_data *flt = (const struct flight_data *)data;
	struct gl_compass *comp = GL_COMPASS(frm);

	gl_compass_set_orientation(comp, flt->sinfi, flt->cosfi);
}

static void clock_callback(struct gl_frame *frm, const void *data)
{
	struct gl_clock *clk = GL_CLOCK(frm);
	const struct gps_data *gps = (const struct gps_data *)data;
	int tm = (int)gps->gga.utc_time;
	char buff[32] = "";

	snprintf(buff, 32, "%02d:%02d", tm / 10000, (tm / 100) % 100);
	gl_clock_set_text(clk, buff, strlen(buff));
	INFO("Time: %s", buff);
}

static void label_datum_callback(struct gl_frame *frm, const void *data)
{
	struct gl_label *label = GL_LABEL(frm);
	const struct flight_data *flt = (const struct flight_data *)data;

	if (flt->at_AGL_height) {
		gl_frame_set_color(frm, svgalib_get_color(0, 10, 0));
		gl_label_set_text(label, "AGL");
		INFO("Datum: AGL");
	} else {
		gl_frame_set_color(frm, svgalib_get_color(5, 15, 15));
		gl_label_set_text(label, "MSL");
		INFO("Datum: MSL");
	}
}

static void data_box_heading_callback(struct gl_frame *frm, const void *data)
{
	struct data_box *dbox = DATA_BOX(frm);
	double heading = *(const double *)data;
	char buff[16] = "";

	snprintf(buff, 16, "%-4.0lf", heading);
	data_box_set_text(dbox, buff, strlen(buff));
}

static void data_box_DTG_callback(struct gl_frame *frm, const void *data)
{
	struct data_box *dbox = DATA_BOX(frm);
	double dtg = *(const double *)data;
	char buff[32] = "";

	dtg /= 1000.0;
	if (dtg > 9999)
		snprintf(buff, 32, "%-8s", "----");
	else
		snprintf(buff, 32, "%-8.3lf", dtg);
	data_box_set_text(dbox, buff, strlen(buff));
}

static void data_box_GS_callback(struct gl_frame *frm, const void *data)
{
	struct data_box *dbox = DATA_BOX(frm);
	double speed = *(const double *)data;
	char buff[32] = "";

	speed = 3.6 * speed;
	if (speed > 9999)
		snprintf(buff, 32, "%-7s", "----");
	else
		snprintf(buff, 32, "%-7.2lf", speed);
	data_box_set_text(dbox, buff, strlen(buff));
}

static void data_box_gpslat_callback(struct gl_frame *frm, const void *data)
{
	struct data_box *dbox = DATA_BOX(frm);
	const struct gps_data *gps = (const struct gps_data *)data;
	char buff[32] = "";

	snprintf(buff, 32, "%-8.5lf%c",
		 gps->gga.latitude, gps->gga.latitude_hemisphere);
	data_box_set_text(dbox, buff, strlen(buff));
}

static void data_box_gpslon_callback(struct gl_frame *frm, const void *data)
{
	struct data_box *dbox = DATA_BOX(frm);
	const struct gps_data *gps = (const struct gps_data *)data;
	char buff[32] = "";

	snprintf(buff, 32, "%-8.5lf%c",
		 gps->gga.longitude, gps->gga.longitude_hemisphere);
	data_box_set_text(dbox, buff, strlen(buff));
}

static void data_box_mag_callback(struct gl_frame *frm, const void *data)
{
	struct data_box *dbox = DATA_BOX(frm);
	const struct mag_data *mag = (const struct mag_data *)data;
	char buff[32] = "";

	snprintf(buff, 32, "%-9.3lf", mag->field_value);
	data_box_set_text(dbox, buff, strlen(buff));
}

static void data_box_disk_space_callback(struct gl_frame *frm, const void *data)
{
	struct data_box *dbox = DATA_BOX(frm);
	char buff[32] = "";

	snprintf(buff, 32, "%-6.2f%%", fetch_disk_space(log_directory));
	data_box_set_text(dbox, buff, strlen(buff));
}

static void data_box_gpsalt_callback(struct gl_frame *frm, const void *data)
{
	struct data_box *dbox = DATA_BOX(frm);
	const struct gps_data *gps = (const struct gps_data *)data;
	char buff[32] = "";

	snprintf(buff, 32, "%-7.2f%c",
		 gps->gga.altitude, tolower(gps->gga.alt_unit));
	data_box_set_text(dbox, buff, strlen(buff));
}

static void data_box_gpsfix_callback(struct gl_frame *frm, const void *data)
{
	struct data_box *dbox = DATA_BOX(frm);
	const struct gps_data *gps = (const struct gps_data *)data;
	char buff[32] = "";

	snprintf(buff, 32, "%02d/%02d", gps->gga.fix, gps->gga.nsat);
	data_box_set_text(dbox, buff, strlen(buff));
}

static void display_course(course_t type, const void *entry,
			   char *buf, int size, int *color)
{
	const struct flt_path *f = NULL;
	const struct way_point *wp = NULL;

	*color = svgalib_get_color(0, 10, 0);

	switch(type) {
	case COURSE_FLT_LINE:
		f = (const struct flt_path *)entry;
		snprintf(buf, size, "FL %d", f->id);
		if (f->status)
			*color = svgalib_get_color(10, 0, 0);
		break;
	case COURSE_TIE_LINE:
		f = (const struct flt_path *)entry;
		snprintf(buf, size, "TL %d", f->id);
		if (f->status)
			*color = svgalib_get_color(10, 0, 0);
		break;
	case COURSE_WAY_POINT:
		wp = (const struct way_point *)entry;
		snprintf(buf, size, "%s", wp->caption);
		break;
	default:
		snprintf(buf, size, "N/A");
		*color = svgalib_get_color(5, 15, 15);
		break;
	}
}

static void data_box_curr_target_callback(struct gl_frame *frm,
					  const void *data)
{
	struct data_box *dbox = DATA_BOX(frm);
	const struct course *cp = (const struct course *)data;
	char buff[64] = "";
	int color;

	display_course(course_curr_type(cp),
		       course_curr_entry(cp), buff, 64, &color);
	gl_frame_set_color(frm, color);
	data_box_set_text(dbox, buff, strlen(buff));
}

static void data_box_next_target_callback(struct gl_frame *frm,
					  const void *data)
{
	struct data_box *dbox = DATA_BOX(frm);
	const struct course *cp = (const struct course *)data;
	char buff[64] = "";
	int color;

	display_course(course_next_type(cp),
		       course_next_entry(cp), buff, 64, &color);
	gl_frame_set_color(frm, color);
	data_box_set_text(dbox, buff, strlen(buff));
}

int graphics_context_init(struct graphics_context **out,
			  const struct course *cp,
			  const struct flight_data *flt,
			  const struct gps_data *gps,
			  const struct mag_data *mag)
{
	struct graphics_context *gc = NULL;
	int x, y, x1, y1, sbar_width, txtcolor, frmcolor, barcolor;
	int footer_height;

	gc = malloc(sizeof(struct graphics_context));
	if (gc == NULL) {
		SYSERR("Out of memory.");
		goto exit;
	}
	gc->context = svgalib_virtual_context_create();
	if (gc->context == NULL) {
		DEBUG("svgalib_virtual_context_create() failed.");
		goto exit_free;
	}

	sbar_width = CTX_HEIGHT >> 3;
	footer_height = sbar_width >> 1;
	x = CTX_HEIGHT >> 2;
	y = (CTX_HEIGHT - x) / 7;
	x1 = (CTX_WIDTH - x - sbar_width) >> 1;
	y1 = CTX_HEIGHT - footer_height;

	txtcolor = svgalib_get_color(31, 31, 31);
	frmcolor = svgalib_get_color(0, 10, 0);
	barcolor = svgalib_get_color(0, 0, 0);

	/* Create clock */
	gl_clock_create(&gc->clock, 0, 0, sbar_width, sbar_width, frmcolor);
	gl_clock_set_bgcolor(GL_CLOCK(gc->clock), barcolor);
	gl_frame_add_callback(gc->clock, RC_GPS_UPDATE, clock_callback, gps);

	/* Create altitude scale bar */
	gl_scale_bar_create(&gc->scale_bar_altitude, 0, sbar_width, sbar_width,
			    CTX_HEIGHT - footer_height - sbar_width, barcolor);
	gl_scale_bar_set_pointer(GL_SCALE_BAR(gc->scale_bar_altitude),
				 GL_SCALE_POINTER_LEFT);
	gl_scale_bar_set_pointer_color(GL_SCALE_BAR(gc->scale_bar_altitude),
				       frmcolor);
	gl_scale_bar_set_text_color(GL_SCALE_BAR(gc->scale_bar_altitude),
				    txtcolor);
	gl_scale_bar_set_color_callback(GL_SCALE_BAR(gc->scale_bar_altitude),
					scale_bar_color_callback_AGL_altitude,
					&flt->altitude);
	gl_frame_add_callback(gc->scale_bar_altitude, RC_FLIGHT_UPDATE,
			      scale_bar_altitude_callback, flt);

	/* Create tracking scale bar */
	gl_scale_bar_create(&gc->scale_bar_tracking, sbar_width, 0,
			    CTX_WIDTH - x - sbar_width, sbar_width, barcolor);
	gl_scale_bar_set_pointer(GL_SCALE_BAR(gc->scale_bar_tracking),
				 GL_SCALE_POINTER_TOP);
	gl_scale_bar_set_pointer_color(GL_SCALE_BAR(gc->scale_bar_tracking),
				       frmcolor);
	gl_scale_bar_set_text_color(GL_SCALE_BAR(gc->scale_bar_tracking),
				    txtcolor);
	gl_scale_bar_set_color_callback(GL_SCALE_BAR(gc->scale_bar_tracking),
					scale_bar_color_callback_tracking,
					&cp->tracking);
	gl_frame_add_callback(gc->scale_bar_tracking, RC_COURSE_UPDATE,
			      scale_bar_tracking_callback, cp);

	/* Create compass */
	gl_compass_create(&gc->compass, CTX_WIDTH - x, 0, x, x, frmcolor);
	gl_compass_set_bgcolor(GL_COMPASS(gc->compass), barcolor);
	gl_frame_add_callback(gc->compass, RC_FLIGHT_UPDATE,
			      compass_callback, flt);

	/* Map area */
	map_context_create(&gc->map_area,
			   sbar_width,
			   sbar_width,
			   CTX_WIDTH - (sbar_width + x),
			   CTX_HEIGHT - (sbar_width + footer_height),
			   barcolor,
			   cp,
			   flt);

	gps_context_create(&gc->gps_context,
			   sbar_width, sbar_width,
			   CTX_WIDTH - (sbar_width + x),
			   CTX_HEIGHT - (sbar_width + footer_height),
			   barcolor);
	gl_frame_add_callback(gc->gps_context, RC_GPS_UPDATE,
			      gps_context_callback, gps);

	file_chooser_create(&gc->file_list,
			    sbar_width,
			    sbar_width,
			    CTX_WIDTH - (sbar_width + x),
			    CTX_HEIGHT - (sbar_width + footer_height),
			    barcolor);

	/* Mag profile view */
	gl_profile_view_create(&gc->profile_frame, sbar_width, sbar_width,
			       CTX_WIDTH - (sbar_width + x),
			       CTX_HEIGHT - (sbar_width + footer_height),
			       barcolor,
			       1, CTX_WIDTH - (sbar_width + x),
			       svgalib_get_color(31, 31, 0),
			       svgalib_get_color(5, 5, 5));
	gl_frame_add_callback(gc->profile_frame, RC_MAG_UPDATE,
			      profile_view_callback, mag);
	/* Profile color */
	gl_profile_view_set_color((struct gl_profile_view *)gc->profile_frame, 0,
				  svgalib_get_color(31, 31, 0));

	/* Curr target data box */
	data_box_create(&gc->data_box_curr_target,
			sbar_width, y1, x1, footer_height, barcolor);
	data_box_set_split(DATA_BOX(gc->data_box_curr_target),
			   SPLIT_HORIZONTAL, 0.4);
	data_box_set_text_color(DATA_BOX(gc->data_box_curr_target), txtcolor);
	data_box_set_caption(DATA_BOX(gc->data_box_curr_target),
			     "Curr Course:");
	gl_frame_add_callback(gc->data_box_curr_target, RC_TARGET_UPDATE,
			      data_box_curr_target_callback, cp);

	/* Next target data box */
	data_box_create(&gc->data_box_next_target, sbar_width + x1, y1, x1,
			footer_height, barcolor);
	data_box_set_split(DATA_BOX(gc->data_box_next_target),
			   SPLIT_HORIZONTAL, 0.4);
	data_box_set_text_color(DATA_BOX(gc->data_box_next_target), txtcolor);
	data_box_set_caption(DATA_BOX(gc->data_box_next_target),
			     "Next Course:");
	gl_frame_add_callback(gc->data_box_next_target, RC_TARGET_UPDATE,
			      data_box_next_target_callback, cp);

	/* Heading data box */
	data_box_create(&gc->data_box_heading,
			CTX_WIDTH - x, x, x, y, frmcolor);
	data_box_set_split(DATA_BOX(gc->data_box_heading), SPLIT_VERTICAL, 0.5);
	data_box_set_text_color(DATA_BOX(gc->data_box_heading), txtcolor);
	data_box_set_caption(DATA_BOX(gc->data_box_heading), "Heading:");
	gl_frame_add_callback(gc->data_box_heading, RC_FLIGHT_UPDATE,
			      data_box_heading_callback, &flt->heading);

	/* GS data box */
	data_box_create(&gc->data_box_GS,
			CTX_WIDTH - x, x + 1 * y, x, y, frmcolor);
	data_box_set_split(DATA_BOX(gc->data_box_GS), SPLIT_VERTICAL, 0.5);
	data_box_set_text_color(DATA_BOX(gc->data_box_GS), txtcolor);
	data_box_set_caption(DATA_BOX(gc->data_box_GS), "GS[km/h]:");
	gl_frame_add_callback(gc->data_box_GS, RC_FLIGHT_UPDATE,
			      data_box_GS_callback, &flt->speed);

	/* DTG data box */
	data_box_create(&gc->data_box_DTG, CTX_WIDTH - x,
			x + 2 * y, x, y, frmcolor);
	data_box_set_split(DATA_BOX(gc->data_box_DTG), SPLIT_VERTICAL, 0.5);
	data_box_set_text_color(DATA_BOX(gc->data_box_DTG), txtcolor);
	data_box_set_caption(DATA_BOX(gc->data_box_DTG), "DTG[km]:");
	gl_frame_add_callback(gc->data_box_DTG, RC_COURSE_UPDATE,
			      data_box_DTG_callback, &cp->distance_to_go);

	/* GPS fix data box */
	data_box_create(&gc->data_box_gpsfix, CTX_WIDTH - x,
			x + 3 * y, x, y, frmcolor);
	data_box_set_split(DATA_BOX(gc->data_box_gpsfix), SPLIT_VERTICAL, 0.5);
	data_box_set_text_color(DATA_BOX(gc->data_box_gpsfix), txtcolor);
	data_box_set_caption(DATA_BOX(gc->data_box_gpsfix), "GPS Fix/Sat:");
	gl_frame_add_callback(gc->data_box_gpsfix, RC_GPS_UPDATE,
			      data_box_gpsfix_callback, gps);

	/* GPS alt data box */
	data_box_create(&gc->data_box_gpsalt, CTX_WIDTH - x,
			x + 4 * y, x, y, frmcolor);
	data_box_set_split(DATA_BOX(gc->data_box_gpsalt), SPLIT_VERTICAL, 0.5);
	data_box_set_text_color(DATA_BOX(gc->data_box_gpsalt), txtcolor);
	data_box_set_caption(DATA_BOX(gc->data_box_gpsalt), "GPS Altitude:");
	gl_frame_add_callback(gc->data_box_gpsalt, RC_GPS_UPDATE,
			      data_box_gpsalt_callback, gps);

	/* GPS lat data box */
	data_box_create(&gc->data_box_gpslat, CTX_WIDTH - x,
			x + 5 * y, x, y, frmcolor);
	data_box_set_split(DATA_BOX(gc->data_box_gpslat), SPLIT_VERTICAL, 0.5);
	data_box_set_text_color(DATA_BOX(gc->data_box_gpslat), txtcolor);
	data_box_set_caption(DATA_BOX(gc->data_box_gpslat), "GPS Latitude:");
	gl_frame_add_callback(gc->data_box_gpslat, RC_GPS_UPDATE,
			      data_box_gpslat_callback, gps);

	/* Mag data box */
	data_box_create(&gc->data_box_mag, CTX_WIDTH - x,
			x + 5 * y, x, y, frmcolor);
	data_box_set_split(DATA_BOX(gc->data_box_mag), SPLIT_VERTICAL, 0.5);
	data_box_set_text_color(DATA_BOX(gc->data_box_mag), txtcolor);

	data_box_set_caption(DATA_BOX(gc->data_box_mag), "MAG Field:");
	gl_frame_add_callback(gc->data_box_mag, RC_MAG_UPDATE,
			      data_box_mag_callback, mag);


	/* GPS Lon data box */
	data_box_create(&gc->data_box_gpslon,
	                CTX_WIDTH - x, x + 6 * y, x, y, frmcolor);
	data_box_set_split(DATA_BOX(gc->data_box_gpslon), SPLIT_VERTICAL, 0.5);
	data_box_set_text_color(DATA_BOX(gc->data_box_gpslon), txtcolor);

	data_box_set_caption(DATA_BOX(gc->data_box_gpslon), "GPS Longitude:");
	gl_frame_add_callback(gc->data_box_gpslon, RC_GPS_UPDATE,
			      data_box_gpslon_callback, gps);

	/* space data box */
	data_box_create(&gc->data_box_space,
			CTX_WIDTH - x, x + 6 * y, x, y, frmcolor);
	data_box_set_split(DATA_BOX(gc->data_box_space), SPLIT_VERTICAL, 0.5);
	data_box_set_text_color(DATA_BOX(gc->data_box_space), txtcolor);
	data_box_set_caption(DATA_BOX(gc->data_box_space), "Disk Space:");
	gl_frame_add_callback(gc->data_box_space, RC_MAG_UPDATE,
				      data_box_disk_space_callback, mag);

	/* Label datum */
	gl_label_create(&gc->label_datum, 0,
			y1, sbar_width, footer_height, frmcolor);
	gl_label_set_color(GL_LABEL(gc->label_datum), txtcolor);
	gl_label_set_font(GL_LABEL(gc->label_datum), FONT_SUN12x22);
	gl_label_set_border_width(GL_LABEL(gc->label_datum), 5);
	gl_frame_add_callback(gc->label_datum, RC_FLIGHT_UPDATE,
			      label_datum_callback, flt);

	gc->mag_disable = 0;
	gc->main_view = VIEW_GPS_CONTEXT;

	/* Initial draw */
	svgalib_set_context(gc->context);

	if (gc->mag_disable) {
		gl_frame_draw(gc->data_box_gpslat, 1);
		gl_frame_draw(gc->data_box_gpslon, 1);
	} else {
		gl_frame_draw(gc->data_box_mag, 1);
		gl_frame_draw(gc->data_box_space, 1);
	}
	gl_frame_draw(gc->data_box_gpsfix, 1);
	gl_frame_draw(gc->data_box_gpsalt, 1);
	gl_frame_draw(gc->data_box_DTG, 1);
	gl_frame_draw(gc->data_box_heading, 1);
	gl_frame_draw(gc->data_box_GS, 1);
	gl_frame_draw(gc->data_box_curr_target, 0);
	gl_frame_draw(gc->data_box_next_target, 0);
	gl_frame_draw(gc->label_datum, 0);
	gl_frame_draw(gc->clock, 0);
	gl_frame_draw(gc->compass, 0);
	gl_frame_draw(gc->scale_bar_altitude, 0);
	gl_frame_draw(gc->scale_bar_tracking, 0);
	gl_frame_draw(gc->gps_context, 0);
	svgalib_show_context(gc->context);

	*out = gc;
	return 0;

 exit_free:
	free(gc);
 exit:
	return -1;
}

void graphics_update(struct graphics_context *gc, rc_t rc)
{
	if (rc & RC_GPS_UPDATE) {
		if (gc->mag_disable) {
			gl_frame_draw(gc->data_box_gpslat, 1);
			gl_frame_draw(gc->data_box_gpslon, 1);
		}
		gl_frame_draw(gc->data_box_gpsfix, 1);
		gl_frame_draw(gc->data_box_gpsalt, 1);
		gl_frame_draw(gc->clock, 1);
	}

	if (rc & RC_MAG_UPDATE) {
		if (!gc->mag_disable) {
			gl_frame_draw(gc->data_box_mag, 1);
			gl_frame_draw(gc->data_box_space, 1);
		}
	}

	if (rc & RC_FLIGHT_UPDATE) {
		gl_frame_draw(gc->label_datum, 0);
		gl_frame_draw(gc->scale_bar_altitude, 1);
		gl_frame_draw(gc->compass, 1);
		gl_frame_draw(gc->data_box_heading, 1);
		gl_frame_draw(gc->data_box_GS, 1);
	}

	if (rc & RC_TARGET_UPDATE) {
		gl_frame_draw(gc->data_box_curr_target, 1);
		gl_frame_draw(gc->data_box_next_target, 1);
	}

	if (rc & RC_COURSE_UPDATE) {
		gl_frame_draw(gc->scale_bar_tracking, 1);
		gl_frame_draw(gc->data_box_DTG, 1);
	}

	switch (gc->main_view) {
	case VIEW_GPS_CONTEXT:
		if (rc & RC_GPS_UPDATE)
			gl_frame_draw(gc->gps_context, 0);
		break;
	case VIEW_FILE_CONTEXT:
		if (rc & RC_MAP_UPDATE)
			gl_frame_draw(gc->file_list, 0);
		break;
	case VIEW_MAG_CONTEXT:
		gl_frame_draw(gc->profile_frame, 0);
		break;
	case VIEW_MAP_CONTEXT:
		gl_frame_draw(gc->map_area, 0);
		break;
	}

	svgalib_show_context(gc->context);
}

void graphics_context_destroy(struct graphics_context *gc)
{
	gl_frame_destroy(gc->scale_bar_tracking);
	gl_frame_destroy(gc->scale_bar_altitude);
	gl_frame_destroy(gc->compass);
	gl_frame_destroy(gc->clock);
	gl_frame_destroy(gc->map_area);
	gl_frame_destroy(gc->gps_context);
	gl_frame_destroy(gc->file_list);
	gl_frame_destroy(gc->label_datum);
	gl_frame_destroy(gc->data_box_curr_target);
	gl_frame_destroy(gc->data_box_next_target);
	gl_frame_destroy(gc->data_box_heading);
	gl_frame_destroy(gc->data_box_GS);
	gl_frame_destroy(gc->data_box_DTG);
	gl_frame_destroy(gc->data_box_gpsfix);
	gl_frame_destroy(gc->data_box_gpsalt);
	gl_frame_destroy(gc->data_box_gpslat);
	gl_frame_destroy(gc->data_box_gpslon);
	gl_frame_destroy(gc->data_box_mag);
	gl_frame_destroy(gc->data_box_space);
	gl_frame_destroy(gc->profile_frame);

	svgalib_virtual_context_destroy(gc->context);
	free(gc);
	gc = NULL;
}

rc_t graphics_controls(struct graphics_context *gc, struct course *cp,
		       struct flight_data *flt, int key)
{
	rc_t rc = RC_NONE;

	switch (key) {
	case 'Q':
		rc |= RC_QUIT;
		break;
	case KEY_ENTER:
		if (gc->main_view == VIEW_GPS_CONTEXT) {
			gc->main_view = VIEW_FILE_CONTEXT;
			rc |= RC_MAP_UPDATE;
		} else if (gc->main_view == VIEW_FILE_CONTEXT) {
			char pgn_file[256] = "";
			if (!file_chooser_get_file(gc->file_list,
						   pgn_file, 256)) {
				course_map_load(cp, pgn_file);
				flight_position_default(cp, &flt->position);
				gc->main_view = VIEW_MAP_CONTEXT;
				rc |= (RC_MAP_UPDATE |
				       RC_COURSE_UPDATE | RC_TARGET_UPDATE);
			} else {
				rc |= RC_QUIT;
			}
		}
		break;
	case KEY_ESC:
		if (gc->main_view == VIEW_MAP_CONTEXT) {
			gc->main_view = VIEW_FILE_CONTEXT;
			course_map_unload(cp);
			rc |= (RC_MAP_UPDATE |
			       RC_COURSE_UPDATE | RC_TARGET_UPDATE);
		}
		break;
	case KEY_ARROWUP:
		if (gc->main_view == VIEW_FILE_CONTEXT) {
			if (!file_chooser_for_each(gc->file_list, 0))
				rc |= RC_MAP_UPDATE;
		}
		break;
	case KEY_ARROWDOWN:
		if (gc->main_view == VIEW_FILE_CONTEXT) {
			if (!file_chooser_for_each(gc->file_list, 1))
				rc |= RC_MAP_UPDATE;
		}
		break;
	case 'Z':
		if (gc->main_view == VIEW_MAP_CONTEXT) {
			map_context_adjust_scale(gc->map_area, MAP_SCALE_UP);
			rc |= RC_MAP_UPDATE;
		} else if (gc->main_view == VIEW_MAG_CONTEXT) {
			struct gl_profile_view *pv =
				(struct gl_profile_view *)gc->profile_frame;
			gl_profile_view_scale_decrement(pv);
		}
		break;
	case 'S':
		if (gc->main_view == VIEW_MAP_CONTEXT) {
			map_context_adjust_scale(gc->map_area, MAP_SCALE_DN);
			rc |= RC_MAP_UPDATE;
		} else if (gc->main_view == VIEW_MAG_CONTEXT) {
			struct gl_profile_view *pv =
				(struct gl_profile_view *)gc->profile_frame;
			gl_profile_view_scale_reset(pv);
		}
		break;
	case 'A':
		if (gc->main_view == VIEW_MAP_CONTEXT) {
			map_context_adjust_scale(gc->map_area, MAP_SCALE_AUTO);
			rc |= RC_MAP_UPDATE;
		} else if (gc->main_view == VIEW_MAG_CONTEXT) {
			struct gl_profile_view *pv =
				(struct gl_profile_view *)gc->profile_frame;
			gl_profile_view_scale_increment(pv);
		}
		break;
	case 'R':
		gc->mag_disable = ((gc->mag_disable) ? 0 : 1);
		rc |= RC_MAG_UPDATE | RC_GPS_UPDATE;
		break;
	case 'V':
		if (gc->main_view == VIEW_MAG_CONTEXT) {
			gc->main_view = VIEW_FILE_CONTEXT;
			rc |= RC_MAP_UPDATE;
		} else {
			gc->main_view = VIEW_MAG_CONTEXT;
			rc |= RC_MAG_UPDATE;
		}
		break;
	default:
		break;
	}
	return rc;
}
