#include <stdio.h>
#include <stdlib.h>

#include "ui.h"
#include "ral.h"
#include "gps.h"
#include "mag.h"
#include "log.h"
#include "doch.h"
#include "debug.h"
#include "config.h"
#include "simulant.h"
#include "trackbar.h"

int main(int argc, char **argv)
{
	if (argc > 1)
		read_config_file(argv[1]);

	if (ui_init(&argc, &argv) != 0) {
		DEBUG("Failed to initialize user interface.");
		return EXIT_FAILURE;
	}

	trackbar_start();

	if (run_mode == RUN_REAL_TIME) {
		gps_start();
		ral_start();
		mag_start();
	} else if (run_mode == RUN_SIM_DATA) {
		sim_data_start();
	}

	doch_start();
	log_start();

	ui_run(run_mode);

	log_stop();
	doch_stop();
	trackbar_stop();
	if (run_mode == RUN_REAL_TIME) {
		ral_stop();
		gps_stop();
		mag_stop();
	} else if (run_mode == RUN_SIM_DATA) {
		sim_data_stop();
	}

	ui_exit();
	return EXIT_SUCCESS;
}
