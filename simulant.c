#include <math.h>
#include <stdlib.h>

#include "simulant.h"
#include "internals.h"

#define TRACKING_TOLERANCE		3.0f
#define KNOTS_TO_KMPH_FACTOR		1.41f
#define ROTATION_ANGLE_DEFAULT		(5 * DEG_TO_RAD)
#define SIMULATOR_SPEED_LIMIT_UP	150.0f
#define SIMULATOR_SPEED_LIMIT_DN	1.0f

static struct point diff = {
	.x = 0.0,
	.y = 1.26,
};

void simulator_speed_control(simulator_flag_t flag)
{
	double speed = sqrt(pow(diff.x, 2) + pow(diff.y, 2));

	switch (flag) {
	case SIMULATOR_SPEED_UP:
		if (speed < 150.0) {
			diff.x *= KNOTS_TO_KMPH_FACTOR;
			diff.y *= KNOTS_TO_KMPH_FACTOR;
		}
		break;
	case SIMULATOR_SPEED_DOWN:
		if (speed > 1.0) {
			diff.x /= KNOTS_TO_KMPH_FACTOR;
			diff.y /= KNOTS_TO_KMPH_FACTOR;
		}
		break;
	default:
		break;
	}
}

void simulator_orientation_control(simulator_flag_t flag)
{
	struct point p = diff;
	double sinfi = sin(ROTATION_ANGLE_DEFAULT);
	double cosfi = cos(ROTATION_ANGLE_DEFAULT);

	switch (flag) {
	case SIMULATOR_ROTATE_LEFT:
		p.x = sinfi * diff.y + cosfi * diff.x;
		p.y = cosfi * diff.y - sinfi * diff.x;
		break;
	case SIMULATOR_ROTATE_RIGHT:
		p.x = cosfi * diff.x - sinfi * diff.y;
		p.y = sinfi * diff.x + cosfi * diff.y;
		break;
	default:
		break;
	}
	diff = p;
}

static struct point auto_control(const struct point *pos, double tracking)
{
	struct point p;
	double fi = 0.0, sinfi, cosfi;

	tracking = tracking * 0.25;

	if (tracking > TRACKING_TOLERANCE)
		fi = TRACKING_TOLERANCE;
	else if (tracking < TRACKING_TOLERANCE)
		fi = tracking;

	if (tracking < -TRACKING_TOLERANCE)
		fi = -TRACKING_TOLERANCE;
	else if (tracking > -TRACKING_TOLERANCE)
		fi = tracking;

	fi = fi * DEG_TO_RAD;
	sinfi = sin(fi);
	cosfi = cos(fi);

	p.x = sinfi * pos->y + cosfi * pos->x;
	p.y = cosfi * pos->y - sinfi * pos->x;

	return p;
}

void simulator_update(const double *tracking, struct point *pos,
		      double *timestamp, double *altitude)
{
	if (tracking != NULL)
		diff = auto_control(&diff, *tracking);

	pos->x += diff.x;
	pos->y += diff.y;
	*timestamp += 0.2;
	*altitude += ((random() % 20) - 9.5) / 10.0;
}
