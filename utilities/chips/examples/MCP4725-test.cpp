#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <MCP4725.h>

int main(int argc, char *argv[])
{
   MCP4725 dac;
   const char *device = "/dev/i2c-1";
   int address = 2;
   int value = 0;
   int powerdown_mode = 0;
   bool persist = false;

   while (1)
   {
      static const struct option lopts[] = {
                  { "device",    1, 0, 'd' },
                  { "address",   1, 0, 'a' },
                  { "value",     1, 0, 'v' },
                  { "persist",   0, 0, 'p' },
                  { "powerdown", 1, 0, 'P' },
                  { "help",      0, 0, '?' },
                  { NULL,        0, 0, 0 } };
      int c;
     
      c = getopt_long(argc, argv, "D:a:p", lopts, NULL);
      if (c == -1)
         break;
     
      switch (c)
      {
      case 'd':
         device = optarg;
         break;

      case 'a':
         address = atoi(optarg);
         break;

      case 'v':
         value = atoi(optarg);
         break;

      case 'p':
         persist = true;
         break;

      case 'P':
         powerdown_mode = atoi(optarg);
         break;

      case '?':
      default:
         puts("Usage: MCP4725-test [options]");
         puts("   Options: -d --device device_name");
         puts("            -a --address i2c_address");
         puts("            -v --value value");
         puts("            -p --persist");
         puts("            -P --powerdown mode");
         puts("            -? --help");
         exit(1);
      }
   }

   if (!dac.begin(device, address))
      exit(1);

   if (powerdown_mode == 0)
      dac.setValue(value, persist);
   else
      dac.powerDown(powerdown_mode, persist);

   dac.end();
}
