/* TCL/TK bindings for liblinradio.so */
/* (C) 1999-2000 <pab@users.sourceforge.net> */

#include <tcl.h>

extern int TclWR_Init(Tcl_Interp *interp);

int Tcl_AppInit(Tcl_Interp *interp) {
  if ( Tcl_Init(interp) == TCL_ERROR ) return TCL_ERROR;
  
  if ( TclWR_Init(interp) == TCL_ERROR ) return TCL_ERROR;	
  Tcl_SetVar(interp, "tcl_rcFileName", "~/.tclradiorc", TCL_GLOBAL_ONLY);
  
  return TCL_OK;
}

int main(int argc, char **argv) {
  Tcl_Main(argc, argv, Tcl_AppInit);
  return 0;
}
