//
//  serialport.cpp
//  tikbd-capture
//
//  Created by Erik Piehl on 10.7.2022.
//

#include "serialport.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

int open_serial_port(const char *p) {
  int fd = open(p, O_RDWR | O_NOCTTY | O_NDELAY);
  if(fd < 0)
    return -1;
  struct termios opt;
  tcgetattr(fd, &opt);
  cfsetispeed(&opt, B115200);
  cfsetospeed(&opt, B115200);

  //Settings from:
  //  https://github.com/kleydon/Mac-SerialPort-Cpp/blob/master/*%20Project/MacSerialPort/SerialPort/SerialPort.cpp
  //  and
  //  https://github.com/Marzac/rs232/blob/master/rs232-linux.c
  //
  opt.c_iflag &= ~(INLCR | ICRNL);
  opt.c_iflag |= IGNPAR | IGNBRK;
  opt.c_oflag &= ~(OPOST | ONLCR | OCRNL);
  opt.c_cflag &= ~(PARENB | PARODD | CSTOPB | CSIZE | CRTSCTS);
  opt.c_cflag |= CLOCAL | CREAD | CS8;
  opt.c_lflag &= ~(ICANON | ISIG | ECHO);
  opt.c_cc[VTIME] = 1;
  opt.c_cc[VMIN]  = 0;
  if(tcsetattr(fd, TCSANOW, &opt) < 0) {
    close(fd);
    return -2;
  }
  return fd;
}

int close_serial_port(int fd) {
  return close(fd);
}
