//
//  serialport.hpp
//  tikbd-capture
//
//  Created by Erik Piehl on 10.7.2022.
//

#ifndef serialport_hpp
#define serialport_hpp

#include <stdio.h>

int open_serial_port(const char *p);
int close_serial_port(int fd);

#endif /* serialport_hpp */
