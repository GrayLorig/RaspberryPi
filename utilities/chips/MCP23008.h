// i2c expander library - slow I/O!
// also works with the MCP23009

// Code by Adafruit Industries/Limor Fried
// Adapted for RaspberryPi by Ignus Porkus/Gray Lorig
// License: LGPL

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>

class MCP23008
{
public:
   static const uint8_t INPUT    = 0;
   static const uint8_t OUTPUT   = 1;
   static const uint8_t LOW      = 0;
   static const uint8_t HIGH     = 1;
   static const uint8_t PULLDOWN = 0;
   static const uint8_t PULLUP   = 1;

   bool begin(const char *device_name, uint8_t addr);
   bool begin() { return begin("/dev/i2c-1", 0); }
   void end();
   bool isOpen() { return deviceFd_ >= 0; }
   int  getFd()  { return deviceFd_; }

   bool setupPins(uint8_t iodir, uint8_t pullup = 0, uint8_t invert = 0);
   bool writePins(uint8_t bits);
   bool readPins(uint8_t &bits);

   bool pinMode(uint8_t p, uint8_t d);
   bool digitalWrite(uint8_t p, uint8_t d);
   bool pullUp(uint8_t p, uint8_t d);
   uint8_t digitalRead(uint8_t p);

   MCP23008()
   {
      i2caddr_ = 0;
      deviceFd_ = -1;
      iodir_ = 0xff; // All inputs
      gppu_ = 0x00;  // No pullup
      olat_ = 0x00;  // Output latches
   }

private:
   static const uint8_t MCP23008_ADDRESS = 0x20;
   static const uint8_t MCP23008_IODIR   = 0x00;
   static const uint8_t MCP23008_IPOL    = 0x01;
   static const uint8_t MCP23008_GPINTEN = 0x02;
   static const uint8_t MCP23008_DEFVAL  = 0x03;
   static const uint8_t MCP23008_INTCON  = 0x04;
   static const uint8_t MCP23008_IOCON   = 0x05;
   static const uint8_t MCP23008_GPPU    = 0x06;
   static const uint8_t MCP23008_INTF    = 0x07;
   static const uint8_t MCP23008_INTCAP  = 0x08;
   static const uint8_t MCP23008_GPIO    = 0x09;
   static const uint8_t MCP23008_OLAT    = 0x0A;

   uint8_t i2caddr_;
   int     deviceFd_;
   uint8_t iodir_;
   uint8_t gppu_;
   uint8_t olat_;
};


//=============================================================================
// begin: Open the device and initialize it.
//
inline bool MCP23008::begin(const char *device, uint8_t addr)
{
//
// Remember chip only supports three bits of addressing
  if (addr > 7)
     i2caddr_ = addr;
  else
     i2caddr_ = addr | MCP23008_ADDRESS; // Add fixed portion of address

//
// Attempt to open socket connection
   if (deviceFd_ >= 0) // Connection is already open
   {
      fputs("MCP23008: Device already open", stderr);
      return false;
   }

   if ((deviceFd_ = open(device, O_RDWR)) < 0)
   {
      fprintf (stderr, "MCP23008: Unable to open device %s", device);
      return false;
   }

//
// Indicate which slave we intend to talk to
   if (ioctl(deviceFd_, I2C_SLAVE, i2caddr_) < 0)
   {
      end();
      fprintf(stderr, "MCP23008: Unable to ioctl %s.", device);
      return false;
   }
//
// Gather in the current state of the device
   uint8_t buffer[1];

   buffer[0] = MCP23008_IODIR;
   write(deviceFd_, buffer, 1);
   read(deviceFd_, &iodir_, 1);

   buffer[0] = MCP23008_GPPU;
   write(deviceFd_, buffer, 1);
   read(deviceFd_, &gppu_, 1);

   buffer[0] = MCP23008_OLAT;
   if (write(deviceFd_, buffer, 1) != 1)
   {
      end();
      fputs("MCP23008: Unable to write register on device.\n", stderr);
      return false;
   }
   if (read(deviceFd_, &olat_, 1) != 1)
   {
      end();
      fputs("MCP23008: Unable to read register from device.\n", stderr);
      return false;
   }
   return true;
}

//=====================================================================
// end: Close the device connection
//
inline void MCP23008::end()
{
   if (deviceFd_ >= 0)
      close (deviceFd_);
   deviceFd_ = -1;
}

//====================================================================
// setupPins: Initialize the GPIO pins on the device.
//
inline bool MCP23008::setupPins(uint8_t iodir, uint8_t pullup, uint8_t invert)
{
   uint8_t buffer[2];

   if (deviceFd_ < 0) // Make sure the device is open
   {
       fputs ("MCP23008: Device is not open\n", stderr);
       return false;
   }

   buffer[0] = MCP23008_IODIR;
   buffer[1] = iodir_ = ~iodir;
   write(deviceFd_, buffer, 2);

   buffer[0] = MCP23008_IPOL;
   buffer[1] = invert;
   write(deviceFd_, buffer, 2);

   buffer[0] = MCP23008_GPPU;
   buffer[1] = gppu_ = pullup;
   if (write(deviceFd_, buffer, 2) != 2)
   {
      end();
      fputs("MCP23008: Unable to write control registers.\n", stderr);
      return false;
   }
   return true;
}

//====================================================================
// writePins: Write all output pins in one fell swoop.
//
inline bool MCP23008::writePins(uint8_t bits)
{
   uint8_t buffer[2];

   if (deviceFd_ < 0) // Make sure the device is open
   {
       fputs ("MCP23008: Device is not open\n", stderr);
       return false;
   }

   buffer[0] = MCP23008_OLAT;
   buffer[1] = olat_ = bits;
   if (write(deviceFd_, buffer, 2) != 2)
   {
      end();
      fputs("MCP23008: Unable to write output latch register.\n", stderr);
      return false;
   }
   return true;
}

//====================================================================
// readPins: Read all of the inputs in one go.
//
inline bool MCP23008::readPins(uint8_t &bits)
{
   uint8_t buffer[1];

   if (deviceFd_ < 0) // Make sure the device is open
   {
       fputs ("MCP23008: Device is not open\n", stderr);
       return false;
   }

   buffer[0] = MCP23008_GPIO;
   if (write(deviceFd_, buffer, 1) != 1)
   {
      end();
      fputs("MCP23008: Unable to initiate GPIO read.\n", stderr);
      return false;
   }
   if (read(deviceFd_, &bits, 1) != 1)
   {
      end();
      fputs("MCP23008: Read of GPIO registered failed.\n", stderr);
      return false;
   }
   return true;
}

//====================================================================
// pinMode: Set whether a pin is an input or an output.
//
inline bool MCP23008::pinMode(uint8_t p, uint8_t d)
{
   if (deviceFd_ < 0) // Make sure the device is open
   {
       fputs ("MCP23008: Device is not open\n", stderr);
       return false;
   }

   if (p > 7) // We only have 8 pins
   {
      fputs ("MCP23008: Invalid pin specified\n", stderr);
      return false;
   }

//
// Set the direction
   if (d == MCP23008::INPUT)
     iodir_ |= 1 << p; 
   else
     iodir_ &= ~(1 << p);

// write the new IODIR
   uint8_t buffer[2];

   buffer[0] = MCP23008::MCP23008_IODIR;
   buffer[1] = iodir_;

   if (write(deviceFd_, buffer, 2) != 2)
   {
      end();
      fputs("MCP23008: Unable to write iodir register\n", stderr);
      return false;
   }

   return true;
}

//==============================================================
// digitalWrite: Set the state of an output pin
//
inline bool MCP23008::digitalWrite(uint8_t p, uint8_t d)
{
   if (deviceFd_ < 0) // Make sure the device is open
   {
       fputs ("MCP23008: Device is not open\n", stderr);
       return false;
   }

   if (p > 7) // We only have 8 pins
   {
      fputs ("MCP23008: Invalid pin specified\n", stderr);
      return false;
   }
//
// Set the appropriate pin  
   if (d == MCP23008::HIGH)
      olat_ |= 1 << p; 
   else
      olat_ &= ~(1 << p);

// write the new output latch values
   uint8_t buffer[2];

   buffer[0] = MCP23008::MCP23008_OLAT;
   buffer[1] = olat_;

   if (write(deviceFd_, buffer, 2) != 2)
   {
      end();
      fputs("MCP23008: Unable to write olat\n", stderr);
      return false;
   }

   return true;
}

//============================================================
// pullUp: Set the pullup resister on a pin
//
inline bool MCP23008::pullUp(uint8_t p, uint8_t d)
{
   if (deviceFd_ < 0) // Make sure the device is open
   {
       fputs ("MCP23008: Device is not open\n", stderr);
       return false;
   }

   if (p > 7) // We only have 8 pins
   {
      fputs ("MCP23008: Invalid pin specified\n", stderr);
      return false;
   }
  
// Set the pullup state
   if (d == MCP23008::PULLUP)
      gppu_ |= 1 << p; 
   else
      gppu_ &= ~(1 << p);

// write the new pull up state
   uint8_t buffer[2];

   buffer[0] = MCP23008::MCP23008_GPPU;
   buffer[1] = gppu_;

   if (write(deviceFd_, buffer, 2) != 2)
   {
      end();
      fputs("MCP23008: Unable to write gppu\n", stderr);
      return false;
   }

   return true;
}

//=========================================================
// digitalRead: Pull in the value of an input pin
//
inline uint8_t MCP23008::digitalRead(uint8_t p)
{
   if (deviceFd_ < 0) // Make sure the device is open
   {
       fputs ("MCP23008: Device is not open\n", stderr);
       return 0xff;
   }

   if (p > 7) // We only have 8 pins
   {
      fputs ("MCP23008: Invalid pin specified\n", stderr);
      return 0xff;
   }

//
// Read back the GPIO register
   uint8_t buffer[1];
   buffer[0] = MCP23008::MCP23008_GPIO;

   if (write(deviceFd_, buffer, 1) != 1)
   {
      end();
      fputs ("MCP23008: Unable to write gpio\n", stderr);
      return 0xff;
   }

   if (read(deviceFd_, buffer, 1) != 1)
   {
      end();
      fputs ("MCP23008: Unable to read back gpio\n", stderr);
      return 0xff;
   }

   return (buffer[0] >> p) & 0x01;
}
