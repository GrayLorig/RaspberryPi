#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <TSL2561.h>

int main (int argc, char *argv[])
{
   int i;
   uint8_t addr = TSL2561::ADDR_29;
   const char *device = "/dev/i2c-1";
   TSL2561 chip;
   uint8_t gain = TSL2561::GAIN_1X;
   uint8_t time = TSL2561::INTEG_TIME_13_7MS;
   bool agc = false;
   bool sweep = false;
   bool once = false;

//
// First parse through the arguments
   for (i = 1 ; i < argc ; ++i)
   {
      if (strcmp(argv[i], "-d") == 0 ||
          strcmp(argv[i], "--device") == 0)
      {
         if (i+1 >= argc) // Missing a required argument
         {
            fprintf (stderr, "ERROR: Required argument for option %s omitted.", argv[i]);
            exit(1);
         }
         device = argv[i++];
      }
      else if (strcmp(argv[i], "-h") == 0 ||
          strcmp(argv[i], "--high-gain") == 0)
      {
         gain = TSL2561::GAIN_16X;
      }
      else if (strcmp(argv[i], "-l") == 0 ||
          strcmp(argv[i], "--low-gain") == 0)
      {
         gain = TSL2561::GAIN_1X;
      }
      else if (strcmp(argv[i], "-s") == 0 ||
          strcmp(argv[i], "--short-integration") == 0)
      {
         time = TSL2561::INTEG_TIME_13_7MS;
      }
      else if (strcmp(argv[i], "-m") == 0 ||
          strcmp(argv[i], "--medium-integration") == 0)
      {
         time = TSL2561::INTEG_TIME_101MS;
      }
      else if (strcmp(argv[i], "-L") == 0 ||
          strcmp(argv[i], "--long-integration") == 0)
      {
         time = TSL2561::INTEG_TIME_402MS;
      }
      else if (strcmp(argv[i], "-a") == 0 ||
          strcmp(argv[i], "--agc") == 0)
      {
         agc = true;
      }
      else if (strcmp(argv[i], "-o") == 0 ||
          strcmp(argv[i], "--once") == 0)
      {
         once = true;
      }
      else if (strcmp(argv[i], "--sweep") == 0)
      {
         sweep = true;
      }
      else
      {
         if (strcmp(argv[i], "-?") != 0 &&
             strcmp(argv[i], "--help") != 0)
            fprintf (stderr, "TSL2561: Unknown option \"%s\".\n", argv[i]);
         puts ("Usage: TSL2561-test [options ...]");
         puts ("       options: -d|--device device_name    Specify device name to use");
         puts ("                -h|--high-gain             Use high gain (16x)");
         puts ("                -l|--low-gain              Use low gain (1x)");
         puts ("                -s|--short_integration     Use a 13.7ms integration time");
         puts ("                -m|--medium_integration    Use a 101ms integration time");
         puts ("                -L|--long_integration      Use a 402ms integration time");
         puts ("                -a|--agc                   Automatically search for best gain and time");
         puts ("                -o|--once                  Only take a single reading");
         puts ("                --sweep                    Sweep through all combinations of gain and time");
         exit(1);
      }
   }

//
// Initialize the chip
   if (!chip.begin(device, addr))
      exit(1);

   chip.enable(true);
   chip.setGain(gain);
   chip.setIntegrationTime(time);

   int vis_ir, ir;
   const char *gain_names[] = {"1x", "16x"};
   const char *integ_time_names[] = {"13.7ms", "101ms", "402ms"};

//
//
   static const int AGC_GAINS[] = {TSL2561::GAIN_1X, TSL2561::GAIN_1X, TSL2561::GAIN_16X,
                                         TSL2561::GAIN_1X, TSL2561::GAIN_16X, TSL2561::GAIN_16X};
   static const int AGC_INTEG_TIMES[] = {TSL2561::INTEG_TIME_13_7MS, TSL2561::INTEG_TIME_101MS,
                                               TSL2561::INTEG_TIME_13_7MS, TSL2561::INTEG_TIME_402MS,
                                               TSL2561::INTEG_TIME_101MS, TSL2561::INTEG_TIME_402MS};
   static const int INTEG_MICROS[3] = {50000, 110000, 410000};
   do
   {
      usleep(INTEG_MICROS[chip.getIntegrationTime()]); // Sleep long enough to insure proper reading
      if (sweep)
      {
         int i;

//
// Sweep through the various combinations of gain and integration time in order
// from least sensitive to most.
         puts ("--------------------");
         for (i = 0 ; i < 6 ; ++i)
         {
            chip.setGain(AGC_GAINS[i]);
            chip.setIntegrationTime(AGC_INTEG_TIMES[i]);
            chip.enable(false);
            chip.enable(true);

            usleep(INTEG_MICROS[AGC_INTEG_TIMES[i]]);
            chip.getReading(vis_ir, ir, false);
             printf ("IR+VIS= %d, IR= %d (gain=%s, integration time=%s)\n",
                     vis_ir, ir,
                     gain_names[chip.getGain() >> 4],
                     integ_time_names[chip.getIntegrationTime()]);
         }
      }
      else
      {
//
// Just take a simple reading
         chip.getReading(vis_ir, ir, agc);
 
         if (agc)
             printf ("IR+VIS= %d, IR= %d (gain=%s, integration time=%s)\n",
                     vis_ir, ir,
                     gain_names[chip.getGain() >> 4],
                     integ_time_names[chip.getIntegrationTime()]);
         else
             printf ("IR+VIS= %d, IR= %d\n", vis_ir, ir);
      }
   } while (!once);

   chip.end();
}
