// tmcusbio.cpp - Helper program to communicate with USB TMC devices from Algol 68 Genie
// =====================================================================================
//
// Communicate with a USB TMC device in a way compatible with pipes.
// Commands are read from stdin. If a command string begins with
// P, treat the rest of it as "put only" with no response expected.
// Write status to stdout: *ok if OK, ?<error message> on error.
// If a command string begins with Q, treat the rest of it as a "query"
// with a response expected. Also write status to stdout along with response
// if there was no error.: *<response> if OK, ?<error message> on error.
// The program is intended to be run using "execve child pipe()" from a68g.
//
// Devices are identified by "vendor-id" (VID) and "product-id" (PID) as listed by lsusb. E.g.
//      Bus 002 Device 018: ID 1ab1:09c4 Rigol Technologies DM3000 SERIES
// Here, VID = 1ab1 and PID = 09c4.
// To allow the device to be accessed without root privileges (sudo), a "udev rule"
// file must be created for the device. To do this:
// 1. Create (as sudo) /etc/udev/rules.d/99-libusb.rules  ("99" can be any number > 50).
// 2. Add this text:
//      SUBSYSTEM=="usb", ATTRS{idVendor}=="<VID>", ATTRS{idProduct}=="<PID>", GROUP="plugdev", MODE="0660"
// E.g. SUBSYSTEM=="usb", ATTRS{idVendor}=="1ab1", ATTRS{idProduct}=="09c4", GROUP="plugdev", MODE="0660"
//    (note that using "uaccess" instead did not work for me with Debian 12).
// 3. Reload and apply the rules:
//      sudo udevadm control --reload-rules
//      sudo udevadm trigger
// These rules should be followed each time the device is seen again (e.g. plugged in).
//
// *********************************
// Background notes.
// The use of libusb rather than the kernel usbtmc devices & driver was because of
// IO errors with usbtmc that "suddenly started" after a long period of correct
// behaviour and that occurred "sometimes" (but, always, eventually -- or even very quickly).
// This happened with either Algol 68 or C++ code. It happened with kernels 5.10 and 6.0.
// After writing this code, it became all too clear that it happened with libusb too!
// Detailed debugging (allowed by using libusb) showed that IO errors were associated with
// low-level error -71, which is EPROTO (or maybe ETIME, EILSEQ, ECOMM or ENOSR). These are never
// supposed to occur with healthy hardware, AFAICS.
// It turned out that assuming the *hardware was working correctly* was a mistake in this case.
// What seems to fix error -71 according to those who have run into it, is *powering down* the
// host computer and starting again. And yes, this does seem to resolve the problem.
// So usbtmc would probably work fine too after a power down ... In a way, this whole "move to
// libusb" was unnecessary. OTOH, it did allow detailed diagnosis and it has all been educational.
// *********************************

// Pre-requisites;
//   CLI11.hpp (https://github.com/CLIUtils/CLI11, Releases page).
//   libusb (sudo apt install libusb-1.0-0-dev).
//
// Build:
//   g++ -O0 -g -Wall -o tmcusbio tmcusbio.cpp -lusb-1.0
//
// Nick Glazzard 2026
//-------------------

#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "CLI11.hpp" // Gigantic, but useful.

static const int BUFFER_SIZE = 2048; // Should be multiple of wMaxPacketSize and divisible by 4. 2048 is "safe".
static const int HDR_SIZE = 12;
static const int BUFFER_HDR_SIZE = BUFFER_SIZE + HDR_SIZE;
static const int ERROR_SIZE = 256;

// Encapsulate communications with a USB TMC device using libusb.
class usbtmcio
{
public:
  libusb_context* ctx;
  libusb_device_handle* handle;
  libusb_device* device;
  uint16_t vendor_id;
  uint16_t product_id;
  int32_t current_config;
  uint8_t out_endpoint;
  uint8_t in_endpoint;
  uint8_t transaction_no;
  uint32_t timeout;
  uint16_t in_chunk_size;
  uint16_t out_chunk_size;
  struct libusb_config_descriptor *config;

  char last_error_string[ERROR_SIZE];
  uint8_t header[HDR_SIZE];
  uint8_t buffer[BUFFER_HDR_SIZE];
  bool ok;

public:
  usbtmcio(uint16_t u_vendor_id, uint16_t u_product_id, bool debug_mode )
  //---------------------------------------------------------------------
  // Constructor. Initialise libusb. Open the specified device.
  {
    // Initialise all member variables.
    ctx = NULL;
    handle = NULL;
    device = NULL;
    vendor_id = u_vendor_id;
    product_id = u_product_id;
    current_config = -1;
    out_endpoint = 0;
    in_endpoint = 0;
    transaction_no = 1;
    timeout = 5000;
    in_chunk_size = 0;
    out_chunk_size = 0;
    memset(last_error_string, 0, sizeof(last_error_string));
    
    // Assume it didn't work. Usually a safe assumption ... :-)
    ok = false;
    
    // Initialise libusb.
    if( libusb_init(&ctx) < 0 ){
      perror("libusb_init() failed");
    }
    else{

      // Optionally turn on very extensive debugging output.
      if( debug_mode )
        libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);

      // Open the desired device.
      handle = libusb_open_device_with_vid_pid(ctx, vendor_id, product_id);
      if( NULL == handle ){
        perror("libusb_open_device_with_vid_pid() failed");
        libusb_exit(ctx);
        ctx = NULL;
      }

      // Open worked ...
      else{
        // Detach the kernel driver (usbtmc) if it is active.
        if( libusb_kernel_driver_active(handle, 0) == 1 )
          libusb_detach_kernel_driver(handle, 0);

        // Claim the interface.
        libusb_claim_interface(handle, 0);

        // Get the current configuration number, although we do nothing with it ATM.
        libusb_get_configuration(handle, &current_config);

        // Look at the current configuration details and try to identify
        // bulk transfer IN_ENDPOINT and OUT_ENDPOINT to read and write to the device.
        struct libusb_config_descriptor *config;
        device = libusb_get_device(handle);
        libusb_get_active_config_descriptor(device, &config);
        
        for( int i=0; i<config->bNumInterfaces; i++){
          const struct libusb_interface *inter = &config->interface[i];
          for( int j=0; j<inter->num_altsetting; j++ ){
            const struct libusb_interface_descriptor *inter_desc = &inter->altsetting[j];
            for( int k=0; k<inter_desc->bNumEndpoints; k++ ){
              const struct libusb_endpoint_descriptor *ep_desc = &inter_desc->endpoint[k];
              uint8_t address = ep_desc->bEndpointAddress;
              uint8_t attribs = ep_desc->bmAttributes;
              uint16_t chunk_size = ep_desc->wMaxPacketSize;
              if( address & LIBUSB_ENDPOINT_IN ){
                if( in_endpoint == 0 && attribs == 2 ){
                  in_endpoint = address;
                  in_chunk_size = chunk_size;
                  if( (BUFFER_SIZE % chunk_size) != 0 )
                    fprintf(stderr,
                            "TMCUSBIO: Warning: BUFFER_SIZE is not a multiple of in wMaxPacketSize for this device.\n");
                }
              } // first in endpoint
              else{
                if( out_endpoint == 0 && attribs == 2 ){
                  out_endpoint = address;
                  out_chunk_size = chunk_size;
                  if( (BUFFER_SIZE % chunk_size) != 0 )
                    fprintf(stderr,
                            "TMCUSBIO: Warning: BUFFER_SIZE is not a multiple of out wMaxPacketSize for this device.\n");
                } // first out endpoint
              } // in endpoint or out endpoint?
            } // endpoints
          } // alt settings
        } // interfaces

        // We must have in_endpoint and out_endpoint now. If so, set ok true.
        if( (in_endpoint != 0) && (out_endpoint != 0) )
          ok = true;
        
      } // device open worked.
    } // libusb initialisation worked.
  };

  ~usbtmcio()
  //---------
  // Destructor. Clean up libusb.
  {
    if( NULL != handle ){
      libusb_release_interface(handle, 0);
      libusb_close(handle);      
    }
    
    if( NULL != ctx )
      libusb_exit(ctx);
  };

  void set_timeout( unsigned int ms )
  //---------------------------------
  // Set transaction timeout in milliseconds.
  {
    timeout = (ms < 100000) ? ms : 100000;
  };
  
  void bump_transaction()
  //---------------------
  // Increment the transaction number.
  // This must start at 1, not zero, incl. after wrap around.
  {
    ++transaction_no;
    if( transaction_no == 0 )
      ++transaction_no;
  };

  void make_header( uint32_t data_length, bool outgoing, bool with_eom )
  //--------------------------------------------------------------------
  // Construct a message header to put/get data_length bytes.
  // If outgoing, the message is to send data. Otherwise, it is a message
  // to request input data. The latter must be sent before trying to read data.
  {
    memset(header, 0, HDR_SIZE);
    header[0] = (outgoing) ? 1 : 2 ; // DEV_DEP_MSG_OUT, [REQUEST_]DEV_DEP_MSG_IN (not defined in libusb.h).
    header[1] = transaction_no;
    header[2] = ~transaction_no;
    header[4] = (uint8_t)(data_length & 0xFF); // Data payload length split into bytes.
    header[5] = (uint8_t)((data_length >> 8) & 0xFF);
    header[6] = (uint8_t)((data_length >> 16) & 0xFF);
    header[7] = (uint8_t)((data_length >> 24) & 0xFF);
    header[8] = (with_eom) ? 1 : 0 ;
  };

  int put_only( const char* string )
  //--------------------------------
  // Send string to the device, not expecting a response.
  // Return -ve status if the transfer fails (and issue an error message).
  // For non-libusb errors, return LIBUSB_ERROR_OTHER (-99).
  {
    if( ! ok ){
      strncpy(last_error_string, "TMCUSBIO_INIT_FAILED", ERROR_SIZE);
      fprintf(stderr, "TMCUSBIO, put_only(): error: %s\n", 
              last_error_string);
      return LIBUSB_ERROR_OTHER;
    }

    // Validate string.
    uint32_t n_chars = (uint32_t)strlen(string);
    if( 0 == n_chars ){
      strncpy(last_error_string, "TMCUSBIO_EMPTY_STRING_ARG", ERROR_SIZE);
      fprintf(stderr, "TMCUSBIO, put_only(): error: %s\n", 
              last_error_string);
      return LIBUSB_ERROR_OTHER;
    }
    if( n_chars > BUFFER_SIZE ){
      strncpy(last_error_string, "TMCUSBIO_STRING_ARG_TOO_LONG", ERROR_SIZE);
      fprintf(stderr, "TMCUSBIO, put_only(): error: %s\n", 
              last_error_string);
      return LIBUSB_ERROR_OTHER;
    }

    // Construct a header to send the string.
    make_header(n_chars, true, true);
    memcpy(buffer, header, HDR_SIZE);
    memcpy(buffer+HDR_SIZE, string, n_chars);

    // Send to the device.
    int transferred = 0;
    int status = libusb_bulk_transfer(handle,
                                      out_endpoint,
                                      buffer, HDR_SIZE+n_chars,
                                      &transferred,
                                      timeout);
    if( 0 != status ){
      strncpy(last_error_string, libusb_error_name(status), ERROR_SIZE);
      fprintf(stderr, "TMCUSBIO, put_only(): error: %s\n... %s\n", 
              last_error_string,
              libusb_strerror(status));
      return status;
    }

    // Bump the transaction count.
    bump_transaction();
    
    return LIBUSB_SUCCESS;
  };

  int query_internal( const char* string, char* response, size_t max_length, uint32_t& response_length )
  //----------------------------------------------------------------------------------------------------
  // Send string to the device, expecting a response. Get the response.
  // The response buffer must be able to receive at least max_length bytes.
  // The number of bytes transferred to the response buffer is returned in response_length.
  // This transfers data in multiple chunks of BUFFER_SIZE (or less) and *should* cope with
  // large data transfers, if response and max_length are larger than any expected transfer.
  // This has not yet been tested, though.
  // Return a libusb status if any transfer fails (and issue an error message).
  {
    if( ! ok ){
      strncpy(last_error_string, "TMCUSBIO_INIT_FAILED", ERROR_SIZE);
      fprintf(stderr, "TMCUSBIO, query(): error: %s\n", 
              last_error_string);
      return LIBUSB_ERROR_OTHER;
    }
    response_length = 0;

    // Validate string.
    uint32_t n_chars = (uint32_t)strlen(string);
    if( 0 == n_chars ){
      strncpy(last_error_string, "TMCUSBIO_EMPTY_STRING_ARG", ERROR_SIZE);
      fprintf(stderr, "TMCUSBIO, query(): error: %s\n", 
              last_error_string);
      return LIBUSB_ERROR_OTHER;
    }
    if( n_chars > BUFFER_SIZE ){
      strncpy(last_error_string, "TMCUSBIO_STRING_ARG_TOO_LONG", ERROR_SIZE);
      fprintf(stderr, "TMCUSBIO, query(): error: %s\n", 
              last_error_string);
      return LIBUSB_ERROR_OTHER;
    }    

    // Construct a header to send the string.
    make_header(n_chars, true, true);
    memcpy(buffer, header, HDR_SIZE);
    memcpy(buffer+HDR_SIZE, string, n_chars);

    // Send to the device.
    int transferred = 0;
    int status = libusb_bulk_transfer(handle,
                                      out_endpoint,
                                      buffer, HDR_SIZE+n_chars,
                                      &transferred,
                                      timeout);
    if( 0 != status ){
      strncpy(last_error_string, libusb_error_name(status), ERROR_SIZE);
      fprintf(stderr, "query(): command out error (1): %s\n... %s\n", 
              last_error_string,
              libusb_strerror(status));
      return status;
    }

    // Request and read the response. Do this in chunks of BUFFER_SIZE or less.
    // Note that each chunk transfer begins with a 12 byte header, I believe.
    // Behind the scenes, data is transferred in blocks of wMaxPacketSize and only the
    // first block has a header, but that is hidden from this level.
    uint32_t bytes_read = 0;           // Bytes placed in output buffer.
    uint32_t bytes_left = max_length;  // Bytes left to fill in output buffer.
    uint32_t bytes_chunk = bytes_left; // Bytes potentially in a chunk (clamped later to BUFFER_SIZE or less).

    // Loop, Keep going until we get less than BUFFER_SIZE bytes returned from device OR 
    // we get EOM OR we have filled the output buffer. This last is NOT a desirable condition.
    while( bytes_left > 0 ){    

      // Find the number of bytes to transfer in this chunk. Limit to BUFFER_SIZE.
      bytes_chunk = (bytes_left > BUFFER_SIZE) ? BUFFER_SIZE : bytes_left ;
      
      // Construct a request response header.
      make_header(bytes_chunk, false, false);

      // Send the header (no data payload).
      status = libusb_bulk_transfer(handle,
                                    out_endpoint,
                                    header,
                                    HDR_SIZE,
                                    &transferred,
                                    timeout);
      if( 0 != status ){
        strncpy(last_error_string, libusb_error_name(status), ERROR_SIZE);
        fprintf(stderr, "query(): request response error (2): %s\n... %s\n", 
                last_error_string,
                libusb_strerror(status));
        return status;
      }

      // Read from the device.
      memset(buffer, 0, BUFFER_SIZE );
      status = libusb_bulk_transfer(handle,
                                    in_endpoint,
                                    buffer,
                                    BUFFER_SIZE,
                                    &transferred,
                                    timeout);
      if( 0 != status ){
        strncpy(last_error_string, libusb_error_name(status), ERROR_SIZE);
        fprintf(stderr, "query(): read response error (3): %s\n... %s\n", 
                last_error_string,
                libusb_strerror(status));
        return status;
      }

      // Check for pathological failues, just in case.
      if( transferred < HDR_SIZE ){
        strncpy(last_error_string, "TMCUSBIO_RESPONSE_SHORTER_THAN_HEADER", ERROR_SIZE);
        fprintf(stderr, "TMCUSBIO, query(): error: %s\n", 
                last_error_string);
        return LIBUSB_ERROR_OTHER;
      }

      // Get the number of bytes returned by the device. This is expected to be 12 bytes less than "transferred" bytes.
      memcpy(header, buffer, HDR_SIZE);
      uint32_t hdr_ret_bytes = header[4] + (header[5] << 8) + (header[6] << 16) + (header[7] << 24);
      
      //fprintf(stderr, "SAME? transferred = %u, hdr_ret_bytes = %u, eom = %u\n",
      //        transferred, hdr_ret_bytes, header[8]);

      // Insert this chunk into the user supplied return buffer.
      if( hdr_ret_bytes > 0 ){
        size_t last_output_byte = bytes_read + hdr_ret_bytes;
        size_t copy_length = (last_output_byte >= max_length) ? max_length - bytes_read - 1 : hdr_ret_bytes ;
        memcpy(response+bytes_read, buffer+HDR_SIZE, copy_length);
      }
      
      // Update byte counts.
      bytes_read += hdr_ret_bytes;
      bytes_left -= hdr_ret_bytes;

      // If transfer is less than BUFFER_SIZE (incl. bytes_chunk) or EOM, break.
      if( hdr_ret_bytes < BUFFER_SIZE )break;
      if( header[8] != 0 )break;

    } // while bytes_left > 0

    // fprintf(stderr, "LEFT? bytes_left = %u\n", bytes_left);

    // Bump the transaction count.
    bump_transaction();

    // Return the query result.
    response_length = bytes_read;
    return LIBUSB_SUCCESS;
  }

  int query( const char* string, char* response, size_t max_length, uint32_t& response_length, bool strip_nl=true )
  //---------------------------------------------------------------------------------------------------------------
  // Return the result of a query command, string, as a NULL terminated string in response.
  // The length of the string, NOT including the terminating NULL, is returned in response_length.
  // The response buffer is dimensioned with max_length bytes and this should be big enough
  // for the maximum expected response string including a terminating NULL.
  // If strip_nl is true, remove any trailing newline character.
  {
    int status = query_internal(string, response, max_length, response_length);
    if( status == LIBUSB_SUCCESS ){
      if( response_length == max_length )
        --response_length; // Oops ... buffer is not big enough to add a terminating NULL.
      response[response_length] = 0;
      if( strip_nl && (response[response_length-1] == '\n') ){
        response[response_length-1] = 0;
        --response_length;
      }
    }
    return status;
  }
}; // End of usbtmcio class declaration.

int quick_test( void )
//--------------------
// Do some transactions with a Rigol DM3000 to see if things basically work.
{
  usbtmcio dev(0x1ab1, 0x09c4, false);
  if( ! dev.ok ){
    fprintf(stderr, "Open failed.\n");
    return 1;
  }

  char response[512];
  uint32_t response_length;
  
  int status = dev.query("*IDN?\n", response, 512, response_length);
  printf("Status = %d\n", status);
  printf("Response length = %d\n", response_length);
  printf("Response = [%s]\n", response);

  for( int i=0; i<100; i++ ){
    status = dev.query(":MEAS:VOLT:AC?\n", response, 512, response_length);
    if( status != 0 )
      break;
    
    //printf("Status = %d\n", status);
    //printf("Response length = %d\n", response_length);
    printf("Count = %4d, Status = %d, Response = %s\n", i, status, response);
  }
  
  return 0;
}

int main( int ArgCount, char **Args )
//-----------------------------------
{
  // Default arguments.
  uint16_t vid = 0x1ab1;   // Vendor id (Rigol).
  uint16_t pid = 0x09c4;   // Product id (DM3000)
  bool version_mode = false;
  bool debug_mode = false;

  // Buffers.
  char incmd[BUFFER_SIZE];
  char response[BUFFER_SIZE];
  char* binary_buffer = NULL;
  uint32_t response_length = 0;
  size_t binary_buffer_size = 1024 * 1024;

  // Parse any command line arguments.
  CLI::App app{"USBTMC transput helper program for a68g"};
  Args = app.ensure_utf8(Args);

  app.add_option("-V,--vendorid", vid, "Vendor id for device.");
  app.add_option("-P,--productid", pid, "Product id for device.");
  app.add_option("-b,--buffer", binary_buffer_size, "Bytes to allocate for binary data buffer.");
  
  app.add_flag("-v,--version", version_mode, "Show version information, then exit,");
  app.add_flag("-d,--debug", debug_mode, "Turn on (very extensive) debug output,");

  CLI11_PARSE(app, ArgCount, Args);

  // Version display only.
  if( version_mode ){
    fprintf(stderr, "TMCUSBIO version: 0.1 (28-AUG-2026)\n");
    return 0;
  }

  // Allocate a buffer for binary data transfers.
  binary_buffer = (char*)malloc(binary_buffer_size);
  if( NULL == binary_buffer ){
    fprintf(stderr, "Binary buffer allocation failed.\n");
    return 1;
  }

#if 0
  quick_test();
  return 0;
#endif
  
  // Open the device.
  fprintf(stderr, "TMCUSBIO, opening VID = 0x%04x, PID = 0x%04x\n", vid, pid);
  usbtmcio dev(vid, pid, debug_mode);
  if( ! dev.ok ){
    fprintf(stderr, "Device open failed.\n");
    free(binary_buffer);
    return 1;
  }

  // Loop forever (more or less).
  bool exiting = false;
  while (1) {
    
    // Read commands from stdin.
    if( NULL == fgets(incmd, BUFFER_SIZE, stdin) ){
      fprintf(stderr, "TMCUSBIO, EOF or error on stdin. Exiting.\n");
      break;
    }
    // fprintf(stderr, "incmd = [%s].\n", incmd);

    // Decode command (first character of incmd).
    switch( incmd[0] ){

    case 'P': // put
      if( dev.put_only(incmd+1) == LIBUSB_SUCCESS )
        printf("*ok\n");
      else
        printf("?%s\n", dev.last_error_string);
      fflush(stdout);
      break;

    case 'Q': // query
      if( dev.query(incmd+1, response, BUFFER_SIZE, response_length, true) == LIBUSB_SUCCESS )
        printf("*%s\n", response); 
      else
        printf("?%s\n", dev.last_error_string);
      fflush(stdout);
      break;

    case 'B': // query with binary response
      if( dev.query_internal(incmd+1, binary_buffer, binary_buffer_size, response_length) == LIBUSB_SUCCESS ){
        char outline[81]; // 80 characters plus space for the blasted NULL terminator.
        printf("*ok:%u\n", response_length); // Total number of bytes is in the first line returned.
        if( response_length > 0 ){
          int j = 0;
          // Write up to 80 character lines containing up to 40 bytes/line as 2 character hexadecimal numbers.
          for( uint32_t i=0; i<response_length; i++ ){
            j = (2 * i) % 80;
            if( (i > 0) && (j == 0) ) // Flush outline after 80 output characters inserted into it.
              printf("%s\n", outline);
            snprintf(&outline[j], 3, "%02x", binary_buffer[i]); // 3 characters written including the blasted NULL.
          } // over output bytes.
          // Anything not written in outline?
          if( j > 0 )
            printf("%s\n", outline);
        } // response_length > 0
      }
      else
        printf("?%s\n", dev.last_error_string);
      fflush(stdout);
      break;

    case 'T': // set time out
      {
        char *endp = NULL;
        errno = 0;
        uint64_t new_timeout = strtoul(incmd+1, &endp, 10);
        if( errno == 0 ){
          printf("*ok\n");
          dev.timeout = (new_timeout > 100000) ? 100000 : (uint32_t)new_timeout ;
        }
        else
          printf("?%s\n", strerror(errno));
        fflush(stdout);
      }
      break;

    case 'X': // close and exit
      exiting = true;
      fprintf(stderr, "TMCUSBIO, Exit requested.\n");
      printf("*ok\n");
      fflush(stdout);
      break;

    default:
      fprintf(stderr, "TMCUSBIO, Unexpected command character: [%c]\n", incmd[0]);
      break;
    } // command switch

    if( exiting )
      break;

  } // eternal loop.

  free(binary_buffer);
  return 0;
}
