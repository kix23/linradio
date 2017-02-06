/* TCL/TK bindings for liblinradio.so */
/* (C) 1999-2000 <pab@users.sourceforge.net> */

#include <stdlib.h>
#include <string.h>

#include "tcl.h"
#include "wrlib/wrapi.h"

typedef struct WR WR;

static int fail(Tcl_Interp *interp, char *s) {
  Tcl_SetResult(interp, s, TCL_VOLATILE);
  return TCL_ERROR;
}
static int retlong(Tcl_Interp *interp, long x) {
  char buf[32];
  sprintf(buf, "%ld", x);
  Tcl_SetResult(interp, buf, TCL_VOLATILE);
  return TCL_OK;
}

static int TclWR_radio(ClientData data, Tcl_Interp *interp,
		       int argc, char **argv) {
  static char *modenames[] = {
    "CW", "AM", "FMN", "FMW", "LSB", "USB", "FMM", "FM6"
  };
  int wr = (int)data;
  int i;

  if ( argc == 1 ) {
    /* Report current state */
    char buf[256];
    sprintf(buf,
	    "-p %d -f %ld -u %d "
	    "-a %d -v %d "
	    "-m %s "
	    "-b %d -i %d "
	    "-A %d -ifg %d",
	    GetPower(wr), GetFrequency(wr), GetMute(wr),
	    GetAtten(wr), GetVolume(wr)*100/GetMaxVolume(wr),
	    modenames[GetMode(wr)],
	    GetBFOOffset(wr), GetIFShift(wr),
	    GetAGC(wr), GetIFGain(wr));
    Tcl_SetResult(interp, buf, TCL_VOLATILE);
    return TCL_OK;
  }

  for ( i=1; i<argc; ++i ) {
    if ( !strcmp(argv[i], "-f") && i+1<argc )
      SetFrequency(wr, strtod(argv[++i], NULL));
    else if ( !strcmp(argv[i], "f") )
      retlong(interp, GetFrequency(wr));

    else if ( !strcmp(argv[i], "-p") && i+1<argc )
			SetPower(wr, atoi(argv[++i]));
    else if ( !strcmp(argv[i], "p") )
      retlong(interp, GetPower(wr));

    else if ( !strcmp(argv[i], "-u") && i+1<argc )
      SetMute(wr, atoi(argv[++i]));
    else if ( !strcmp(argv[i], "u") )
      retlong(interp, GetMute(wr));

    else if ( !strcmp(argv[i], "-A") && i+1<argc )
      SetAGC(wr, atoi(argv[++i]));
    else if ( !strcmp(argv[i], "A") )
      retlong(interp, GetAGC(wr));

    else if ( !strcmp(argv[i], "-a") && i+1<argc )
      SetAtten(wr, atoi(argv[++i]));
    else if ( !strcmp(argv[i], "a") )
      retlong(interp, GetAtten(wr));

    else if ( !strcmp(argv[i], "-v") && i+1<argc )
      SetVolume(wr, (atoi(argv[++i])*GetMaxVolume(wr)+99)/100);
    else if ( !strcmp(argv[i], "v") )
      retlong(interp, GetVolume(wr)*100/GetMaxVolume(wr));

    else if ( !strcmp(argv[i], "-m") && i+1<argc ) {
      int m;
      ++i;
      for ( m=0; m<sizeof(modenames)/sizeof(char*); ++m )
	if ( !strcmp(argv[i], modenames[m]) ) { SetMode(wr, m); break; }
      if ( m == sizeof(modenames)/sizeof(char*) )
	return fail(interp, "radio modes: {CW|AM|FMN|FMW|LSB|USB|FMM|FM6}");
    }
    else if ( !strcmp(argv[i], "m") )
      Tcl_SetResult(interp, modenames[GetMode(wr)], TCL_STATIC);

    else if ( !strcmp(argv[i], "-b") && i+1<argc )
      SetBFOOffset(wr, strtod(argv[++i], NULL));
    else if ( !strcmp(argv[i], "b") )
      retlong(interp, GetBFOOffset(wr));

    else if ( !strcmp(argv[i], "-i") && i+1<argc )
      SetIFShift(wr, strtod(argv[++i], NULL));
    else if ( !strcmp(argv[i], "i") )
      retlong(interp, GetIFShift(wr));

    else if ( !strcmp(argv[i], "-ifg") && i+1<argc )
      SetIFGain(wr, strtod(argv[++i], NULL));
    else if ( !strcmp(argv[i], "ifg") )
      retlong(interp, GetIFGain(wr));
    else if ( !strcmp(argv[i], "maxifg") )
      retlong(interp, GetMaxIFGain(wr));

    else if ( !strcmp(argv[i], "version") )
      Tcl_SetResult(interp, GetDescr(wr), TCL_VOLATILE);

    else if ( !strcmp(argv[i], "ss") )
      retlong(interp, GetSignalStrength(wr));
    else fail(interp, "radio controls: [-f <freq>][-m <mode>]"
	      "[-v <vol>][-u <mute>][-b <bfo>][-i <ifshift>][-p <power>]"
	      "[-A <AGC>]"
	      " [f][m][v][u][b][p] [ss]"" [version]");
  }

  return TCL_OK;
}

static int TclWR_winradio(ClientData data, Tcl_Interp *interp,
			  int argc, char **argv) {
  int wr;
  int port;

  if ( argc < 3 ) return fail(interp, "usage: winradio <name> <port>");

  port = strtol(argv[2], NULL, 0);
  if ( port>=0 && port<=3 ) ;
  else if ( port>=0x180 && port<0x1C0 ) ;
  else return fail(interp, "winradio: unrecognised port");
  
  wr = OpenRadioDevice(port);
  if ( !wr ) {
    Tcl_SetResult(interp, "Device not found", TCL_STATIC);
    return TCL_ERROR;
  }

  Tcl_CreateCommand(interp, argv[1], TclWR_radio, (ClientData)wr, NULL);

  return TCL_OK;
}

int TclWR_Init(Tcl_Interp *interp) {
  Tcl_CreateCommand(interp, "winradio", TclWR_winradio, NULL, NULL);
  return TCL_OK;
}
