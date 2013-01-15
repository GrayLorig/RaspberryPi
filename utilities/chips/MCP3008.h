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
   bool begin(const char *device, uint32_t speed = 1000000)
   {
      if (fd_ >= 0)
      {
         fputs("ERROR: MCP3008 device already open.\n", stderr);
         return false;
      }

      if ((fd_ = open(device, O_RDWR)) < 0)
      {
         fprintf (stderr, "ERROR: Unable to open device %s.\n", device);
         return false;
      }

      uint8_t mode = 0;
      uint8_t bits = 8;
      speed_ = speed;

      if (ioctl(fd_, SPI_IOC_RD_MODE, &mode) < 0 ||
          ioctl(fd_, SPI_IOC_WD_MODE, &mode) < 0 ||
          ioctl(fd_, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0 ||
          ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
          ioctl(fd_, SPI_IOC_RD_MAX_SPEED_HZ, &speed_) < 0 ||
          ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed_) < 0)
      {
         fprintf (stderr, "ERROR: Unable to configure device %s properly.", device);
         close(fd_);
         fd_ = -1;
         return false;
      }
          
      return true;
   }
};
