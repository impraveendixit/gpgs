/*******************************************************************************
 * Copyright (c) 2016, Praveen Kumar Dixit <impraveendixit@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * FILE NAME: adc-rtd6430.c
 *
 * DESCRIPTION: Implement DM6430 ADC device interface.
 ******************************************************************************/


#include <stdlib.h>

#include "adc-private.h"
#include "debug.h"
#include "lib/dm6430lib.h"	/* Vendor library header file */

#ifndef CONFIG_DEBUG_ADC_DEVICE
#undef DEBUG
#define DEBUG(M, ...) do {} while (0)
#endif

/* Size of FIFO buffer in bytes */
#define FIFO_SIZE	1024

/* Sampling rate in Hz */
#define SAMPLE_RATE	1000

/* Number Bits divided by AD Range */
#define ADSLOPE	(65536.0 / 20.0)

#define ADC_CHANNEL DM6430HR_AIN2

#define ADC_RTD6430_DEVICE(device)  ((struct adc_rtd6430_device *)(device))

struct adc_rtd6430_device {
	device_t base;
	int descriptor;
};

static int __init_device(int minor_number)
{
	int descriptor = -1;
	double actual_rate = 0.0;
	ADTableRow ADTable = {
		.Channel	= ADC_CHANNEL,
		.Gain		= DM6430HR_GAINx1,
		.Se_Diff	= DM6430HR_SE_SE,
		.Pause		= 0,
		.Skip		= 0,
	};

	descriptor = OpenBoard6430(minor_number);
	if (descriptor == -1) {
		SYSERR("OpenBoard6430() FAILED.");
		goto exit;
	}
	if (InitBoard6430(descriptor) == -1) {
		SYSERR("InitBoard6430() FAILED");
		goto exit_close;
	}
	if (SetPacerClock6430(descriptor, SAMPLE_RATE, &actual_rate) != 0) {
		SYSERR("SetPacerClock6430() FAILED");
		goto exit_close;
	}
	INFO("Actual pacer clock rate: %lf", actual_rate);

	if (SetStartTrigger6430(descriptor, DM6430HR_START_TRIG_SOFTWARE) != 0) {
		SYSERR("SetStartTrigger6430() FAILED");
		goto exit_close;
	}
	if (SetStopTrigger6430(descriptor, DM6430HR_STOP_TRIG_SOFTWARE) != 0) {
		SYSERR("SetStopTrigger6430() FAILED");
		goto exit_close;
	}
	if (SetConversionSelect6430(descriptor,
				    DM6430HR_CONV_PACER_CLOCK) != 0) {
		SYSERR("SetConversionSelect6430() FAILED");
		goto exit_close;
	}
	if (LoadADTable6430(descriptor, 1, &ADTable) != 0) {
		SYSERR("LoadADTable6430() FAILED");
		goto exit_close;
	}
	if (EnableTables6430(descriptor, 1, 0) != 0) {
		SYSERR("EnableTables6430() FAILED");
		goto exit_close;
	}
	if (ClearADFIFO6430(descriptor) != 0) {
		SYSERR("ClearADFIFO6430() FAILED");
		goto exit_close;
	}
	if (StartConversion6430(descriptor) != 0) {
		SYSERR("StartConversion6430() FAILED");
		goto exit_close;
	}
	return descriptor;

 exit_close:
	CloseBoard6430(descriptor);
 exit:
	return -1;
}

static void adc_rtd6430_device_close(device_t *abstract)
{
	struct adc_rtd6430_device *dev = ADC_RTD6430_DEVICE(abstract);

	if (CloseBoard6430(dev->descriptor) != 0)
		SYSERR("CloseBoard6430() Failed.");
	device_free((device_t *)dev);
}

static int adc_rtd6430_device_get_descriptor(device_t *abstract)
{
	struct adc_rtd6430_device *dev = ADC_RTD6430_DEVICE(abstract);
	return dev->descriptor;
}

static int adc_rtd6430_device_read(device_t *abstract, void *buf, size_t size)
{
	struct adc_rtd6430_device *dev = ADC_RTD6430_DEVICE(abstract);
	register int i = 0;
	double sum = 0;
	int stopped = 0;

	for (i = 0; i < FIFO_SIZE; i++) {
		int16_t sample = 0;
		int empty = 0;

		/* Return -1 if function fails */
		if (IsADFIFOEmpty6430(dev->descriptor, &empty) != 0) {
			SYSERR("IsADFIFOEmpty6430() FAILED");
			return -1;
		}

		if (empty)
			break;

		if (ReadADData6430(dev->descriptor, &sample) != 0) {
			SYSERR("ReadADData6430() FAILED");
			return -1;
		}

		/* convert volt output to millivolt */
		sum += (double)(sample * 1000.0) / ADSLOPE;
	}

	/* Check whether ADC is halted because of FIFO full */
	if (IsADHalted6430(dev->descriptor, &stopped) != 0) {
		SYSERR("IsADHalted6430() failed.");
		return -1;
	}

	/* If true, adc halted, so clear fifo and restart conversion */
	if (stopped) {
		if (ClearADFIFO6430(dev->descriptor) != 0) {
			SYSERR("ClearADFIFO6430() FAILED");
			return -1;
		}
		if (StartConversion6430(dev->descriptor) != 0) {
			SYSERR("StartConversion6430() FAILED");
			return -1;
		}
	}

	/* No sample fetched */
	if (i == 0)
		return -1;

	*(double *)buf = sum / i;
	return 0;
}

static const device_ops_t adc_rtd6430_device_ops = {
	.get_descriptor = adc_rtd6430_device_get_descriptor,
	.read           = adc_rtd6430_device_read,
	.write          = NULL,
	.close          = adc_rtd6430_device_close,
};

int adc_rtd6430_device_open(device_t **out, const void *userdata)
{
	struct adc_rtd6430_device *dev = NULL;
	int minor = *(const int *)userdata;

	if (out == NULL)
		return -1;

	dev = ADC_RTD6430_DEVICE(device_allocate(sizeof(struct adc_rtd6430_device)));
	if (dev == NULL) {
		SYSERR("Failed to allocate for device structure");
		goto exit;
	}

	dev->descriptor = __init_device(minor);
	if (dev->descriptor == -1) {
		ERROR("Failed to initialize adc device.");
		goto exit_free;
	}

	device_register_operations((device_t *)dev, &adc_rtd6430_device_ops);
	*out = (device_t *)dev;
	return 0;

 exit_free:
	device_free((device_t *)dev);
 exit:
	return -1;
}
