#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int write_string_usbtmc( int fd, const char* string )
//---------------------------------------------------
// Write only the characters of string to a usbtmc device..
// No new line or any other terminator.
// Return 0 if OK, else errno.
{
  size_t length = strlen(string);
  if( write(fd, string, length) != length ){
    perror("write_string_usbtmc()");
    return errno;
  }
  else
    return 0;
}

int read_string_usbtmc( int fd, char* string, int length )
//--------------------------------------------------------
// Read a string from the usbtmc device. It seems this must be
// done 1 character at a time, ending when '\n' is read. Trying
// to read more data at a time "works" once, ending with a
// "connection timed out error" and subsequent reads do not work.
// Return the number of characters read. Return 0 on error or EOF.
{
  char in1;
  int i, retval = -1;
  ssize_t n_read = 0;

  // Loop over size of output buffer.
  for( i=0; i<(length-1); i++ ){
    n_read = read(fd, &in1, 1);

    // EOF?
    if( n_read == 0 ){
      fprintf(stderr, "EOF\n");
      string[i] = 0;
      retval = 0;
      break;
    }

    // Error?
    else if( n_read < 0 ){
      perror("read_string_usbtmc()");
      string[i] = 0;
      retval = 0;
      break;
    }

    // Add a character unless \n. In that case, terminate string and exit.
    else{
      //fprintf(stderr, "[%c]", in1);
      if( in1 == '\n' ){
        string[i] = 0;
        retval = i;
        break;
      }
      else
        string[i] = in1;
    }
  }

  return retval;
}

int main( int argc, char** argv )
{
  int fd = open("/dev/usbtmc2", O_RDWR);
  if( fd < 0 ){
    fprintf(stderr, "Device open failed.\n");
    perror("Device open");
  }

  char result[4000];
  int n_result = 0;

  if( write_string_usbtmc(fd, "*IDN?" ) != 0 )exit(1);
  if( (n_result = read_string_usbtmc(fd, result, 4000)) == 0 )exit(2);
  fprintf(stderr, "[%s]\n", result);

  for(int j=0; j<20; j++ ){
    if( write_string_usbtmc(fd, ":MEAS:VOLT:DC?" ) != 0 )exit(3);
    if( (n_result = read_string_usbtmc(fd, result, 4000)) == 0 )exit(4);
    fprintf(stderr, "[%s]\n", result);
  }

  close(fd);
  return 0;
}
