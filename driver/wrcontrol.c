/* Test program for liblinradio.so */
/* (C) 1999-2000 <pab@users.sourceforge.net> */
/* Based on (C) 1997 Michael McCormack */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "wrlib/wrapi.h"

void fatal(char *a) { perror(a); exit(1); }

void usage(char *prog) {
  printf("usage: %s [-P port:0-3,0x180-0x1b8)] [-f freqHz] [-v vol] "
	 "[-u mute:0,1] [-a atten:0,1] [-A agc:0,1] "
	 "[-m 0=CW,1=AM,2=FMN,3=FMW,4=LSB,5=USB,6=FMN,7=FM6]\n", prog);
  exit(1);
}

int main(int argc, char **argv) {
  char **p;
  int wr;
  int port = 0x180;
  float sc_begin, sc_end, sc_step;

  for ( p=argv+1; p<argv+argc; p++ ) {
    if ( !strcmp(*p, "-P") && *++p ) port = strtol(*p, NULL, 0);
    else if ( !strcmp(*p, "-h") ) usage(argv[0]);
  }

  wr = OpenRadioDevice(port);
  if ( !wr ) fatal("error opening device");

  for ( p=argv+1; p<argv+argc; p++ ) {
    if ( !strcmp(*p, "-P") && *++p )
      ;
    else if ( !strcmp(*p, "-p") && *++p )
      SetPower(wr, strtol(*p, NULL, 0));
    else if ( !strcmp(*p, "-f") && *++p )
      SetFrequency(wr, atof(*p));
    else if ( !strcmp(*p, "-v") && *++p )
      SetVolume(wr, strtol(*p, NULL, 0));
    else if ( !strcmp(*p, "-u") && *++p )
      SetMute(wr, strtol(*p, NULL, 0));
    else if ( !strcmp(*p, "-a") && *++p )
      SetAtten(wr, strtol(*p, NULL, 0));
    else if ( !strcmp(*p, "-m") && *++p )
      SetMode(wr, strtol(*p, NULL, 0));
    else if ( !strcmp(*p, "-A") && *++p )
      SetAGC(wr, strtol(*p, NULL, 0));
    else if ( !strcmp(*p, "-scan") && *++p &&
	      sscanf(*p, "%f,%f,%f", &sc_begin, &sc_end, &sc_step)==3 ) {
      /* For benchmarking. */
      float f;
      printf("Scanning from %f to %f, step=%f\n", sc_begin, sc_end, sc_step);
      for ( f=sc_begin; f<sc_end; f+=sc_step ) {
	SetFrequency(wr, f);
	printf("%f: %d\n", f, GetSignalStrength(wr));
	fflush(stdout);
      }
      printf("\n");
    }
    else usage(argv[0]);
  }

  printf("descr=%s\n", GetDescr(wr));
  printf("port=0x%X power=%d freq=%u mode=%d vol=%d mute=%d atten=%d agc=%d\n",
	 port, GetPower(wr), (unsigned int)GetFrequency(wr), GetMode(wr),
	 GetVolume(wr), GetMute(wr), GetAtten(wr), GetAGC(wr));
  printf("SS=%d\n", GetSignalStrength(wr));

  return 0;
}
