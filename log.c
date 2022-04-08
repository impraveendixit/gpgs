#include <sys/statfs.h>
#include <time.h>

#include "log.h"
#include "debug.h"
#include "gps.h"
#include "ral.h"
#include "mag.h"
#include "config.h"
#include "course.h"
#include "internals.h"

/* 10 minutes */
#define RECORD_TIMEOUT  600

/* Log file pointer */
static FILE *log_fp = NULL;
static time_t record_timer;

static int open_log_file(const char *path, FILE **fpp)
{
	char filename[256] = "";
	struct tm utc;
	time_t rawtime;

	if (fpp == NULL)
		return -1;

	time(&rawtime);
	gmtime_r(&rawtime, &utc);
	snprintf(filename, 256, "%sgpgs_%02d%02d%02d_%02d%02d.dat",
		 path,
		 (utc.tm_year + 1900) % 100,
		 utc.tm_mon + 1,
		 utc.tm_mday,
		 utc.tm_hour,
		 utc.tm_min);

	/* Opening file first time */
	if (*fpp == NULL)
		*fpp = fopen(filename, "w");
	else
		*fpp = freopen(filename, "w", *fpp);

	if (*fpp == NULL) {
		SYSERR("Failed to open file: %s", filename);
		return -1;
	}
	fprintf(*fpp,
		"#================================\n"
		"# GPGS-%s DATA FILE:\n"
		"# %s"
		"# email: impraveendixit@gmail.com\n"
		"#================================\n\n",
		GPGS_VERSION, ctime(&rawtime));

	fprintf(*fpp, "GPSTime,GPSLat,GPSLon,GPSAlt,RDRAlt,Line,MAGField\n");
	fflush(*fpp);
	return 0;
}

/**
 * This function gets free space in the mounted file system.
 *
 * @path: Pointer to mounted file system path.
 * @return: Free space percent left in file system.
 */
float fetch_disk_space(const char *path)
{
	struct statfs stfs;

	if (!statfs(path, &stfs))
		return ((stfs.f_bavail / (float)stfs.f_blocks) * 100.0);
	else
		return 0;
}

void log_data(const struct course *cp, const struct gps_data *gps,
	      const struct ral_data *ral,
	      const struct mag_data *mag, rc_t rc)
{
	time_t curtime;

	if (log_fp == NULL || !cp->map_loaded)
		return;

	time(&curtime);
	if ((curtime - record_timer) > RECORD_TIMEOUT) {
		if (open_log_file(log_directory, &log_fp) != 0) {
			DEBUG("open_log_file() failed.");
			return;
		}
		record_timer = curtime;
	}

	/* Data log only on GPS update as it is marked
	 * as datum for extracting data later on.
	 */
	if (rc & RC_GPS_UPDATE) {
		fprintf(log_fp, "%9.2lf,%11.7lf,%11.7lf,%7.2lf,%7.2lf,%d,%9.3lf\n",
			gps->gga.utc_time, gps->gga.latitude, gps->gga.longitude,
			gps->gga.altitude, ral->agl_height,
			cp->active_line_id, mag->field_value);
		fflush(log_fp);
	}
}

int log_start(void)
{
	FILE *fp = NULL;

	if (log_disable) {
		WARN("Data Log disabled..");
		return -1;
	}

	if (open_log_file(log_directory, &fp) != 0) {
		DEBUG("open_log_file() failed.");
		return -1;
	}

	record_timer = time(NULL);
	log_fp = fp;
	return 0;
}

void log_stop(void)
{
	if (log_fp != NULL)
		fclose(log_fp);
	log_fp = NULL;
}
