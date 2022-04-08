#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doch.h"
#include "gps.h"
#include "ral.h"
#include "serial.h"
#include "course.h"
#include "debug.h"
#include "device.h"

#ifndef CONFIG_DEBUG_DOCH_DEVICE
#undef DEBUG
#define DEBUG(M, ...) do {} while (0)
#endif

struct doch_ports {
	device_t **port_list;
	int nr_ports;
};

static struct doch_ports *doch_ports_create(int nr)
{
	struct doch_ports *dop = calloc(1, sizeof(struct doch_ports));
	if (dop == NULL) {
		SYSERR("Failed to allocate doch port structure.");
		goto exit;
	}

	dop->port_list = calloc(nr, sizeof(device_t *));
	if (dop->port_list == NULL) {
		SYSERR("Failed to allocate device structure.");
		goto cleanup;
	}
	dop->nr_ports = nr;
	return dop;

 cleanup:
	free(dop);
 exit:
	return NULL;
}

static void doch_ports_destroy(struct doch_ports *dop)
{
	free(dop->port_list);
	free(dop);
}

static struct doch_ports *out_ports = NULL;

int doch_data_out(const struct course *cp, const struct gps_data *gps,
		  const struct ral_data *ral, rc_t rc)
{
	register int i;
	char buff[256] = "";
	int retval = -1;

	if (out_ports == NULL) {
		DEBUG("Invalid arguments.");
		return -1;
	}

	/* No data sent when map not loaded */
	if (!cp->map_loaded)
		return -1;

	if (!(rc & RC_GPS_UPDATE)) {
		DEBUG("NO GPS update.");
		return -1;
	}

	snprintf(buff, 256, "%s\n$RDALT,%-.1lf\n$LINE,%d\n",
		 gps->nmea_string, ral->agl_height, cp->active_line_id);

	for (i = 0; i < out_ports->nr_ports; i++) {
		device_t *dev = out_ports->port_list[i];
		if (dev == NULL)
			continue;
		if (device_write(dev, buff, strlen(buff)) != 0) {
			DEBUG("Failed to write data to DOCH device port.");
			retval = -1;
		}
	}
	return retval;
}

int doch_start(void)
{
	register int i;
	struct doch_ports *dop = NULL;
	int failed = 0;
	struct serial_attribute attr;
	char *portnames[2] = {"/dev/ttyS2", "/dev/ttyS3"};

	dop = doch_ports_create(2);
	if (dop == NULL) {
		DEBUG("Failed to allocate DOCH ports.");
		goto exit;
	}

	attr.baudrate = 9600;
	attr.databits = 8;
	attr.stopbits = STOPBITS_ONE;
	attr.parity = PARITY_NONE;
	attr.flowcontrol = FLOWCONTROL_NONE;
	attr.canonical_read = 0;
	attr.timeout = 0;
	for (i = 0; i < dop->nr_ports; i++) {
		attr.name = portnames[i];
		if (device_open(&dop->port_list[i], DEVICE_DOCH, &attr) != 0) {
			DEBUG("Failed to initialize DOCH device: %s", attr.name);
			failed++;
		}
	}
	if (failed == dop->nr_ports) {
		WARN("No DOCH device opened.");
		goto exit_free;
	}

	out_ports = dop;
	return 0;

 exit_free:
	doch_ports_destroy(dop);
 exit:
	return -1;
}

void doch_stop(void)
{
	if (out_ports)
		doch_ports_destroy(out_ports);
}
