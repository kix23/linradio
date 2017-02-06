#include "wrdef.h"
#include "wrio.h"
#include "wrapi.h"

/* WR2000 : last byte is now useless ? */
static const BYTE Modes[] = {0x5e, 0x5f, 0x60, 0x61, 0x5e, 0x5e, 0x61, 0x60};

BOOL SetMode1000(int hRadio, int iMode)
{
	LPRADIOSETTINGS rs = RadioSettings[hRadio];

	if (!WriteMcuByte(hRadio, Modes[iMode]))
		return FALSE;

	/*  if was or will be in a mode that does frequency mangling, update the frequency */
	if ((iMode == RMD_CW) || (iMode == RMD_LSB) ||
		(iMode == RMD_USB) || (iMode == RMD_FMW) ||
		(rs->iCurMode == RMD_CW) || (rs->iCurMode == RMD_LSB) ||
		(rs->iCurMode == RMD_USB) || (rs->iCurMode == RMD_FMW))
	{
		int oMode = rs->iCurMode;
		rs->iCurMode = iMode;

		if (((oMode == RMD_CW) || (oMode == RMD_LSB) || (oMode == RMD_USB)) &&
			!((iMode == RMD_CW) || (iMode == RMD_LSB) || (iMode == RMD_USB)))
			rs->lpSetBfoProc(hRadio, 0);	/*  reset BFO if not in SSB anymore */

		SetFrequency(hRadio, rs->dwFreq);
	}

	/*  set narrow filter if in SSB mode */
	if (rs->riInfo.wHWVer >= RHV_1500)
		WriteMcuByte(hRadio, ((iMode == RMD_CW) || (iMode == RMD_LSB) ||
			(iMode == RMD_USB)) ? 0x6b : 0x6c);

	rs->iCurMode = iMode;

	return TRUE;
}

/* WR2000 TODO: bits 8-15 of iMode may contain RBW_6k. */
/* WR2000 TODO: Obtain the value of RBW_6k. */
BOOL SetMode2000(int hRadio, int iMode)
{
	LPRADIOSETTINGS rs = RadioSettings[hRadio];
	int select;

	if (!WriteMcuByte(hRadio, Modes[iMode&255]))
		return FALSE;

	select = iMode >= RMD_FMM;
	/* WR2000 select = (iMode&255)==RMD_FMM || iMode==(RMD_FMN|RBW_6k); */
	if (!WriteMcuByte(hRadio, select ? 0x59 : 0x58))
		return FALSE;

	/*  if was or will be in a mode that does frequency mangling, update the frequency */
	if (rs->riInfo.wHWVer >= RHV_1500)
	{
		/* WR2000 TODO mask RBW_6k */
		if ((iMode == RMD_CW) || (iMode == RMD_LSB) ||
			(iMode == RMD_USB) || (iMode == RMD_FMW) ||
				(iMode == RMD_AM) || /* WR2000 */
			(rs->iCurMode == RMD_CW) || (rs->iCurMode == RMD_LSB) ||
			(rs->iCurMode == RMD_USB) || (rs->iCurMode == RMD_FMW)
				|| (rs->iCurMode == RMD_AM) /* WR2000 */ )
		{
			int oMode = rs->iCurMode;
			rs->iCurMode = iMode;
			/* WR2000 TODO mask RBW_6k */
			if (((oMode == RMD_CW) || (oMode == RMD_LSB) || (oMode == RMD_USB)) &&
				!((iMode == RMD_CW) || (iMode == RMD_LSB) || (iMode == RMD_USB)))
				rs->lpSetBfoProc(hRadio, 0);	/*  reset BFO if not in SSB anymore */

			SetFrequency(hRadio, rs->dwFreq);
		}

		if (!(rs->riInfo.dwFeatures & RIF_AGC))
			rs->lpSetAgcProc(hRadio, iMode != RMD_FMN); /* WR2000 TODO mask RBW_6k */

		/*  set narrow filter if in SSB mode */
		/* WR2000 TODO mask RBW_6k, match on RMD_AM|RBW_2500 too */
		WriteMcuByte(hRadio, ((iMode == RMD_CW) || (iMode == RMD_LSB) ||
			(iMode == RMD_USB)) ? 0x70 : 0x71);
	}
	rs->iCurMode = iMode;

	return TRUE;
}
