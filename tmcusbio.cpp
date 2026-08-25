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

static const int BUFFER_SIZE = 2048;

// Encapsulate communications with a USB TMC device using libusb.
class usbtmcio
{
public:
  libusb_context* ctx;
  libusb_device_handle* handle;
  libusb_device* device;
  uint16_t vendor_id;
  uint16_t product_id;
  int current_config;
  uint8_t out_endpoint;
  uint8_t in_endpoint;
  uint8_t transaction_no;
  unsigned int timeout;
  struct libusb_config_descriptor *config;

  char last_error_string[256];
  uint8_t header[12];
  uint8_t buffer[BUFFER_SIZE+12];
  bool ok;

public:
  usbtmcio(uint16_t u_vendor_id, uint16_t u_product_id )
  //----------------------------------------------------
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
    memset(last_error_string, 0, sizeof(last_error_string));
    
    // Assume it didn't work. Usually a safe assumption ... :-)
    ok = false;
    
    // Initialise libusb.
    if( libusb_init(&ctx) < 0 ){
      perror("libusb_init() failed");
    }
    else{

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
              if( address & LIBUSB_ENDPOINT_IN ){
                if( in_endpoint == 0 && attribs == 2 )
                  in_endpoint = address;
              }
              else{
                if( out_endpoint == 0 && attribs == 2 )
                  out_endpoint = address;
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
    timeout = (ms < 10000) ? ms : 10000;
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
    memset(header, 0, 12);
    header[0] = (outgoing) ? 1 : 2 ; // DEV_DEP_MSG_OUT, REQUEST_DEV_DEP_MSG_IN
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
    if( ! ok )
      return LIBUSB_ERROR_OTHER;

    // Validate string.
    uint32_t n_chars = (uint32_t)strlen(string);
    if( 0 == n_chars )
      return LIBUSB_ERROR_OTHER;
    if( n_chars > BUFFER_SIZE )
      return LIBUSB_ERROR_OTHER;

    // Construct a header to send the string.
    make_header(n_chars, true, true);
    memcpy(buffer, header, 12);
    memcpy(buffer+12, string, n_chars);

    // Send to the device.
    int transferred = 0;
    int status = libusb_bulk_transfer(handle,
                                      out_endpoint,
                                      buffer, 12+n_chars,
                                      &transferred,
                                      timeout);
    if( 0 != status ){
      strncpy(last_error_string, libusb_error_name(status), 256);
      fprintf(stderr, "TMCUSBIO, put_only(): error: %s\n", last_error_string);
      return status;
    }

    // Bump the transaction count.
    bump_transaction();
    
    return LIBUSB_SUCCESS;
  };

  int query( const char* string, char* response, size_t max_length, uint32_t& response_length )
  //-------------------------------------------------------------------------------------------
  // Send string to the device, expecting a response. Get the response.
  // Return a libusb status if any transfer fails (and issue an error message).
  {
    if( ! ok )
      return LIBUSB_ERROR_OTHER;
    response_length = 0;

    // Validate string.
    uint32_t n_chars = (uint32_t)strlen(string);
    if( 0 == n_chars )
      return LIBUSB_ERROR_OTHER;
    if( n_chars > BUFFER_SIZE )
      return LIBUSB_ERROR_OTHER;

    // Construct a header to send the string.
    make_header(n_chars, true, true);
    memcpy(buffer, header, 12);
    memcpy(buffer+12, string, n_chars);

    // Send to the device.
    int transferred = 0;
    int status = libusb_bulk_transfer(handle,
                                      out_endpoint,
                                      buffer, 12+n_chars,
                                      &transferred,
                                      timeout);
    if( 0 != status ){
      strncpy(last_error_string, libusb_error_name(status), 256);
      fprintf(stderr, "query(): command out error: %s\n", last_error_string);
      return status;
    }

    // Construct a request response header.
    make_header(BUFFER_SIZE, false, false);

    // Send the header (no data payload).
    status = libusb_bulk_transfer(handle,
                                  out_endpoint,
                                  header,
                                  12,
                                  &transferred,
                                  timeout);
    if( 0 != status ){
      strncpy(last_error_string, libusb_error_name(status), 256);
      fprintf(stderr, "query(): request response error: %s\n", last_error_string);
      return status;
    }

    // Read the response.
    memset(buffer, 0, BUFFER_SIZE );
    status = libusb_bulk_transfer(handle,
                                  in_endpoint,
                                  buffer,
                                  BUFFER_SIZE,
                                  &transferred,
                                  timeout);
    if( 0 != status ){
      strncpy(last_error_string, libusb_error_name(status), 256);
      fprintf(stderr, "query(): read response error: %s\n", last_error_string);
      return status;
    }

    // Bump the transaction count.
    bump_transaction();

    // Return the query result.
    memcpy(header, buffer, 12);
    response_length = header[4] + (header[5] << 8) + (header[6] << 16) + (header[7] << 24);
    size_t copy_length = (response_length >= max_length) ? max_length-1 : response_length ;
    memcpy(response, buffer+12, copy_length);

    // Terminate the string with a 0 as per C strings. Maybe lose \n?
    response[response_length-1] = 0;

    return LIBUSB_SUCCESS;
  };
};

int quick_test( void )
//--------------------
// Do some transactions with a Rigol DM3000 to see if things basically work.
{
  usbtmcio dev(0x1ab1, 0x09c4);
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

  for( int i=0; i<1000; i++ ){
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

  // Buffers.
  char incmd[BUFFER_SIZE];
  char response[BUFFER_SIZE];
  uint32_t response_length = 0;

  // Parse any command line arguments.
  CLI::App app{"USBTMC transput helper program for a68g"};
  Args = app.ensure_utf8(Args);

  app.add_option("-V,--vendorid", vid, "Vendor id for device.");
  app.add_option("-P,--productid", pid, "Product id for device.");

  app.add_flag("-v,--version", version_mode, "Show version information, then exit,");
  
  CLI11_PARSE(app, ArgCount, Args);

  // Version display only.
  if( version_mode ){
    fprintf(stderr, "TMCUSBIO version: 0.1 (25-AUG-2026)\n");
    return 0;
  }

  // Open the device.
  fprintf(stderr, "TMCUSBIO, opening VID = 0x%04x, PID = 0x%04x\n", vid, pid);
  usbtmcio dev(vid, pid);
  if( ! dev.ok ){
    fprintf(stderr, "Device open failed.\n");
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
    case 'P':
      if( dev.put_only(incmd+1) == LIBUSB_SUCCESS )
        printf("*ok\n");
      else
        printf("?%s\n", dev.last_error_string);
      fflush(stdout);
      break;

    case 'Q':
      if( dev.query(incmd+1, response, BUFFER_SIZE, response_length) == LIBUSB_SUCCESS )
        printf("*%s\n", response); 
      else
        printf("?%s\n", dev.last_error_string);
      fflush(stdout);
      break;

    case 'X':
      exiting = true;
      fprintf(stderr, "TMCUSBIO, Exit requested.\n");
      printf("*ok\n");
      fflush(stdout);
      break;

    default:
      fprintf(stderr, "TMCUSBIO, Unexpected command character: [%c]\n", incmd[0]);
      break;
          
    }

    if( exiting )
      break;

  } // eternal loop.
  
  return 0;
}
