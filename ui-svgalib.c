#include <math.h>
#include <sys/time.h>
#include <ctype.h>

#include "ui.h"
#include "svgalib.h"
#include "debug.h"
#include "simulant.h"
#include "gps.h"
#include "mag.h"
#include "ral.h"
#include "log.h"
#include "doch.h"
#include "event.h"
#include "trackbar.h"
#include "course.h"
#include "flight.h"
#include "keyboard.h"

int ui_init(int *argc, char ***argv)
{
	if (svgalib_init(VGAMODE) != 0) {
		DEBUG("svgalib_init() failed.");
		return -1;
	}

	/**
	 * Set raw keyboard only after initializing svgalib library,
	 * otherwise after exit pseudo-tty would be in awkward state.
	 * Same goes for the reset function also..!!
	 */
	if (keyboard_init() != 0) {
		DEBUG("keyboard_init() failed.");
		svgalib_exit();
		return -1;
	}

	icons_initialize();
	return 0;
}

int ui_run(run_mode_t run_mode)
{
	struct gps_data gps;
	struct mag_data mag;
	struct ral_data ral;
	struct course course;
	struct flight_data flt;
	struct graphics_context *gc = NULL;
	struct trackbar_context *tbar_ctx = NULL;
	int timeout = 200;

	gps_data_init(&gps);
	mag_data_init(&mag);
	ral_data_init(&ral);
	course_init(&course);
	flight_init(&flt);

	if (graphics_context_init(&gc, &course, &flt, &gps, &mag) != 0) {
		DEBUG("graphics_context_init() failed.");
		return -1;
	}
	trackbar_context_init(&tbar_ctx);

	if (run_mode == RUN_REAL_TIME)
		timeout = 500;

	for (;;) {
		rc_t rc = RC_NONE;
		event_t event;

		/* Wait on event */
		event = event_wait_poll(timeout);

		if (event & EVENT_KEYPRESSED) {
			int key = toupper(keyboard_getkey());
			rc |= graphics_controls(gc, &course, &flt, key);
			if (rc & RC_QUIT)
				break;
			rc |= flight_controls(&flt, &course, run_mode, key);
			rc |= course_controls(&course, &flt, key);
			trackbar_controls(&course, key);
		}

		if (run_mode == RUN_REAL_TIME) {
			if (event & EVENT_GPS_READY) {
				if (!gps_acquire_data(&gps))
					rc |= RC_GPS_UPDATE;
			}
			if (event & (EVENT_GPS_READY | EVENT_TIMEOUT)) {
				if (!ral_acquire_data(&ral))
					rc |= RC_RAL_UPDATE;
			}
			if (event & EVENT_MAG_READY) {
				if (!mag_acquire_data(&mag))
					rc |= RC_MAG_UPDATE;
			}

		} else if (run_mode == RUN_SIM_DATA) {
			rc |= sim_data_update(&gps, &ral, &mag);
		}

		rc |= flight_update(&flt, &course, &gps, &ral, run_mode, rc);
		rc |= course_update(&course, &flt, rc);
		trackbar_context_display(tbar_ctx, &course, &flt, rc);
		graphics_update(gc, rc);
		doch_data_out(&course, &gps, &ral, rc);
		log_data(&course, &gps, &ral, &mag, rc);
	}

	trackbar_context_free(tbar_ctx);
	graphics_context_destroy(gc);
	return 0;
}

void ui_exit(void)
{
	keyboard_close();
	svgalib_exit();
}
