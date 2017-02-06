#include "wrapi.h"

#ifndef _WRMISC_H_
#define _WRMISC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Define DEBUG to enable tracing to stdout/kmsg. */
/* #define DEBUG */

/* Define one of these to speedup serial comms (default:9600b/s) */
/* #define WRSERIAL_115200 */
#define WRSERIAL_38400

/**********************************************************************/

#ifdef __KERNEL__
#  define PRINT(x) printk x
#else
#  define PRINT(x) printf x; fflush(stdout)
#endif

#ifdef DEBUG
#  define TRACE(x) PRINT(x)
#else
#  define TRACE(x) { }
#endif

typedef int (* GETSLEVELPROC)(int);
typedef BOOL (* SETFREQPROC)(int, double, double *);
typedef BOOL (* SETMODEPROC)(int, int);
typedef BOOL (* SETBFOPROC)(int, int);
typedef BOOL (* SETAGCPROC)(int, BOOL);
typedef BOOL (* SETIFGAINPROC)(int, int);

typedef struct _RADIOSETTINGS		/*  stored settings for each device */
{
	RADIOINFO	riInfo;

	double		ftFreqHz;			/*  desired frequency in Hz */
	double		ftActFreq;			/*  actual frequency set */
	DWORD		dwFreq;				/*  frequency set from SetFrequency */
	int 		iFreqErr;			/*  difference between desired freq and actual freq */

	int			iCurMode;			/*  current mode */
	int			iCurVolume;			/*  current volume level */
	BOOL		fCurMute;
	BOOL		fCurAtten;
	BOOL		fCurPower;
	int			iCurBfo;
	int			iCurIfShift;
	BOOL		fCurAgc;
	int			iCurIfGain;

	BOOL		fInitVolume;		/*  TRUE if volume pot need reseting */
	BOOL		fLastMute;			/*  last mute state */

	DWORD		dwRefFreq;			/*  usually 12.8 or 25.6 MHz */
	double		ftIfXOverFreq;
	int			iShfState;			/*  microwave convertor state */

	int			iPort;
	int			iIrq;				/*  IRQ number (if used!) */
	int fd;           /* File descriptor for serial (PB) */

	/*  The following contains platform dependant fields */

	WORD		wIoAddr;			/*  ISA or serial port address */

	GETSLEVELPROC 	lpGetSLevelProc;
	SETFREQPROC     lpSetFreqProc;
	SETMODEPROC		lpSetModeProc;
	SETBFOPROC		lpSetBfoProc;
	SETAGCPROC		lpSetAgcProc;
	SETIFGAINPROC	lpSetIfGainProc;

} RADIOSETTINGS, *PRADIOSETTINGS,  *LPRADIOSETTINGS;

#define MAX_DEVICES 8

extern PRADIOSETTINGS RadioSettings[MAX_DEVICES+1];
#ifdef __KERNEL__
extern RADIOSETTINGS RadioSettings_static[MAX_DEVICES+1];
#endif

BOOL SetFreq(int, double, double*);	/*  located in wrcmd.c */
BOOL ResetRadio(int);		

#ifndef NULL
#  define NULL 0
#endif

#ifdef __cplusplus
}
#endif

#endif 
