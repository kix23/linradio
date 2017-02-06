#include "wrdef.h"
#include "wrio.h"
#include "wrapi.h"

BOOL SetAgc1000(int hRadio, BOOL fAgc)
{
	return FALSE;	/*  not supported at all by series I receivers. */
}


/* WR2000 TODO: update this */
BOOL SetAgc2000(int hRadio, BOOL fAgc)
{
	LPRADIOSETTINGS rs = RadioSettings[hRadio];
	BYTE buf[4];
	int bufidx = 0;

	rs->fCurAgc = fAgc;

	if (rs->riInfo.dwFeatures & RIF_USVERSION)
		buf[bufidx++] = 0x78;
	else
	{
		buf[bufidx++] = 0x6c;
		buf[bufidx++] = 0x6d;
	}

	buf[bufidx] = ((rs->iCurMode == RMD_CW) || (rs->iCurMode == RMD_LSB) ||
		(rs->iCurMode == RMD_USB)) ? 0xc0 : 0xc2;

	if (!fAgc)
		buf[bufidx] |= 1;

	if (!(rs->riInfo.dwFeatures & RIF_USVERSION))
		buf[++bufidx] = 0x6b;

	return McuTransfer(hRadio, bufidx + 1, buf, 0, NULL);
}

/* -------------------------------------------------------------------------- */
/*  */
/*   SetIFGain */
/*  */
/* -------------------------------------------------------------------------- */

BOOL SetIfGain1000(int hRadio, int iIfGain)
{
	return FALSE;
}

/* WR2000 TODO: update this */
BOOL SetIfGain2000(int hRadio, int iIfGain)
{
	LPRADIOSETTINGS rs = RadioSettings[hRadio];
	long v;

	rs->iCurIfGain = iIfGain;

	v = (long)iIfGain * 32767 / 100;

	return (WriteMcuByte(hRadio, 0x0b) &&
			WriteMcuByte(hRadio, v >> 8) &&
			WriteMcuByte(hRadio, v & 0xff));
}
