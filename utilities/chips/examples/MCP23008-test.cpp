#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <MCP23008.h>

int main(int argc, char *argv[])
{
   int i;
   uint8_t pin;
   uint8_t value;
   const char *device = "/dev/i2c-1";
   char *eptr;
   uint8_t address = 0;
   MCP23008 chip;

//
// First parse through the arguments looking or the general ones
   for (i = 1 ; i < argc ; ++i)
   {
      if (strcmp(argv[i], "-d") == 0 ||
          strcmp(argv[i], "-device") == 0)
      {
         if (i+1 >= argc) // Missing a required argument
         {
            fprintf (stderr, "ERROR: Required argument for option %s omitted.", argv[i]);
            exit(1);
         }
         device = argv[++i];
      }
      else if (strcmp(argv[i], "-a") == 0 ||
               strcmp(argv[i], "-address") == 0)
      {
         if (i+1 >= argc) // Missing a required argument
         {
            fprintf (stderr, "ERROR: Required argument for option %s omitted.", argv[i]);
            exit(1);
         }
         address = strtol(argv[++i], &eptr, 0);
      }
   }

   if (!chip.begin(device, address)) // Open up our connection
      exit(1);

//
// Now run through the command line treating the remainder of the options like commands to
// be done in sequence.
   for (i = 1 ; i < argc ; ++i)
   {
      if (strcmp(argv[i], "-w") == 0) // Write a bit
      {
         if (i+2 >= argc) // Missing a required argument
         {
            fprintf (stderr, "ERROR: Required argument for option %s omitted.", argv[i]);
            exit(1);
         }
         pin = atoi(argv[++i]);
         ++i;
         if (strcmp(argv[i], "HIGH") == 0 ||
             strcmp(argv[i], "H") == 0 ||
             strcmp(argv[i], "1") == 0 ||
             strcmp(argv[i], "TRUE") == 0 ||
             strcmp(argv[i], "T") == 0)
         {
            value = MCP23008::HIGH;
         }
         else if (strcmp(argv[i], "LOW") == 0 ||
             strcmp(argv[i], "L") == 0 ||
             strcmp(argv[i], "0") == 0 ||
             strcmp(argv[i], "FALSE") == 0 ||
             strcmp(argv[i], "F") == 0)
         {
            value = MCP23008::LOW;
         }
         else
         {
             fprintf (stderr,"ERROR: Unknown pin state %s.\n", argv[i]);
             exit(1);
         }
         if (!chip.digitalWrite(pin, value))
            exit(1);
      }
      else if (strcmp(argv[i], "-W") == 0) // Write all bits
      {
         if (i+1 >= argc) // Missing a required argument
         {
            fprintf (stderr, "ERROR: Required argument for option %s omitted.", argv[i]);
            exit(1);
         }
         value = strtol(argv[++i], &eptr, 0);
         if (!chip.writePins(value))
            exit(1);
      }
      else if (strcmp(argv[i], "-r") == 0) // Read a pin
      {
         if (i+1 >= argc) // Missing a required argument
         {
            fprintf (stderr, "ERROR: Required argument for option %s omitted.", argv[i]);
            exit(1);
         }
         pin = atoi(argv[++i]);
         if (pin < 0 || pin >= 8)
         {
            fputs("ERROR: Invalid pin specified.\n", stderr);
            exit(1);
         }
         printf ("Pin %d is %s\n", pin,
                 chip.digitalRead(pin) == MCP23008::HIGH ? "HIGH" : "LOW");
      }
      else if (strcmp(argv[i], "-R") == 0) // Read all bits
      {
         int j;

         chip.readPins(value);
         printf("Pins = 0x%x ", value);

         for (j = 0x80 ; j != 0 ; j >>= 1)
            putchar(j&value ? '1' : '0');
         putchar('\n');
      }
      else if (strcmp(argv[i], "-io") == 0) // Set direction for pin
      {
         if (i+2 >= argc) // Missing a required argument
         {
            fprintf (stderr, "ERROR: Required argument for option %s omitted.", argv[i]);
            exit(1);
         }
         pin = atoi(argv[++i]);
         ++i;
         if (strcmp(argv[i], "IN") == 0 ||
             strcmp(argv[i], "INPUT") == 0)
         {
            value = MCP23008::INPUT;
         }
         else if (strcmp(argv[i], "OUT") == 0 ||
                  strcmp(argv[i], "OUTPUT") == 0)
         {
            value = MCP23008::OUTPUT;
         }
         else
         {
             fprintf (stderr,"ERROR: Unknown pin direction %s.\n", argv[i]);
             exit(1);
         }
         if (!chip.pinMode(pin, value))
            exit(1);
      }
      else if (strcmp(argv[i], "-pullup") == 0) // Set pullup on an output pin
      {
         if (i+2 >= argc) // Missing a required argument
         {
            fprintf (stderr, "ERROR: Required argument for option %s omitted.", argv[i]);
            exit(1);
         }
         pin = atoi(argv[++i]);
         ++i;
         if (strcmp(argv[i], "ON") == 0 ||
             strcmp(argv[i], "UP") == 0)
         {
            value = MCP23008::PULLUP;
         }
         else if (strcmp(argv[i], "OFF") == 0 ||
                  strcmp(argv[i], "DOWN") == 0)
         {
            value = MCP23008::PULLDOWN;
         }
         else
         {
             fprintf (stderr,"ERROR: Unknown pullup setting %s.\n", argv[i]);
             exit(1);
         }
         if (!chip.pullUp(pin, value))
            exit(1);
      }
      else if (strcmp(argv[i], "-pins") == 0) // Setup pins
      {
         int j, b;
         uint8_t iodir = 0;
         uint8_t ipol = 0;
         uint8_t gppu = 0;

         if (i+1 >= argc) // Missing a required argument
         {
            fprintf (stderr, "ERROR: Required argument for option %s omitted.", argv[i]);
            exit(1);
         }
         ++i;
//
// parse the descriptive string
         for (j = 0, b = 0x100 ; argv[i][j] != '\0' && b != 0 ; j++)
            switch(argv[i][j])
            {
            case 'I': // Input line
               b >>= 1;
               iodir &= ~b;
               break;

            case 'O': // Output line
               b >>= 1;
               iodir |= b;
               break;

            case '-': // Invert the input
               ipol |= b;
               break;

            case 'p': // Pullup
               gppu |= b;
               break;

            default:
               fprintf (stderr, "ERROR: Unknown pin specifier \'%c\'\n", argv[i][j]);
               exit(1);
            }

         if (!chip.setupPins(iodir, gppu, ipol))
            exit(1);
      }
      else if (strcmp(argv[i], "-d") != 0 &&
               strcmp(argv[i], "-a") != 0)
      {
         if (strcmp(argv[i], "-?") != 0 &&
             strcmp(argv[i], "-help") != 0)
            fprintf (stderr, "ERROR: Unknown argument %s\n", argv[i]);
         puts ("Usage: MCP23008-test [options] [commands]");
         puts ("   Options: -d device_name                    Specify device bus");
         puts ("            -a address                        Specify device address on bus");
         puts ("            -help                             Print message");
         puts ("  Commands: -w pin HIGH|LOW|TRUE|FALSE|1|0    Write single output line");
         puts ("            -W bits                           Write all eight output lines at once");
         puts ("            -r pin                            Read single input line");
         puts ("            -R                                Read all eight input lines at once");
         puts ("            -io pin INPUT|OUTPUT              Set direction of line");
         puts ("            -pullup pin UP|DOWN               Set pullup resister on individual input line");
         puts ("            -pins I|O[p-]...                  Set pin direction (I|O), pullup (p), and polarity(-)");
      }
   } // End of second pass through arguments list

   chip.end();
}
