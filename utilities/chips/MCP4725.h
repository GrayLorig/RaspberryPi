// Support for MCP4725 12-bit DAC

// Code by Ignus Porkus/Gray Lorig
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

class MCP4725
{
public:
   static const uint8_t MODE_NORMAL         = 0x00;
   static const uint8_t MODE_POWERDOWN_1K   = 0x01;
   static const uint8_t MODE_POWERDOWN_100K = 0x02;
   static const uint8_t MODE_POWERDOWN_500K = 0x03;

   bool begin(const char *device_name, uint8_t addr);
   bool begin() { return begin("/dev/i2c-1", 0); }
   void end();
   bool isOpen() { return deviceFd_ >= 0; }
   int  getFd()  { return deviceFd_; }

   bool powerDown(uint8_t mode, bool persist=false);
   bool setValue(uint16_t value, bool persist=false);

   MCP4725()
   {
      i2caddr_ = 0;
      deviceFd_ = -1;
   }

private:
   static const uint8_t MCP4725_ADDRESS      = 0x60;
   static const uint8_t MCP4725_FAST_WRITE   = 0x00;
   static const uint8_t MCP4725_DAC_WRITE    = 0x40;
   static const uint8_t MCP4725_EEPROM_WRITE = 0x60;
   static const uint16_t MCP4725_MAX_VALUE   = 0x0fff; // We are a 12-bit DAC

   uint8_t i2caddr_;
   int     deviceFd_;
};


//=============================================================================
// begin: Open the device and initialize it.
//
inline bool MCP4725::begin(const char *device, uint8_t addr)
{
//
// Remember chip only supports three bits of addressing
  if (addr > 7)
     i2caddr_ = addr;
  else
     i2caddr_ = addr | MCP4725_ADDRESS; // Add fixed portion of address

//
// Attempt to open the i2c device driver
   if (deviceFd_ >= 0) // Connection is already open
   {
      fputs("MCP4725: Device already open", stderr);
      return false;
   }

   if ((deviceFd_ = open(device, O_RDWR)) < 0)
   {
      fprintf (stderr, "MCP4725: Unable to open device %s", device);
      return false;
   }

//
// Indicate which slave we intend to talk to
   if (ioctl(deviceFd_, I2C_SLAVE, i2caddr_) < 0)
   {
      end();
      fprintf(stderr, "MCP4725: Unable to ioctl %s.", device);
      return false;
   }

   return true;
}

//=====================================================================
// end: Close the device connection
//
inline void MCP4725::end()
{
   if (deviceFd_ >= 0)
      close (deviceFd_);
   deviceFd_ = -1;
}

//====================================================================
// powerDown: Set the DAC into powered down state. The output line will
//            be pulled down to ground using the indicated resistor.
//
inline bool MCP4725::powerDown(uint8_t mode, bool persist)
{
   uint8_t buffer[3];
   int size;

   if (deviceFd_ < 0) // Make sure the device is open
   {
       fputs ("MCP4725: Device is not open\n", stderr);
       return false;
   }

   if (mode != MODE_POWERDOWN_1K &&
       mode != MODE_POWERDOWN_100K &&
       mode != MODE_POWERDOWN_500K) // Only supported powerdown modes
   {
      fputs ("MCP4725: Unsupported powerdown mode\n", stderr);
      return false;
   }

   if (persist)
   {
      buffer[0] = MCP4725_EEPROM_WRITE | (mode << 1);
      buffer[1] = 0x80; // Set to mid scale
      buffer[2] = 0x00;

      size = 3;
   }
   else // If we are not persisting use a fast write
   {
      buffer[0] = MCP4725_FAST_WRITE | (mode << 4) | 0x08;
      buffer[1] = 0;

      size = 2;
   }

   if (write(deviceFd_, buffer, size) != size)
   {
      end();
      fputs("MCP4725: Unable to write power down command\n", stderr);
      return false;
   }

   return true;
}

//====================================================================
// setValue: Set the output value as indicated.
//
inline bool MCP4725::setValue(uint16_t value, bool persist)
{
   uint8_t buffer[3];
   int size;

   if (deviceFd_ < 0) // Make sure the device is open
   {
       fputs ("MCP4725: Device is not open\n", stderr);
       return false;
   }

   if (value > MCP4725_MAX_VALUE) // Make sure we are not out of range
   {
      fputs ("MCP4725: Value is out of range\n", stderr);
      return false;
   }

   if (persist)
   {
      buffer[0] = MCP4725_EEPROM_WRITE | (MODE_NORMAL << 1);
      buffer[1] = (value >> 4) & 0xff; // Top 8 bits
      buffer[2] = (value << 4) & 0xff; // Bottom 4 bits

      size = 3;
   }
   else // If we are not persisting use a fast write
   {
      buffer[0] = MCP4725_FAST_WRITE | (MODE_NORMAL << 4) | (value >> 8);
      buffer[1] = value & 0xff;

      size = 2;
   }

   if (write(deviceFd_, buffer, size) != size)
   {
      end();
      fputs("MCP4725: Unable to write value command\n", stderr);
      return false;
   }

   return true;
}
