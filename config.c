#include "config.h"
#include "debug.h"
#include "lib/confuse.h"

run_mode_t run_mode = 0;
double survey_height_AGL = 263;
double HAC_height_MSL = 4000;
double warning_height_AGL = 150;
int catch_radius = 100;
int turn_radius = 150;
char map_directory[256] = "/mnt/dataflash/map/";
char log_directory[256] = "/mnt/dataflash/log/";
int log_disable = 1;
int mag_disable = 1;

static run_mode_t get_run_mode(int val)
{
	run_mode_t mode;

	switch (val) {
	default:
	case 0:
		mode = RUN_REAL_TIME;
		break;
	case 1:
		mode = RUN_SIM_DATA;
		break;
	case 2:
		mode = RUN_SIM_MANUAL;
		break;
	case 3:
		mode = RUN_SIM_AUTO;
		break;
	}

	return mode;
}

int read_config_file(const char *cfg_file)
{
	int retval = -1;
	cfg_opt_t opts[] = {
		CFG_INT("APP_RUN_MODE", 0, CFGF_NONE),
		CFG_FLOAT("SURVEY_HEIGHT_AGL", 263, CFGF_NONE),
		CFG_FLOAT("HAC_HEIGHT_MSL", 4000, CFGF_NONE),
		CFG_FLOAT("TOUCHDOWN_WARN_HEIGHT_AGL", 150, CFGF_NONE),
		CFG_INT("CATCH_RADIUS", 100, CFGF_NONE),
		CFG_INT("TURN_RADIUS", 150, CFGF_NONE),
		CFG_STR("MAP_DIRECTORY", "/mnt/dataflash/map/", CFGF_NONE),
		CFG_BOOL("LOG_DISABLED", cfg_true, CFGF_NONE),
		CFG_STR("LOG_DIRECTORY", "/mnt/dataflash/log/", CFGF_NONE),
		CFG_BOOL("MAG_DISABLED", cfg_true, CFGF_NONE),
		CFG_END()
	};
	cfg_t *cfg = cfg_init(opts, CFGF_NONE);

	if (cfg_parse(cfg, cfg_file) != CFG_SUCCESS) {
		ERROR("Configuration file '%s' couldn't be read.\n", cfg_file);
		retval = -1;
	} else {
		run_mode = get_run_mode(cfg_getint(cfg, "APP_RUN_MODE"));
		survey_height_AGL = cfg_getfloat(cfg, "SURVEY_HEIGHT_AGL");
		HAC_height_MSL = cfg_getfloat(cfg, "HAC_HEIGHT_MSL");
		warning_height_AGL = cfg_getfloat(cfg, "TOUCHDOWN_WARN_HEIGHT_AGL");
		catch_radius = cfg_getint(cfg, "CATCH_RADIUS");
		turn_radius = cfg_getint(cfg, "TURN_RADIUS");
		log_disable = cfg_getbool(cfg, "LOG_DISABLED");
		mag_disable = cfg_getbool(cfg, "MAG_DISABLED");
		snprintf(map_directory, 256, "%s", cfg_getstr(cfg, "MAP_DIRECTORY"));
		snprintf(log_directory, 256, "%s", cfg_getstr(cfg, "LOG_DIRECTORY"));
		retval = 0;

		cfg_free(cfg);
	}

	INFO("Configuration setting:");
	INFO("APP run mode: %i", run_mode);
	INFO("Survey Height AGL: %lf", survey_height_AGL);
	INFO("HAC Height MSL: %lf", HAC_height_MSL);
	INFO("TouchDown Warn Height AGL: %lf", warning_height_AGL);
	INFO("Catch radius:%d", catch_radius);
	INFO("Turn radius:%d", turn_radius);
	INFO("MAP Directory: %s", map_directory);
	INFO("LOG disabled: %d", log_disable);
	INFO("LOG Directory: %s", log_directory);
	INFO("MAG disabled: %d", mag_disable);

	return retval;
}
