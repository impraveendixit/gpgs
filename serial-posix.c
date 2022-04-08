#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "serial.h"
#include "debug.h"

struct serial_port_t {
	/* Keep old port attributes */
	struct termios tio;
	/* Read timeout in milliseconds */
	int timeout;
	/* port descriptor */
	int descriptor;
};

int serial_port_descriptor(serial_port_t *port)
{
    return port->descriptor;
}

void serial_set_timeout(serial_port_t *port, int timeout)
{
	port->timeout = timeout;
}

int serial_open(serial_port_t **out, const char *name)
{
	serial_port_t *port = NULL;

	if (out == NULL || name == NULL) {
		ERROR("Invalid argument.");
		goto exit;
	}

	/* Create and initialize serial object */
	port = malloc(sizeof(serial_port_t));
	if (port == NULL) {
		SYSERR("Failed to allocate for serial port.");
		goto exit;
	}
	port->descriptor = -1;
	port->timeout = -1;	/* default to blocking read */

	/**
	 * Open port for reading and writing and not as controlling tio
	 * because we don't want to get killed if linenoise sends CTRL-C.
	 * Open the port in non-blocking mode, to return immediately
	 * without waiting for the modem connection to complete.
	 */
	INFO("Opening Serial Port: %s", name);
	port->descriptor = open(name, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (port->descriptor < 0) {
		SYSERR("Failed to open serial port: %s", name);
		goto exit_free;
	}

	/**
	 * Note that open() follows POSIX semantics: multiple open() calls to
	 * the same file will succeed unless the TIOCEXCL ioctl is issued.
	 * This will prevent additional opens except by root-owned processes.
	 */
	if (ioctl(port->descriptor, TIOCEXCL, NULL) != 0) {
		SYSERR("ioctl() error.");
		goto exit_close;
	}

	/* Old setting is saved in this structure to restore later.. */
	if (tcgetattr(port->descriptor, &port->tio) != 0) {
		SYSERR("tcgetattr() failed.");
		goto exit_close;
	}

	*out = port;
	return 0;

 exit_close:
	close(port->descriptor);
 exit_free:
	free(port);
 exit:
	return -1;
}

void serial_close(serial_port_t *port)
{
	/* Disable exclusive access mode. */
	if (ioctl(port->descriptor, TIOCNXCL, NULL) != 0)
		SYSERR("ioctl() error.");

	if (tcsetattr(port->descriptor, TCSANOW, &port->tio) != 0)
		SYSERR("tcsetattr() failed.");

	if (close(port->descriptor) != 0)
		SYSERR("Failed to close serial port.");

	free(port);
	port = NULL;
}

int serial_configure(const serial_port_t *port, unsigned int baudrate,
		unsigned int databits, stopbits_t stopbits, parity_t parity,
		flowcontrol_t flowcontrol, unsigned int canonical_read)
{
	struct termios tio;
	speed_t baud = 0;

	bzero(&tio, sizeof(struct termios));

	INFO("Serial port configuration: baudrate=%i, "
		"databits=%i, parity=%i, stopbits=%i, flowcontrol=%i",
		baudrate, databits, parity, stopbits, flowcontrol);

	/* Retrieve old setting */
	tio = port->tio;

	/* Setup raw input/output mode without echo. */
	tio.c_iflag &= ~(IGNBRK | BRKINT | ISTRIP | INLCR | IGNCR | ICRNL);
	tio.c_oflag &= ~(OPOST);
	tio.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);

	/* Enable the receiver (CREAD) and ignore modem control lines (CLOCAL). */
	tio.c_cflag |= (CLOCAL | CREAD);

	/* Enable canonical mode */
	if (canonical_read)
		tio.c_lflag |= ICANON;

	/**
	 * VMIN is the minimum number of characters for non-canonical read
	 * and VTIME is the timeout in deciseconds for non-canonical read.
	 * Setting both of these parameters to zero implies that a read
	 * will return immediately, only giving the currently available
	 * characters (non-blocking read behaviour). However, a non-blocking
	 * read (or write) can also be achieved by using O_NONBLOCK.
	 * But together with VMIN = 1, it becomes possible to recognize
	 * the difference between a timeout and modem disconnect (EOF)
	 * when read() returns zero.
	 */
	tio.c_cc[VMIN]	= 1;
	tio.c_cc[VTIME] = 0;

	switch (baudrate) {
	case 0: baud = B0; break;
	case 50: baud = B50; break;
	case 75: baud = B75; break;
	case 110: baud = B110; break;
	case 134: baud = B134; break;
	case 150: baud = B150; break;
	case 200: baud = B200; break;
	case 300: baud = B300; break;
	case 600: baud = B600; break;
	case 1200: baud = B1200; break;
	case 1800: baud = B1800; break;
	case 2400: baud = B2400; break;
	case 4800: baud = B4800; break;
	case 9600: baud = B9600; break;
	case 19200: baud = B19200; break;
	case 38400: baud = B38400; break;
#ifdef B57600
	case 57600: baud = B57600; break;
#endif
#ifdef B115200
	case 115200: baud = B115200; break;
#endif
#ifdef B230400
	case 230400: baud = B230400; break;
#endif
#ifdef B460800
	case 460800: baud = B460800; break;
#endif
#ifdef B500000
	case 500000: baud = B500000; break;
#endif
#ifdef B576000
	case 576000: baud = B576000; break;
#endif
#ifdef B921600
	case 921600: baud = B921600; break;
#endif
#ifdef B1000000
	case 1000000: baud = B1000000; break;
#endif
#ifdef B1152000
	case 1152000: baud = B1152000; break;
#endif
#ifdef B1500000
	case 1500000: baud = B1500000; break;
#endif
#ifdef B2000000
	case 2000000: baud = B2000000; break;
#endif
#ifdef B2500000
	case 2500000: baud = B2500000; break;
#endif
#ifdef B3000000
	case 3000000: baud = B3000000; break;
#endif
#ifdef B3500000
	case 3500000: baud = B3500000; break;
#endif
#ifdef B4000000
	case 4000000: baud = B4000000; break;
#endif
	default:
		baud = B9600;
		WARN("No baudrate option. Default baud rate chosen as 9600.");
		break;
	}

	if (cfsetispeed(&tio, baud) != 0) {
		SYSERR("cfsetispeed() failed.");
		return -1;
	}
	if (cfsetospeed(&tio, baud) != 0) {
		SYSERR("cfsetospeed() failed.");
		return -1;
	}

	tio.c_cflag &= ~CSIZE;
	switch (databits) {
	case 5:
		tio.c_cflag |= CS5;
		break;
	case 6:
		tio.c_cflag |= CS6;
		break;
	case 7:
		tio.c_cflag |= CS7;
		break;
	default:
		WARN("No databits option. Default data bits chosen as 8.");
	case 8:
		tio.c_cflag |= CS8;
		break;
	}

#ifdef CMSPAR
	tio.c_cflag &= ~(PARENB | PARODD | CMSPAR);
#else
	tio.c_cflag &= ~(PARENB | PARODD);
#endif	/* CMSPAR */
	tio.c_iflag &= ~(IGNPAR | PARMRK | INPCK);

	switch (parity) {
	default:
		WARN("No parity option. Default parity level set as NONE.");
	case PARITY_NONE:
		tio.c_iflag |= IGNPAR;
		break;
	case PARITY_EVEN:
		tio.c_cflag |= PARENB;
		tio.c_iflag |= INPCK;
		break;
	case PARITY_ODD:
		tio.c_cflag |= (PARENB | PARODD);
		tio.c_iflag |= INPCK;
		break;
#ifdef CMSPAR
	case PARITY_MARK:
		tio.c_cflag |= (PARENB | PARODD | CMSPAR);
		tio.c_iflag |= INPCK;
		break;

	case PARITY_SPACE:
		tio.c_cflag |= (PARENB | CMSPAR);
		tio.c_iflag |= INPCK;
		break;
#endif	/* CMSPAR */
	}

	switch (stopbits) {
	default:
		WARN("No stopbits option found. Default stop bits set as 1.");
	case STOPBITS_ONE:
		tio.c_cflag &= ~CSTOPB;
		break;
	case STOPBITS_TWO:
		tio.c_cflag |= CSTOPB;
		break;
	}

	switch (flowcontrol) {
	default:
		WARN("No flowcontrol option. Default flowcontrol set as NONE");
	case FLOWCONTROL_NONE:
#ifdef CRTSCTS
		tio.c_cflag &= ~CRTSCTS;
#endif	/* CRTSCTS */
		tio.c_iflag &= ~(IXON | IXOFF | IXANY);
		break;
	case FLOWCONTROL_HARDWARE:
#ifdef CRTSCTS
		tio.c_cflag |= CRTSCTS;
		tio.c_iflag &= ~(IXON | IXOFF | IXANY);
		break;
#else
		ERROR("Given flow control not supported by hardware.");
		return -1;
#endif	/* CRTSCTS */
	case FLOWCONTROL_SOFTWARE:
#ifdef CRTSCTS
		tio.c_cflag &= ~CRTSCTS;
#endif	/* CRTSCTS */
		tio.c_iflag |= (IXON | IXOFF);
		break;
	}

	/* Flush the line and activate the new settings immediately.*/
	if (tcflush(port->descriptor, TCIOFLUSH) != 0)
		SYSERR("tcflush() failed.");

	if (tcsetattr(port->descriptor, TCSANOW, &tio) != 0) {
		SYSERR("tcsetattr() failed.");
		return -1;
	}

	return 0;
}

int serial_read(const serial_port_t *port,
		void *buf, size_t size, size_t *actual)
{
	size_t nbytes = 0;
	struct timeval tve; /* The absolute target time. */
	int init = 1;
	int retval = -1;

	while (nbytes < size) {
		fd_set fds;
		struct timeval tvt;

		FD_ZERO(&fds);	/* Clear file descriptor set */
		FD_SET(port->descriptor, &fds);	/* Add port descriptor to set */

		if (port->timeout > 0) {
			struct timeval now;
			if (gettimeofday(&now, NULL) != 0) {
				SYSERR("gettimeofday() failed.");
				goto exit;
			}
			if (init) {
				/* Calculate the initial timeout. */
				tvt.tv_sec  = (port->timeout / 1000);
				tvt.tv_usec = (port->timeout % 1000) * 1000;
				timeradd(&now, &tvt, &tve); /* Calculate the target time. */
			} else {
				/* Calculate the remaining timeout. */
				if (timercmp(&now, &tve, <))
					timersub(&tve, &now, &tvt);
				else
					timerclear(&tvt);
			}
			init = 0;

		} else if (port->timeout == 0) {
			timerclear(&tvt);
		}

		/* wait for data on port */
		int rc = select(port->descriptor + 1, &fds, NULL, NULL,
				port->timeout >= 0 ? &tvt : NULL);
		if (rc < 0) {
			if (errno == EINTR)
				continue; /* Retry. */
			SYSERR("select() error.");
			goto exit;
		} else if (rc == 0) {
			break; /* Timeout. */
		}

		ssize_t n = read(port->descriptor,
				 (char *)buf + nbytes, size - nbytes);
		if (n < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue; /* retry reading. */
			SYSERR("read() error.");
			goto exit;
		} else if (n == 0) {
			break;	/* EOF */
		}
		nbytes += n;
	}
	retval = 0;
	if (nbytes != size)
		DEBUG("Port read=%zu bytes, requested=%zu bytes", nbytes, size);

 exit:
	if (actual)
		*actual = nbytes;
	return retval;
}

int serial_write(const serial_port_t *port, const void *buf,
		size_t size, size_t *actual)
{
	size_t nbytes = 0;
	int retval = -1;

	while (nbytes < size) {
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(port->descriptor, &fds);

		int rc = select(port->descriptor + 1, NULL, &fds, NULL, NULL);
		if (rc < 0) {
			if (errno == EINTR)
				continue; /* Retry. */
			SYSERR("select() error.");
			goto exit;
		} else if (rc == 0) {
			break; /* Timeout. */
		}

		ssize_t n = write(port->descriptor,
				(const char *)buf + nbytes, size - nbytes);
		if (n < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue; /* Retry. */
			SYSERR("write() error.");
			goto exit;
		} else if (n == 0) {
			 break; /* EOF. */
		}
		nbytes += n;
	}

	/* Wait until all data has been transmitted. */
	while (tcdrain(port->descriptor) != 0) {
		if (errno != EINTR) {
			SYSERR("tcdrain() error.");
			goto exit;
		}
	}
	retval = 0;

 exit:
	if (actual)
		*actual = nbytes;
	return retval;
}

int serial_purge(const serial_port_t *port, direction_t direction)
{
	int flags = 0;

	switch (direction) {
	case DIRECTION_INPUT:
		flags = TCIFLUSH;
		break;
	case DIRECTION_OUTPUT:
		flags = TCOFLUSH;
		break;
	case DIRECTION_ALL:
	default:
		flags = TCIOFLUSH;
		break;
	}

	if (tcflush(port->descriptor, flags) != 0) {
		SYSERR("tcflush() failed");
		return -1;
	}
	return 0;
}

int serial_set_DTR(const serial_port_t *port, unsigned int level)
{
	unsigned long action;
	int value = TIOCM_DTR;

	INFO("DTR: value=%i", level);
	action = (level ? TIOCMBIS : TIOCMBIC);
	if (ioctl(port->descriptor, action, &value) != 0) {
		SYSERR("ioctl() failed.");
		return -1;
	}
	return 0;
}

int serial_set_RTS(const serial_port_t *port, unsigned int level)
{
	unsigned long action;
	int value = TIOCM_RTS;

	INFO("RTS: value=%i", level);
	action = (level ? TIOCMBIS : TIOCMBIC);
	if (ioctl(port->descriptor, action, &value) != 0) {
		SYSERR("ioctl() failed.");
		return -1;
	}
	return 0;
}

int serial_get_available(const serial_port_t *port, size_t *value)
{
	int bytes = 0;

	if (ioctl(port->descriptor, TIOCINQ, &bytes) != 0) {
		SYSERR("ioctl() failed.");
		return -1;
	}

	if (value)
		*value = bytes;
	return 0;
}

int serial_get_lines(const serial_port_t *port, unsigned int *value)
{
	unsigned int lines = 0;
	int status = 0;

	if (ioctl(port->descriptor, TIOCMGET, &status) != 0) {
		SYSERR("ioctl() failed.");
		return -1;
	}

	if (status & TIOCM_CAR)
		lines |= LINE_DCD;
	if (status & TIOCM_CTS)
		lines |= LINE_CTS;
	if (status & TIOCM_DSR)
		lines |= LINE_DSR;
	if (status & TIOCM_RNG)
		lines |= LINE_RNG;

	if (value)
		*value = lines;
	return 0;
}

int serial_sleep(const serial_port_t *port, unsigned int timeout)
{
	struct timespec ts;

	INFO("Sleeping serial port: %d", timeout);
	ts.tv_sec  = (timeout / 1000);
	ts.tv_nsec = (timeout % 1000) * 1000000;

	while (nanosleep(&ts, &ts) != 0) {
		if (errno != EINTR) {
			SYSERR("nanosleep() failed.");
			return -1;
		}
	}
	return 0;
}
