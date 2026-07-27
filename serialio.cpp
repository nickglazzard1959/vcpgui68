// serialio.cpp - Helper program to serial line i/o from Algol 68 Genie
// ====================================================================
//
// 1. Configures a serial port (e.g. /dev/ttyACM0) for "transparent" passage
//    of characters at 8,N,1 and a specified baud rate.
// 2. Anything received on stdin is sent to the serial port.
// 3. Anything received from the serial port is sent to stdout.
//
// The program is intended to be run using "execve child pipe()" from a68g.
// Reading from and writing to the pipes created by that function will send
// and receive characters to/from the serial attached device.
//
// This is intended for controlling homemade electronics controlled by
// Arduinos. It has only been tested for that purpose.

// Pre-requisites:
//   CLI11.hpp (https://github.com/CLIUtils/CLI11, Releases page).
//
// Build:
//   g++ -O0 -g serialio.cpp -o serialio
//
// Nick Glazzard 2026
// ------------------

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/select.h>

#include "CLI11.hpp" // Gigantic, but useful.

typedef std::map<std::string, speed_t> BAUD_MAP;

int main( int ArgCount, char **Args )
{
  static const int BUFLEN = 256;
  unsigned char buf[BUFLEN+1];

  // Map standard baud rate strings to speed constants.
  BAUD_MAP baud_map
    {
      {"0", B0},
      {"50",B50},           {"75",B75},           {"110",B110},       {"134",134},
      {"150",B150},         {"200",B200},         {"300",B300},       {"600",B600}, 
      {"1200",B1200},       {"1800",B1800},       {"2400",B2400},     {"4800",B4800}, 
      {"9600",B9600},       {"19200",B19200},     {"38400",B38400},   {"57600",B57600}, 
      {"115200",B115200},   {"230400",B230400},   {"460800",B460800}, {"500000",B500000},
      {"576000",B576000},   {"921600",B921600},   {"1000000",B1000000},
      {"1152000",B1152000}, {"1500000",B1500000}, {"2000000",B2000000}
    };

  // Default arguments.
  std::string serial_port_name = "/dev/ttyACM0";
  std::string baud_rate = "9600";

  // Parse any command line arguments.
  CLI::App app{"Display surface program for a68g"};
  Args = app.ensure_utf8(Args);

  app.add_option("-d,--device", serial_port_name, "Serial port device name (e.g. /dev/ttyACM0).");
  app.add_option("-b,--baud", baud_rate, "Serial Baud rate to use.");

  CLI11_PARSE(app, ArgCount, Args);

  // Validate and prepare to set the baud rate.
  if( baud_map.find(baud_rate) == baud_map.end() ){
    fprintf(stderr, "Non-standard or garbled baud rate: %s\n", baud_rate.c_str());
    return 1;
  }
  speed_t baud_speed = baud_map[baud_rate];
  
  // Open the serial port.
  int serial_port = open(serial_port_name.c_str(), O_RDWR);
  if( serial_port < 0 ){
    fprintf(stderr, "Failed to open serial port: %s, error: %s\n",
            serial_port_name.c_str(), strerror(errno));
    return 1;
  }

  // Get the current attributes on that serial port.
  struct termios tty;
  if( tcgetattr(serial_port, &tty) != 0 ){
    fprintf(stderr, "Failed to get serial port attributes, error: %s\n",
            strerror(errno));
    return 2;
  }

  // Modify the attributes to match the expectations of the device we are talking to.
  tty.c_cflag &= ~PARENB;  // Disable parity.
  tty.c_cflag &= ~CSTOPB;  // 1 stop bit only.
  tty.c_cflag &= ~CSIZE;   // Clear data size bits.
  tty.c_cflag |= CS8;      // 8 data bits.
  tty.c_cflag &= ~CRTSCTS; // No RTS/CTS flow control.
  tty.c_cflag |= CREAD|CLOCAL; // Ignore modem control lines. Enable receiver.

  // Modify attributes so the port is "transparent".
  tty.c_lflag &= ~ICANON;  // Turn off canonical processing.
  tty.c_lflag &= ~ECHO;    // Turn off echo.
  tty.c_lflag &= ~ECHOE;   // Turn off erasure echo.
  tty.c_lflag &= ~ECHONL;  // Turn off newline echo.
  tty.c_lflag &= ~ISIG;    // Turn off interpretation of INTR, SUSP and QUIT.

  // Including special input character handling ...
  tty.c_iflag &= ~(IXON|IXOFF|IXANY); // Turn off software flow control.
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

  // And output character handling ...
  tty.c_oflag &= ~OPOST;   // Turn off any special intepretation of output characters.
  tty.c_oflag &= ~ONLCR;   // Do not convert NL to CR NL.

  // Timeouts. We will use select() so we don't want any blocking on read (I think).
  tty.c_cc[VTIME] = 0;
  tty.c_cc[VMIN] = 0;

  // Set the baud rates.
  cfsetispeed(&tty, baud_speed);
  cfsetospeed(&tty, baud_speed);

  // Set these modified settings on the serial port.
  if( tcsetattr(serial_port, TCSANOW, &tty) != 0 ){
    fprintf(stderr, "Failed to set serial port attributes, error: %s\n",
            strerror(errno));
    return 3;
  }
  
  // Setup an initial timeout for select.
  struct timeval ts;
  ts.tv_sec = 1; // 1 second.
  ts.tv_usec = 0;

  // Loop forever-ish ...
  while (1) {
    
    // Setup select() ...
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(serial_port, &fds); // serial port ...
    FD_SET(0, &fds); // stdin ...

    // Wait for data from either the serial device or stdin.
    int nready = select(serial_port+1, &fds, (fd_set*)0, (fd_set*)0, &ts);
    if( nready < 0 ){
      fprintf(stderr, "Failed to select(), error: %s\n",
              strerror(errno));
      return 4;
    }

    // Nothing yet. Wait another second.
    else if( nready == 0 ){
      ts.tv_sec = 1;
      ts.tv_usec = 0;
    }

    // From serial port ...
    else if( FD_ISSET(serial_port, &fds) ){

      // Read all available bytes.
      ssize_t n_recv = read(serial_port, buf, BUFLEN);
      if( n_recv < 0 ){
        fprintf(stderr, "Failed to read(serial_port), error: %s\n",
                strerror(errno));
        return 9;
      }

      // EOF on serial_port?
      else if( n_recv == 0 ){
        break; 
      }

      // Send everything that has been read from serial_port to stdout.
      else{
        ssize_t n_put = write(fileno(stdout), buf, n_recv);
        if( n_put < 0 ){
          fprintf(stderr, "Failed to write(serial_port), error: %s\n",
                  strerror(errno));
          return 10;
        }
        if( n_put < n_recv ){
          fprintf(stderr, "Partial write(serial_port), hmm ... \n");
          return 11;
        } 
      } // serial_port -> stdout
    } // got data from serial_port

    // From stdin ...
    else if( FD_ISSET(0, &fds) ){

      // Read all available bytes.
      ssize_t n_send = read(fileno(stdin), buf, BUFLEN);
      if( n_send < 0 ){
        fprintf(stderr, "Failed to read(stdin), error: %s\n",
                strerror(errno));
        return 5;
      }

      // EOF on stdin?
      else if( n_send == 0 ){
        break; 
      }

      // Send everything that has been read from stdin to serial_port.
      else{
        ssize_t n_put = write(serial_port, buf, n_send);
        if( n_put < 0 ){
          fprintf(stderr, "Failed to write(serial_port), error: %s\n",
                  strerror(errno));
          return 6;
        }
        if( n_put < n_send ){
          fprintf(stderr, "Partial write(serial_port), hmm ... \n");
          return 7;
        }
      } // stdin -> serial_port
    } // got data from stdin

  } // Loop forever-ish.

  // Close serial port and return success condition code.
  close(serial_port);
  return 0;
}
