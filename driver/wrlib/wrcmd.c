#if defined(__linux__) && !defined(__KERNEL__)
#  include <stdio.h>
#  include <string.h>
#  include <time.h>
#  include <math.h>
#  include <sys/io.h>
#  include <malloc.h>
#endif /* __linux__ && !__KERNEL__ */

#if defined(__linux__) && defined(__KERNEL__)
#  include <linux/string.h>
#  include <linux/kernel.h>
#  include <math.h>
#endif /* __linux__ && __KERNEL__ */

#ifdef __FreeBSD__
#  include <stdio.h>
#  include <string.h>
#  include <time.h>
#  include <math.h>
#  include <stdlib.h>
#  include <fcntl.h>
#  include <sys/param.h>
#  include <machine/cpufunc.h>
#endif /* __FreeBSD__ */

#include "wrdef.h"
#include "wrio.h"
#include "wrserial.h"

#ifdef __FreeBSD__
static int dev_io_fd = 0;
#endif

/*  This file contains a subset of the WRAPI functions. */
/*  These functions should not require any modifications for other platforms. */

/*
static int IsValidAddress(int addr)
{
	int a = (addr - 0x180) / 8;
	return ((a >= 0) && (a <= 7) && ((addr & 7) == 0));
}
*/

/* -------------------------------------------------------------------- */
/*  */
/*  OpenRadioDevice */
/*  */
/* 	iPort = 0-3 for a serial port or 0x180-0x1b8 (in increments of 8) */
/* 		for an ISA device. */
/*  */
/* 	iIrq = an optional interrupt number for	a serial port interrupt */
/* 		handler, 0 = default. */
/*  */
/* 	Returns hRadio to be used in the other functions. */
/*  */
/* -------------------------------------------------------------------- */

int OpenRadioDevice(WORD iPort)
{
	int i;
	{
		/*  manual specification */
		if ((iPort >= 4) && (((iPort & 7) != 0) || (iPort < 0x180) || (iPort > 0x1b8)))
			return 0;	/*  invalid port specified */

		/*  make sure device is not already open... */
		for (i = 1; i <= MAX_DEVICES; i++)
			if ((RadioSettings[i]) && (RadioSettings[i]->iPort == iPort))
				return FALSE;

		/*  find next available RadioSettings entry */
		i = 1;
		while ((i <= MAX_DEVICES) && (RadioSettings[i]))
			i++;

		if (i > MAX_DEVICES)
			return 0;	/*  no more entries available (increase MAX_DEVICES) */

#ifndef __KERNEL__
		RadioSettings[i] = (PRADIOSETTINGS)malloc(sizeof(RADIOSETTINGS));
#else
		RadioSettings[i] = &RadioSettings_static[i];
#endif

		if (!RadioSettings[i])
			return 0;

		RadioSettings[i]->iPort = iPort;

		if (iPort < 4)	/*  serial port */
		{
			RadioSettings[i]->riInfo.iHWInterface = RHI_SERIAL;
			if (OpenSerialPort(i))
			{
			  if (ResetRadio(i))
			    return i;
				CloseSerialPort(i);
			}
		}
		else
		{
			RadioSettings[i]->riInfo.iHWInterface = RHI_ISA;
			RadioSettings[i]->wIoAddr = iPort;
#if defined(__linux__) && !defined(__KERNEL__)
			{
			  int err = ioperm(iPort, 8, 1);
			  if ( err ) {
			    perror("I/O port");
			    return FALSE;
			  }
			}
#endif /* __linux__ && !__KERNEL__ */

#ifdef __FreeBSD__
			/* FreeBSD requires having /dev/io open in order */
			/* to be able to use inb() and outb(). */
			if (dev_io_fd == 0)
			  dev_io_fd = open("/dev/io", O_RDWR);
			if (dev_io_fd < 0)
			  return FALSE;
#endif /* __FreeBSD__ */
			if (ResetRadio(i))
			  return i;
		}
	}
	/*  not successful, clear RadioSettings[i] and return false */
#ifndef __KERNEL__
	free(RadioSettings[i]);
#endif
	RadioSettings[i] = NULL;
	return 0;
}

/* -------------------------------------------------------------------------- */
/*  */
/*   CloseRadioDevice */
/*  */
/* -------------------------------------------------------------------------- */

/*  DumpSettings stores the current settings in the receiver's MCU. */
/*  It allows rapid identification and operation of the receiver in successive */
/*  operations. The settings are lost if the receiver is switched off. */

static void DumpSettings(int hRadio)
{
#if 0
	LPRADIOSETTINGS rs = RadioSettings[hRadio];
	BYTE checksum = 0, buf[15];
	int i;

	memset(buf, 0, sizeof(buf));

	/*  store the current frequency first... */
	buf[0] = rs->dwFreq;
	buf[1] = rs->dwFreq >> 8;
	buf[2] = rs->dwFreq >> 16;
	buf[3] = rs->dwFreq >> 24;
	/*  then the mode... */
	buf[4] = rs->iCurMode;
	/*  flags... */
	if (rs->fCurAtten)
		buf[5] |= 0x01;
	if (rs->fCurMute)
		buf[5] |= 0x02;
	if (rs->fLastMute)
		buf[5] |= 0x04;
	if (rs->fCurAgc)
		buf[5] |= 0x08;
	if (rs->dwRefFreq == 25600000L)
		buf[5] |= 0x40;
	if (rs->riInfo.dwFeatures & RIF_USVERSION)
		buf[5] |= 0x80;
	/*  BFO or IF shift depending on the receiver */
	if ((rs->riInfo.wHWVer <= RHV_1000b) || ((rs->iCurMode == RMD_CW) &&
		!(rs->riInfo.dwFeatures & RIF_CWIFSHIFT)))
	{
		buf[6] = rs->iCurBfo;
		buf[7] = rs->iCurBfo >> 8;
	} else
	{
		buf[6] = rs->iCurIfShift;
		buf[7] = rs->iCurIfShift >> 8;
	}
	/*  IF gain */
	buf[8] = rs->iCurIfGain;
	/*  Hardware version */
	buf[13] = rs->riInfo.wHWVer;
	buf[14] = rs->riInfo.wHWVer >> 8;

	for (i = 0; i < 15; i++)
	{
		WriteMcuByte(hRadio, 0x13);
		WriteMcuByte(hRadio, 0xb0 + i);
		WriteMcuByte(hRadio, buf[i]);
		checksum += buf[i];	/*  generate checksum */
	}
	WriteMcuByte(hRadio, 0x13);
	WriteMcuByte(hRadio, 0xbf);
	WriteMcuByte(hRadio, checksum);
#endif /* 0 */
}

BOOL CloseRadioDevice(int hRadio)
{
	if (!ValidateHandle(hRadio, NULL))
		return FALSE;

	DumpSettings(hRadio);

	if (RadioSettings[hRadio]->riInfo.iHWInterface == RHI_SERIAL)
	{
		/*  drop baud-rate back to 9600 */
		WriteMcuByte(hRadio, 0xae);
		WriteMcuByte(hRadio, 12);

		CloseSerialPort(hRadio);
	}

#ifndef __KERNEL__
	free(RadioSettings[hRadio]);
#endif
	RadioSettings[hRadio] = NULL;
	return TRUE;
}

/* -------------------------------------------------------------------------- */
/*  */
/*   GetRadioDeviceInfo */
/*  */
/* -------------------------------------------------------------------------- */

int GetRadioDeviceInfo(int hRadio, LPRADIOINFO lpInfo)
{
	LPRADIOSETTINGS rs;

	if (!ValidateHandle(hRadio, &rs))
		return -1;

	if (lpInfo->dwSize > sizeof(RADIOINFO))
		lpInfo->dwSize = sizeof(RADIOINFO);

	memcpy((void *)&lpInfo->dwFeatures, (void *)&rs->riInfo.dwFeatures,
		lpInfo->dwSize - sizeof(DWORD));

	return GetMcuStatus(hRadio);
}

/* -------------------------------------------------------------------------- */
/*  */
/*   GetSignalStrength */
/*  */
/* -------------------------------------------------------------------------- */

int GetSignalStrength(int hRadio)
{
	if (!ValidateHandle(hRadio, NULL))
		return 0;

	return RadioSettings[hRadio]->lpGetSLevelProc(hRadio);
}


static const struct _BFOCONSTANTS
{
	int X; int Y; int W1; int W2;
} BfoConstants[3] = {
		{ -13000, 500, -11900, 3800 },	/*  series I receivers */
		{ -13000, 500, -16500, 3500 },	/*  series II receivers */
		{ -13000, 500, -14200, 1200 }};	/*  series II with SSB filter */

/* -------------------------------------------------------------------------- */
/*  */
/*   Set/GetFrequency */
/*  */
/* -------------------------------------------------------------------------- */

BOOL SetFrequency(int hRadio, DWORD dwFreq)
{
	LPRADIOSETTINGS rs;
	double dFreq, f;

	if (!ValidateHandle(hRadio, &rs))
		return FALSE;

	if (dwFreq & RFQ_X10)	/*  if MSB set, multiply low 31 bits by 10 */
		dFreq = 10.0 * (dwFreq & 0x7fffffffL);
	else
		dFreq = (double)dwFreq;

	/*  check to make sure frequency is within valid range */
	if ((dFreq < rs->riInfo.dwMinFreq) ||
		((dFreq / 1e3) > rs->riInfo.dwMaxFreqkHz))
		return FALSE;

	/*  check for US restricted frequencies */
	if ((rs->riInfo.dwFeatures & RIF_USVERSION) &&
		(((dFreq > 825e6) && (dFreq < 849e6)) ||
		 ((dFreq > 870e6) && (dFreq < 894e6))))
		return FALSE;

	rs->dwFreq = dwFreq;
	rs->ftFreqHz = dFreq;

	/*  if in a mode that uses IF-shift, call SetIFShift to set the frequency */
	if ((rs->iCurMode == RMD_LSB) || (rs->iCurMode == RMD_USB) ||
		((rs->iCurMode == RMD_CW) && (rs->riInfo.dwFeatures & RIF_CWIFSHIFT)))
		return SetIFShift(hRadio, rs->iCurIfShift);

	if (rs->lpSetFreqProc(hRadio, dFreq, &f))
	{
		rs->iFreqErr = (int)(f - dFreq + 0.5);
		if (rs->iCurMode == RMD_CW)	/*  update BFO to compensate for freq error */
			return SetBFOOffset(hRadio, rs->iCurBfo);
	}
	else
	{
		rs->iFreqErr = 0;
		return FALSE;
	}

	return TRUE;
}

DWORD GetFrequency(int hRadio)
{
	if (ValidateHandle(hRadio, NULL))
		return RadioSettings[hRadio]->dwFreq;
	else
		return -1;
}

/* -------------------------------------------------------------------------- */
/*  */
/*   Set/GetMode */
/*  */
/* -------------------------------------------------------------------------- */

BOOL SetMode(int hRadio, int iMode)
{
	LPRADIOSETTINGS rs;

	if (!ValidateHandle(hRadio, &rs))
		return FALSE;

	/*  validate iMode parameter */
	if ((iMode < 0) || (iMode >= rs->riInfo.iNumModes))
		return FALSE;

	return rs->lpSetModeProc(hRadio, iMode);
}

int GetMode(int hRadio)
{
	if (ValidateHandle(hRadio, NULL))
		return RadioSettings[hRadio]->iCurMode;
	else
		return -1;
}

/* -------------------------------------------------------------------------- */
/*  */
/*   Set/GetVolume */
/*  */
/* -------------------------------------------------------------------------- */

int GetMaxVolume(int hRadio) {
  return RadioSettings[hRadio]->riInfo.iMaxVolume;
}

BOOL SetVolume(int hRadio, int iVol)
{
	static const BYTE InitVolCode[] = {0x6a, 0x1f, 0x69, 0x00};
	LPRADIOSETTINGS rs;

	if (!ValidateHandle(hRadio, &rs))
		return FALSE;

	/*  validate the value of iVol */
	if ((iVol < 0) || (iVol > RadioSettings[hRadio]->riInfo.iMaxVolume))
		return FALSE;

	/*  initialize volume control if required */
	if (rs->fInitVolume)
	{
		McuTransfer(hRadio, sizeof(InitVolCode), (LPBYTE)InitVolCode, 0, NULL);
		rs->fInitVolume = FALSE;
	}
	rs->iCurVolume = iVol;

	return (WriteMcuByte(hRadio, 0x69) && WriteMcuByte(hRadio, iVol));
}

int GetVolume(int hRadio)
{
	if (ValidateHandle(hRadio, NULL))
		return RadioSettings[hRadio]->iCurVolume;
	else
		return -1;
}

/* -------------------------------------------------------------------------- */
/*  */
/*   Set/GetAtten */
/*  */
/* -------------------------------------------------------------------------- */

BOOL SetAtten(int hRadio, BOOL fAtten)
{
	if (!ValidateHandle(hRadio, NULL))
		return FALSE;

	RadioSettings[hRadio]->fCurAtten = fAtten;

	if (fAtten)
		return WriteMcuByte(hRadio, 0x56);
	else
		return WriteMcuByte(hRadio, 0x57);
}

BOOL GetAtten(int hRadio)
{
	if (ValidateHandle(hRadio, NULL))
		return RadioSettings[hRadio]->fCurAtten;
	else
		return FALSE;
}

/* -------------------------------------------------------------------------- */
/*  */
/*   Set/GetMute */
/*  */
/* -------------------------------------------------------------------------- */

static BOOL UpdateMute(int hRadio)
{
	LPRADIOSETTINGS rs = RadioSettings[hRadio];

	if ((rs->fCurMute || (!rs->fCurPower &&
		(rs->riInfo.iHWInterface == RHI_SERIAL))) ^ rs->fLastMute)
	{
		/*  only update mute status when it actually changes (minimises noise) */
		rs->fLastMute ^= 1;	/*  invert fLastMute */

		if (rs->fLastMute)
			return WriteMcuByte(hRadio, 0x51);
		else
			return WriteMcuByte(hRadio, 0x50);
	}
	return TRUE;
}

BOOL SetMute(int hRadio, BOOL fMute)
{
	if (!ValidateHandle(hRadio, NULL))
		return FALSE;

	RadioSettings[hRadio]->fCurMute = fMute;

	return UpdateMute(hRadio);
}

BOOL GetMute(int hRadio)
{
	if (ValidateHandle(hRadio, NULL))
		return RadioSettings[hRadio]->fCurMute;
	else
		return FALSE;
}

/* -------------------------------------------------------------------------- */
/*  */
/*   Set/GetPower */
/*  */
/* -------------------------------------------------------------------------- */

BOOL SetPower(int hRadio, BOOL fPower)
{
	BYTE b;
	long timeout;
	LPRADIOSETTINGS rs;

	if (!ValidateHandle(hRadio, &rs))
		return FALSE;

	rs->fCurPower = fPower;

	if (rs->riInfo.iHWInterface == RHI_SERIAL)
	{
		/*  mute or un-mute receiver if serially connected */
		return UpdateMute(hRadio);
	}

	if (fPower)	/*  turn power on */
	{
		WriteMcuByte(hRadio, 10);
		ReadMcuByte(hRadio, &b);
		if (b)
			return TRUE;	/*  already on */

		WriteMcuByte(hRadio, 8);
		rs->fCurPower = TRUE;

		/*  wait for unit to power up... */
		WriteMcuByte(hRadio, 10);

		timeout = GetTickCount() + 1000;	/*  wait up to one second */
		while (!ReadMcuByte(hRadio, &b)) {
		  if ( GetTickCount()-timeout > 0 ) {
		    TRACE(("setpower timeout\n"));
		    break;
		  }
		};

		WriteMcuByte(hRadio, 0);

		Delay(100);

		if (GetMcuStatus(hRadio) & 1)
			ReadMcuByte(hRadio, &b);	/*  if any data in output port, read it */

		/*  program PLLs if required */
		if ((rs->riInfo.wHWVer == RHV_1500) ||
			(rs->riInfo.wHWVer >= RHV_3100))
		{
			if ((rs->iCurMode != RMD_CW) &&
				(rs->iCurMode != RMD_LSB) &&
				(rs->iCurMode != RMD_USB))
				rs->lpSetBfoProc(hRadio, 0);
			SetFrequency(hRadio, rs->dwFreq);
		}

		/*  set volume control */
		rs->fInitVolume = TRUE;
		SetVolume(hRadio, rs->iCurVolume);
	}
	else
	{
		WriteMcuByte(hRadio, 9);
		Delay(200);
	}
	return TRUE;
}

BOOL GetPower(int hRadio)
{
	if (ValidateHandle(hRadio, NULL))
		return RadioSettings[hRadio]->fCurPower;
	else
		return FALSE;
}

/* -------------------------------------------------------------------------- */
/*  */
/*   Set/GetBFOOffset */
/*  */
/* -------------------------------------------------------------------------- */

BOOL SetBFOOffset(int hRadio, int iBFO)
{
	LPRADIOSETTINGS rs;

	if (!ValidateHandle(hRadio, &rs))
		return FALSE;

	/*  check BFO parameter and whether the function is supported */
	if ((iBFO < -rs->riInfo.iMaxBFO) ||
		(iBFO > rs->riInfo.iMaxBFO) ||
		(rs->riInfo.dwFeatures & RIF_CWIFSHIFT))
		return FALSE;

	rs->iCurBfo = iBFO;

	if (rs->riInfo.wHWVer >= RHV_1500)
	{
		int B;

		if (rs->iCurMode != RMD_CW)	/*  will be set when appropriate */
			return TRUE;

		B = iBFO - (BfoConstants[0].X - BfoConstants[0].W1 - BfoConstants[0].W2) / 2;
		if (rs->ftActFreq < rs->ftIfXOverFreq)
			B -= rs->iFreqErr;
		else
			B += rs->iFreqErr;

		return rs->lpSetBfoProc(hRadio, B);
	}

	if (rs->ftFreqHz < rs->ftIfXOverFreq)
		return rs->lpSetBfoProc(hRadio, -iBFO - rs->iFreqErr);
	else
		return rs->lpSetBfoProc(hRadio, iBFO + rs->iFreqErr);
}

int GetBFOOffset(int hRadio)
{
	if (ValidateHandle(hRadio, NULL))
		return RadioSettings[hRadio]->iCurBfo;
	else
		return 0;
}

/* -------------------------------------------------------------------------- */
/*  */
/*   Set/GetIFShift */
/*  */
/* -------------------------------------------------------------------------- */

BOOL SetIFShift(int hRadio, int iIfShift)
{
	LPRADIOSETTINGS rs;
	BOOL BelowIFXOver;
	long A, B=0;
	double f;
	int bci;	/*  BFO constant index */

	if (!ValidateHandle(hRadio, &rs))
		return FALSE;

	/*  check for IF shift support */
	if (!(rs->riInfo.dwFeatures & RIF_LSBUSB))
		return FALSE;

	/*  check supplied parameter */
	if ((iIfShift < -rs->riInfo.iMaxIFShift) ||
		(iIfShift > rs->riInfo.iMaxIFShift))
		return FALSE;

	rs->iCurIfShift = iIfShift;

	if ((rs->riInfo.wHWVer <= RHV_1500) || ((rs->riInfo.wHWVer >= RHV_3000) &&
		(rs->riInfo.wHWVer <= RHV_3100)))
			bci = 0;
	else
		if (rs->riInfo.wHWVer >= RHV_3150)
			bci = 1;
		else
			bci = 2;

	BelowIFXOver = rs->ftFreqHz < rs->ftIfXOverFreq;

	/*  establish frequency and BFO to perform desired IF shift */
	switch (rs->iCurMode)
	{
		case RMD_CW:
			{
				A = (BfoConstants[bci].X + BfoConstants[bci].W1 + BfoConstants[bci].W2) / 2;
				if (BelowIFXOver)
					A = -A;
				A += iIfShift;
				B = -(BfoConstants[bci].X - BfoConstants[bci].W1 - BfoConstants[bci].W2) / 2;
				break;
			}
		case RMD_LSB:
			{
				if (BelowIFXOver)
					A = -BfoConstants[bci].X - BfoConstants[bci].Y - BfoConstants[bci].W2;
				else
					A = BfoConstants[bci].W1 - BfoConstants[bci].Y;
				A -= iIfShift;
				break;
			}
		case RMD_USB:
			{
				if (BelowIFXOver)
					A = BfoConstants[bci].Y - BfoConstants[bci].W1;
				else
					A = BfoConstants[bci].X + BfoConstants[bci].Y + BfoConstants[bci].W2;
				A += iIfShift;
				break;
			}
		default:
			return TRUE;
	}

	/*  adjust the RX frequency */
	if (!rs->lpSetFreqProc(hRadio, rs->ftFreqHz + A, &f))
		return FALSE;

	rs->iFreqErr = (int)(f - (rs->ftFreqHz + A) + 0.5);

	if (rs->iCurMode == RMD_CW)
	{
		if (BelowIFXOver)
			B -= rs->iFreqErr + iIfShift;
		else
			B += rs->iFreqErr + iIfShift;
	}
	else
	{
		if (BelowIFXOver)
			B = -A - rs->iFreqErr - BfoConstants[bci].X;
		else
			B = A + rs->iFreqErr - BfoConstants[bci].X;
	}

	/*  set the BFO */
	return rs->lpSetBfoProc(hRadio, B);
}

int GetIFShift(int hRadio)
{
	if (ValidateHandle(hRadio, NULL))
		return RadioSettings[hRadio]->iCurIfShift;
	else
		return 0;
}

/* -------------------------------------------------------------------------- */
/*  */
/*   Set/GetAGC */
/*  */
/* -------------------------------------------------------------------------- */

BOOL SetAGC(int hRadio, BOOL fAgc)
{
	LPRADIOSETTINGS rs;

	if (!ValidateHandle(hRadio, &rs) || !(rs->riInfo.dwFeatures & RIF_AGC))
		return FALSE;

	return rs->lpSetAgcProc(hRadio, fAgc);
}

BOOL GetAGC(int hRadio)
{
	if (ValidateHandle(hRadio, NULL))
		return RadioSettings[hRadio]->fCurAgc;
	else
		return FALSE;
}

/* -------------------------------------------------------------------------- */
/*  */
/*   Set/GetIFGain */
/*  */
/* -------------------------------------------------------------------------- */

BOOL SetIFGain(int hRadio, int iIfGain)
{
	LPRADIOSETTINGS rs;

	if (!ValidateHandle(hRadio, &rs) || !(rs->riInfo.dwFeatures & RIF_IFGAIN) ||
		(iIfGain < 0) || (iIfGain > rs->riInfo.iMaxIFGain))
		return FALSE;

	return rs->lpSetIfGainProc(hRadio, iIfGain);
}

int GetIFGain(int hRadio)
{
	if (ValidateHandle(hRadio, NULL))
		return RadioSettings[hRadio]->iCurIfGain;
	else
		return -1;
}

int GetMaxIFGain(int hRadio)
{
	LPRADIOSETTINGS rs;

	if (!ValidateHandle(hRadio, &rs) || !(rs->riInfo.dwFeatures & RIF_IFGAIN))
		return FALSE;

	return rs->riInfo.iMaxIFGain;
}

char *GetDescr(int hRadio)
{
	if (ValidateHandle(hRadio, NULL))
		return RadioSettings[hRadio]->riInfo.descr;
	else
		return NULL;
}
