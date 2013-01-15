#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <MCP3008.h>

int main(int argc, char *argv[])
{
   MCP3008 adc;
   const char *device = "/dev/spidev0.0";
   int channel = 0;
   int speed = 1000000;
   int input_mode = MCP3008::INPUT_MODE_SINGLE;
   int value;

   while (1)
   {
      static const struct option lopts[] = {
                  { "device",    1, 0, 'd' },
                  { "channel",   1, 0, 'c' },
                  { "speed",     1, 0, 's' },
                  { "differential", 0, 0, 'D' },
                  { "single",    0, 0, 'S' },
                  { "help",      0, 0, '?' },
                  { NULL,        0, 0, 0 } };
      int c;
     
      c = getopt_long(argc, argv, "d:c:s:DS?", lopts, NULL);
      if (c == -1)
         break;
     
      switch (c)
      {
      case 'd':
         device = optarg;
         break;

      case 'c':
         channel = atoi(optarg);
         break;

      case 's':
         speed = atoi(optarg);
         break;

      case 'D':
         input_mode = MCP3008::INPUT_MODE_DIFFERENTIAL;
         break;

      case 'S':
         input_mode = MCP3008::INPUT_MODE_SINGLE;
         break;

      case '?':
      default:
         puts("Usage: MCP3008-test [options]");
         puts("   Options: -d --device device_name");
         puts("            -c --channel input_channel");
         puts("            -s --speed speed");
         puts("            -D --differential");
         puts("            -S --single");
         puts("            -? --help");
         exit(1);
      }
   }

   if (!adc.begin(device, speed))
      exit(1);

   if ((value = adc.getValue(channel, input_mode)) >= 0)
      printf ("Input value: %d\n", value);

   adc.end();
}
