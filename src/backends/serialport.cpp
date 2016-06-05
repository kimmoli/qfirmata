// QFirmata - a Firmata library for QML
//
// Copyright 2016 - Calle Laakkonen
// 
// QFirmata is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// QFirmata is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

#include "serialport.h"

#include <QPointer>
#include <QDebug>


#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/serial.h>

struct SerialFirmata::Private {
    int baudRate;
    int port_fd;
	QString device;
    struct termios settings;

	Private()
    : baudRate(57600), port_fd(0)
	{ }
};

SerialFirmata::SerialFirmata(QObject *parent)
	: FirmataBackend(parent), d(new Private)
{
    polltimer = new QTimer();
    connect(polltimer, SIGNAL(timeout()), this, SLOT(pollInput()));
    polltimer->setInterval(100);
    polltimer->setSingleShot(false);
    polltimer->start();
}

SerialFirmata::~SerialFirmata()
{
	delete d;
}

QString SerialFirmata::device() const
{
	return d->device;
}

void SerialFirmata::setDevice(const QString &device)
{
    struct termios settings_orig;
    if(device != d->device)
    {
		d->device = device;
        if(device.isEmpty())
        {
            d->port_fd = 0;
			setStatusText(QStringLiteral("Device not set"));
        }
        else
        {
            struct serial_struct kernel_serial_settings;
            int bits;
            d->port_fd = open(d->device.toLatin1().constData(), O_RDWR | O_NOCTTY | O_NONBLOCK);
            if (d->port_fd < 0)
            {
                if (errno == EACCES)
                {
                    qWarning() << "Unable to access " << d->device << ", insufficient permission";
                }
                else if (errno == EISDIR)
                {
                    qWarning() << "Unable to open " << d->device << ", Object is a directory, not a serial port";
                }
                else if (errno == ENODEV || errno == ENXIO)
                {
                    qWarning() << "Unable to open " << d->device << ", Serial port hardware not installed";
                }
                else if (errno == ENOENT)
                {
                    qWarning() <<"Unable to open " << d->device << ", Device name does not exist";
                }
                else
                {
                    qWarning() << "Unable to open " << d->device <<  ", " << strerror(errno);
                }
                return;
            }
            if (ioctl(d->port_fd, TIOCMGET, &bits) < 0)
            {
                close(d->port_fd);
                qWarning() << "Unable to query serial port signals";
                return;
            }
            bits &= ~(TIOCM_DTR | TIOCM_RTS);
            if (ioctl(d->port_fd, TIOCMSET, &bits) < 0)
            {
                close(d->port_fd);
                qWarning() << "Unable to control serial port signals";
                return;
            }
            if (tcgetattr(d->port_fd, &settings_orig) != 0)
            {
                close(d->port_fd);
                qWarning() << "Unable to query serial port settings (perhaps not a serial port)";
                return;
            }
            memset(&d->settings, 0, sizeof(d->settings));
            d->settings.c_iflag = IGNBRK | IGNPAR;
            d->settings.c_cflag = CS8 | CREAD | HUPCL | CLOCAL;
            setBaudRate(d->baudRate);

            if (ioctl(d->port_fd, TIOCGSERIAL, &kernel_serial_settings) == 0)
            {
                kernel_serial_settings.flags |= ASYNC_LOW_LATENCY;
                ioctl(d->port_fd, TIOCSSERIAL, &kernel_serial_settings);
            }
            tcflush(d->port_fd, TCIFLUSH);
		}

        qDebug() << device << "opened succesfully";
        setAvailable(true);
        setStatusText(QStringLiteral("Serial port opened"));

		emit deviceChanged(device);
	}
}

int SerialFirmata::baudRate() const
{
	return d->baudRate;
}

void SerialFirmata::setBaudRate(int baudRate)
{
    d->baudRate = baudRate;
    if(d->port_fd)
    {
        speed_t spd;
        switch (baudRate) {
            case 230400:    spd = B230400;	break;
            case 115200:    spd = B115200;	break;
            case 57600:     spd = B57600;	break;
            case 38400:     spd = B38400;	break;
            case 19200:     spd = B19200;	break;
            case 9600:      spd = B9600;	break;
            case 4800:      spd = B4800;	break;
            case 2400:      spd = B2400;	break;
            case 1800:      spd = B1800;	break;
            case 1200:      spd = B1200;	break;
            case 600:       spd = B600;     break;
            case 300:	spd = B300;	break;
            case 200:	spd = B200;	break;
            case 150:	spd = B150;	break;
            case 134:	spd = B134;	break;
            case 110:	spd = B110;	break;
            case 75:	spd = B75;	break;
            case 50:	spd = B50;	break;
            default: qWarning() << "unsupported baudrate" << baudRate; return;
        }
        cfsetospeed(&d->settings, spd);
        cfsetispeed(&d->settings, spd);
        if (tcsetattr(d->port_fd, TCSANOW, &d->settings) < 0)
        {
            qWarning() << "failed to set baudrate" << baudRate;
            return;
        }
    }

    qDebug() << "baudrate changed to" << baudRate;

    emit baudRateChanged(baudRate);
}

void SerialFirmata::writeBuffer(const uint8_t *buffer, int len)
{
    if(!d->port_fd)
    {
		qWarning() << "Device" << d->device << "not open!";
		return;
	}

    QByteArray aa;
    int i;
    for (i=0 ; i<len; i++)
        aa.append(buffer[i]);

    qDebug() << "write" << len << "bytes:" << aa.toHex();

    int n, written=0;
    fd_set wfds;
    struct timeval tv;

    while (written < len)
    {
        n = write(d->port_fd, buffer + written, len - written);
        if (n < 0 && (errno == EAGAIN || errno == EINTR))
            n = 0;

        if (n < 0)
        {
            qWarning() << "write failed";
            return;
        }
        if (n > 0)
        {
            written += n;
        }
        else
        {
            tv.tv_sec = 10;
            tv.tv_usec = 0;
            FD_ZERO(&wfds);
            FD_SET(d->port_fd, &wfds);
            n = select(d->port_fd+1, NULL, &wfds, NULL, &tv);
            if (n < 0 && errno == EINTR)
                n = 1;
            if (n <= 0)
            {
                qWarning() << "write failed ??";
                return;
            }
        }
    }

    if(written != len)
    {
        qWarning() << d->device << "error while writing buffer";
	}
}

void SerialFirmata::pollInput()
{
    if (!d->port_fd)
        return;

    if (waitInput() > 0)
        doRead();
}

void SerialFirmata::doRead()
{
    int n, bits;
    char buf[1024];

    n = read(d->port_fd, buf, sizeof(buf));

    if (n < 0 && (errno == EAGAIN || errno == EINTR))
    {
        qWarning() << "error reading";
        return;
    }
    if (n == 0 && ioctl(d->port_fd, TIOCMGET, &bits) < 0)
    {
        qWarning() << "TIOCMGET";
        return;
    }

    qDebug() << "read" << n << "bytes";

    bytesRead(buf, n);
}

int SerialFirmata::waitInput()
{
    int msec = 40;
    fd_set rfds;
    struct timeval tv;

    tv.tv_sec = msec / 1000;
    tv.tv_usec = (msec % 1000) * 1000;
    FD_ZERO(&rfds);
    FD_SET(d->port_fd, &rfds);

    return select(d->port_fd+1, &rfds, NULL, NULL, &tv);
}

void SerialFirmata::closeDevice(void)
{
    close(d->port_fd);
}

