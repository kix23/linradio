/* ioctl stub for radio devices */
/* (C) 1999 <pab@users.sourceforge.net> */

/* Maps wrlib/wrapi.h calls to ioctls */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "wrlib/wrapi.h"

#include <sys/ioctl.h>
#include "radio_ioctl.h"

/*********************************************************************/

int OpenRadioDevice(WORD port) {
  char dev[32];
  int fd;
  if ( port>=0x180 && port<0x1C0 )
    sprintf(dev, "/dev/winradio%d", (port-0x180)/8);
  else if ( port<4 )
    sprintf(dev, "/dev/winradioS%d", port);
  else
    return 0;

  fd = open(dev, O_RDONLY, 0);
  if ( fd < 0 ) return 0;
  return fd;
}

static inline unsigned long ioget(int h, int cmd) {
  unsigned long tmp;
  if ( ioctl(h, cmd, &tmp) < 0 ) return -1;
  return tmp;
}
static inline BOOL ioset(int h, int cmd, unsigned long arg) {
  if ( ioctl(h, cmd, &arg) < 0 ) return 0;
  return 1;
}

int   GetSignalStrength(int h)     { return ioget(h, RADIO_GET_SS); }
BOOL  SetFrequency(int h, DWORD f) { return ioset(h, RADIO_SET_FREQ, f); }
BOOL  SetMode(int h, int m)        { return ioset(h, RADIO_SET_MODE, m); }
BOOL  SetVolume(int h, int v)      { return ioset(h, RADIO_SET_VOL, v); }
BOOL  SetAtten(int h, BOOL a)      { return ioset(h, RADIO_SET_ATTN, a); }
BOOL  SetMute(int h, BOOL u)       { return ioset(h, RADIO_SET_MUTE, u); }
BOOL  SetPower(int h, BOOL p)      { return ioset(h, RADIO_SET_POWER, p); }
BOOL  SetBFOOffset(int h, int b)   { return ioset(h, RADIO_SET_BFO, b); }
BOOL  SetIFShift(int h, int i)     { return ioset(h, RADIO_SET_IFS, i); }
BOOL  SetAGC(int h, BOOL a)        { return ioset(h, RADIO_SET_AGC, a); }
BOOL  SetIFGain(int h, int i)      { return ioset(h, RADIO_SET_IFG, i); }

DWORD GetFrequency(int h)          { return ioget(h, RADIO_GET_FREQ); }
int   GetMode(int h)               { return ioget(h, RADIO_GET_MODE); }
int   GetVolume(int h)             { return ioget(h, RADIO_GET_VOL); }
int   GetMaxVolume(int h)          { return ioget(h, RADIO_GET_MAXVOL); }
BOOL  GetAtten(int h)              { return ioget(h, RADIO_GET_ATTN); }
BOOL  GetMute(int h)               { return ioget(h, RADIO_GET_MUTE); }
BOOL  GetPower(int h)              { return ioget(h, RADIO_GET_POWER); }
int   GetBFOOffset(int h)          { return ioget(h, RADIO_GET_BFO); }
int   GetIFShift(int h)            { return ioget(h, RADIO_GET_IFS); }
BOOL  GetAGC(int h)                { return ioget(h, RADIO_GET_AGC); }
int   GetIFGain(int h)             { return ioget(h, RADIO_GET_IFG); }
int   GetMaxIFGain(int h)          { return ioget(h, RADIO_GET_MAXIFG); }

char *GetDescr(int h) {
  static char buf[100];
  if ( ioctl(h, RADIO_GET_DESCR, buf) < 0 ) return "?";
  return buf;
}
