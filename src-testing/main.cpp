/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include <iostream>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "libs/predict/predict.h"

#include <sys/time.h>
#include <thread>
#include <cmath>

int set_interface_attribs(int fd, int speed, int parity)
{
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0)
    {
        // error_message("error %d from tcgetattr", errno);
        return -1;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK; // disable break processing
    tty.c_lflag = 0;        // no signaling chars, no echo,
                            // no canonical processing
    tty.c_oflag = 0;        // no remapping, no delays
    tty.c_cc[VMIN] = 0;     // read doesn't block
    tty.c_cc[VTIME] = 5;    // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);   // ignore modem controls,
                                       // enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        // error_message("error %d from tcsetattr", errno);
        return -1;
    }
    return 0;
}

void set_blocking(int fd, int should_block)
{
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0)
    {
        // error_message("error %d from tggetattr", errno);
        return;
    }

    tty.c_cc[VMIN] = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5; // 0.5 seconds read timeoutngl bu

    // if (tcsetattr(fd, TCSANOW, &tty) != 0)
    // error_message("error %d setting term attributes", errno);
}

int main(int argc, char *argv[])
{
    char *portname = "/dev/ttyUSB0";
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);

    if (fd < 0)
    {
        printf("error %d opening %s: %s", errno, portname, strerror(errno));
        return -1;
    }

    set_interface_attribs(fd, B9600, 0); // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking(fd, 0);                 // set no blocking

    // Setup predict
    predict_orbital_elements_t *satellite_object;
    predict_position satellite_orbit;
    predict_observer_t *predict_observer;
    predict_observation satellite_observation;
    predict_observer = predict_create_observer("QTH", 48.7814 / 57.29578, 1.8206 / 57.29578, 20);

    satellite_object = predict_parse_tle("1 43609U 18068A   22077.00247406  .00000079  00000+0  43302-4 0  9997", "2 43609  98.4660 152.7294 0013578  49.1763 311.0587 14.34243124184603");

    char command[1000];
    char reply[1000];

    float az = 0, el = 0;

    while (1)
    {
        uint64_t millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        double current_time = millis / 1000.0;

        predict_orbit(satellite_object, &satellite_orbit, predict_to_julian_double(current_time));
        predict_observe_orbit(predict_observer, &satellite_orbit, &satellite_observation);

        // std::cout << current_time << " " << satellite_observation.azimuth * 57.29578 << " " << satellite_observation.elevation * 57.29578 << " POS " << az << " " << el << std::endl;
        float d_az = fabs(float(satellite_observation.azimuth * 57.29578) - az);
        float d_el = fabs(float(satellite_observation.elevation * 57.29578) - el);
        printf("%f %.4f %.4f POS %.4f %.4f ABS %.4f %.4f\n", current_time, satellite_observation.azimuth * 57.29578, satellite_observation.elevation * 57.29578, az, el, d_az, d_el);

        if (satellite_observation.elevation < 0)
        {
            satellite_observation.elevation = 0 / 57.29578;
            satellite_observation.azimuth = 0 / 57.29578;
        }

        int size = sprintf(command, "AZ%f EL%f\n", satellite_observation.azimuth * 57.29578, satellite_observation.elevation * 57.29578);
        write(fd, command, size);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Read position
        size = sprintf(command, "AZ EL\n");
        write(fd, command, size);
        int n = read(fd, reply, 1000);

        int r = sscanf(reply, "AZ%f EL%f\n", &az, &el);
        if (r == 2)
        {
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}