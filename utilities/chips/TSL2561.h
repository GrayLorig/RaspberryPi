// Raspberry Pi support for the TSL2561 Luminosity Sensor
//

// Code by Ignus Porkus/Gray Lorig
// License: LGPL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>

class TSL2561
{
public:
    static const uint8_t  ADDR_29 = 0x29;   // Address line low
    static const uint8_t  ADDR_39 = 0x39;   // Address line floating
    static const uint8_t  ADDR_49 = 0x49;   // Address line high

    static const uint8_t  GAIN_1X = 0x00;   // Low gain mode
    static const uint8_t  GAIN_16X = 0x10;  // High gain mode
    static const uint8_t  GAIN_MASK = 0x10;

    static const uint8_t  INTEG_TIME_13_7MS = 0x00;
    static const uint8_t  INTEG_TIME_101MS = 0x01;
    static const uint8_t  INTEG_TIME_402MS = 0x02;
    static const uint8_t  INTEG_TIME_MANUAL = 0x03;
    static const uint8_t  INTEG_TIME_MASK = 0x03;

private:
    uint8_t i2caddr_;
    int     deviceFd_;
    uint8_t gain_;
    uint8_t integTime_;

    static const uint8_t COMMAND_BIT = 0x80;
    static const uint8_t CLEAR_BIT = 0x40;
    static const uint8_t WORD_BIT = 0x20;
    static const uint8_t BLOCK_BIT = 0x10;

    static const uint8_t CONTROL_POWERON  = 0x03;
    static const uint8_t CONTROL_POWEROFF = 0x00;

    static const uint8_t REG_CONTROL = 0x00;
    static const uint8_t REG_TIMING = 0x01;
    static const uint8_t REG_ID = 0x0A;
    static const uint8_t REG_CHAN_0 = 0x0C;
    static const uint8_t REG_CHAN_1 = 0x0E;

public:
/**
 * Standard constructor
 */
     TSL2561()
     {
         i2caddr_ = 0;
         deviceFd_ = -1;
         gain_ = GAIN_1X;
         integTime_ = INTEG_TIME_13_7MS;
     }

/**
 * Open up communication with the chip.
 * @param	device	The device on which i2c communications can be initiated
 * @param	addr	i2c address at which the chip resides
 * @return	true is initialization was successful
 */
    bool begin(const char *device, uint8_t addr)
    {
//
// Insure we are reaching out on a valid address
        if (addr != ADDR_29 && addr != ADDR_39 && addr != ADDR_49)
        {
           fprintf(stderr, "TSL2561: Invalid i2c address specified: %02x\n", addr);
           return false;
        }
        i2caddr_ = addr;

//
// Attempt to open the i2c device
        if (deviceFd_ >= 0) // The connection is already open
        {
           fputs ("TSL2561: Device already open", stderr);
           return false;
        }

        if ((deviceFd_ = open(device, O_RDWR)) < 0)
        {
            fprintf (stderr, "TSL2561: Unable to open device %s\n", device);
            return false;
        }

//
// Indicate which slave we intend to talk to
        if (ioctl(deviceFd_, I2C_SLAVE, i2caddr_) < 0)
        {
            end(); // Close connection down
            fprintf (stderr, "TSL2561: Unable to ioctl %s\n", device);
            return false;
        }

//
// Now make sure there is an actual device out there
        enable(true); // Wake the chip up

        uint8_t buffer[2];
        buffer[0] = REG_ID;
        write(deviceFd_, buffer, 1);
        read(deviceFd_, buffer, 1);
        if ((buffer[0] & 0x0f) != 0x0a)
        {
           fprintf(stderr, "TSL2561: Unable to find chip address at address %02x (id = 0x%02x)\n",
                   i2caddr_, buffer[0]);
           end();
           return false;
        }

//
// Set the timing mode
        buffer[0] = COMMAND_BIT | REG_TIMING;
        buffer[1] = gain_ | integTime_;
        write (deviceFd_, buffer, 2);

        return true;
    }

/**
 * Close the connection to the chip down
 */
    void end()
    {
        if (deviceFd_ >= 0)
            close (deviceFd_);
        deviceFd_ = -1;
    }

/**
 * enable: Enable or Disable the chip putting into a power-saving mode
 * @param enable   If true chip is powered up
 */
    void enable(bool e)
    {
        uint8_t buffer[2];

        if (deviceFd_ < 0) return; // We are not yet initialized

        buffer[0] = COMMAND_BIT | REG_CONTROL;
        if (e)
           buffer[1] = CONTROL_POWERON;
        else
           buffer[1] = CONTROL_POWEROFF;
        write(deviceFd_, buffer, 2);
    }

/**
 * Set the integration time to be used for measurements
 * @param time One of the available time constants
 */
    void setIntegrationTime(uint8_t time)
    {
        integTime_ = time;

        if (deviceFd_ < 0) return;

        uint8_t buffer[2];
        buffer[0] = COMMAND_BIT | REG_TIMING;
        buffer[1] = gain_ | integTime_;
        write (deviceFd_, buffer, 2);
    }
/**
 * Get the current integration time setting.
 * @return The current setting for integration time.
 */
    uint8_t getIntegrationTime() {return integTime_;}
/**
 * Get the current gain setting.
 * @return The current setting for gain
 */
    uint8_t getGain() {return gain_;}

/**
 * Set the gain for the sensor
 * @param gain One of the specified gain values
 */
   void setGain(uint8_t gain)
    {
        gain_ = gain;

        if (deviceFd_ < 0) return;

        uint8_t buffer[2];
        buffer[0] = COMMAND_BIT | REG_TIMING;
        buffer[1] = gain_ | integTime_;
        write (deviceFd_, buffer, 2);
    }

/**
 * Retrieve a reading from the chip
 * @param ir_vis Combined visible and infrared reading
 * @param ir     Just the infrared component
 * @param agc    Automatically adjust the gain and integration time
 */
    void getReading(int &ir_vis, int &ir, bool agc = false)
    {
        uint8_t buffer[2];
        
        if (deviceFd_ < 0) return;
//
// Grab an initial visible reading
        buffer[0] = COMMAND_BIT | WORD_BIT | REG_CHAN_0;
        write(deviceFd_, buffer, 1);
        read(deviceFd_, buffer, 2);
        ir_vis = buffer[0] + ((int)buffer[1]<<8);
  
//
// If we are adjusting the gain automatically check how we are doing
        if (agc)
        {
//            static const double AGC_SCALES[] = {1.0000, 7.2723, 16.0000, 29.3431, 117.95620, 469.4891};
            static const int AGC_GAINS[] = {GAIN_1X, GAIN_1X, GAIN_16X, GAIN_1X, GAIN_16X, GAIN_16X};
            static const int AGC_INTEG_TIMES[] = {INTEG_TIME_13_7MS, INTEG_TIME_101MS, INTEG_TIME_13_7MS,
                                                           INTEG_TIME_402MS, INTEG_TIME_101MS, INTEG_TIME_402MS};
            static const int AGC_LOWER_LIMITS[] = {4237, 15835, 17867, 8151, 8232, 0};
            static const int AGC_XREF[2][3] = {{0, 1, 3}, {2, 4, 5}};
            static const int INTEG_MICROS[3] = {50000, 110000, 410000};

            int factor = AGC_XREF[gain_ >> 4][integTime_];
            while (true)
            {
//
// First look for erroneous values. There are just some light levels which cause this chip
// to get whacked out.
                
                if (ir_vis > 32768 && factor != 0) // Too bright, scale down
                    factor--;
                else if (ir_vis < AGC_LOWER_LIMITS[factor]) // Too dim, scale up
                    factor++;
                else
                    break; // Just right

//
// Set the gain and integration time. Then we will need to wait for another reading
                gain_ = AGC_GAINS[factor];
                integTime_ = AGC_INTEG_TIMES[factor];
                buffer[0] = COMMAND_BIT | REG_TIMING;
                buffer[1] = gain_ | integTime_;
                write (deviceFd_, buffer, 2);

                enable(false); // Force a new integration to start
                enable(true);

                usleep(INTEG_MICROS[integTime_]);
//
// Grab an initial visible reading
                buffer[0] = COMMAND_BIT | WORD_BIT | REG_CHAN_0;
                write(deviceFd_, buffer, 1);
                read(deviceFd_, buffer, 2);
                ir_vis = buffer[0] + ((int)buffer[1]<<8);
            }
        }
       
//
// Now fetch the ir reading
        buffer[0] = COMMAND_BIT | WORD_BIT | REG_CHAN_1;
        write(deviceFd_, buffer, 1);
        read(deviceFd_, buffer, 2);
        ir = buffer[0] + ((int)buffer[1]<<8);
    }
};

