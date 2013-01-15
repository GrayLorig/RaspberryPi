/*
 * MCP3008.h: Support library for the MCP3008 10-bit A-to-D converter.
 *
 * Code by Barking Trout Productions/Gray Lorig
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <unistd.h>

class MCP3008
{
private:
   int fd_;
   uint32_t speed_;  // Connection speed

public:
   const static int MODE_LOOP = SPI_LOOP;
   const static int MODE_CPHA = SPI_CPHA;
   const static int MODE_CPOL = SPI_CPOL;
   const static int MODE_LSB_FIRST = SPI_LSB_FIRST;
   const static int MODE_CS_HIGH = SPI_CS_HIGH;
   const static int MODE_3WIRE = SPI_3WIRE;
   const static int MODE_NO_CS = SPI_NO_CS;
   const static int MODE_READY = SPI_READY;

   MCP3008()
   {
      fd_ = -1;
      speed_ = 0;
   }
//==============================================================================
// begin: Open access to the chip
//
   bool begin(const char *device, uint32_t speed = 1000000)
   {
      if (fd_ >= 0)
      {
         fputs("MCP3008: Device already open.\n", stderr);
         return false;
      }

      if ((fd_ = open(device, O_RDWR)) < 0)
      {
         fprintf (stderr, "MCP3008: Unable to open device %s.\n", device);
         return false;
      }

      uint8_t mode = 0;
      uint8_t bits = 8;
      speed_ = speed;

      if (ioctl(fd_, SPI_IOC_RD_MODE, &mode) < 0 ||
          ioctl(fd_, SPI_IOC_WR_MODE, &mode) < 0 ||
          ioctl(fd_, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0 ||
          ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
          ioctl(fd_, SPI_IOC_RD_MAX_SPEED_HZ, &speed_) < 0 ||
          ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed_) < 0)
      {
         fprintf (stderr, "MCP3008: Unable to configure device %s properly.", device);
         close(fd_);
         fd_ = -1;
         return false;
      }
          
      return true;
   }
//====================================================================================
// end: Close access to the chip
//
   void end()
   {
      if (fd_ >= 0)
      {
         close(fd_);
         fd_ = -1;
      }
      return;
   }

//======================================================================================
// getValue: Retrieve value from converter. If failure then return -1.
//
   const static int INPUT_MODE_SINGLE = 0;
   const static int INPUT_MODE_DIFFERENTIAL = 1;

   int getValue(uint8_t channel, int input_mode)
   {
      uint8_t rx_data[3];
      uint8_t tx_data[3];
      struct spi_ioc_transfer msg = {(unsigned long) tx_data,   // Transmit buffer
                                     (unsigned long) rx_data,   // Receive buffer
                                     3,                         // Data length
                                     0,                         // Delay
                                     speed_,                    // Transmission speed
                                     8};                        // Bits per word

      if (channel >= 8)
      {
         fputs("MCP3008: Invalid input channel specified.\n", stderr);
         return -1;
      }
      if (input_mode < 0 || input_mode > 1)
      {
         fputs("MCP3008: Invalid input mode specified.\n", stderr);
         return -1;
      }
      if (fd_ < 0)
      {
         fputs("MCP3008: Device has not been opened.\n", stderr);
         return -1;
      }

      tx_data[0] = 1; // Nothing but start bit
      tx_data[1] = (input_mode == INPUT_MODE_DIFFERENTIAL ? 0x80 : 0x00) | (channel << 4);
      tx_data[2] = 0;

      if (ioctl(fd_, SPI_IOC_MESSAGE(1), &msg) < 0)
         return -1;

      return ((((int) rx_data[1]) & 0x03) << 8) | ((int) rx_data[2]);
   }
};
