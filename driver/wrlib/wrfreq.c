#ifndef __KERNEL__
#  include <string.h>
#  include <stdlib.h>
#else
#  include <linux/kernel.h>
#  include <linux/string.h>
#endif
#include <math.h>
#include "wrdef.h"
#include "wrio.h"

#ifdef __KERNEL__
double modf(double x, double *y) {
  double z = x;
  *y = (int)z;
  return z - *y;
}
#endif

typedef DWORD MATRIX[2][2];

static int GetVcoParams(int hRadio, DWORD fvco, DWORD fref, int minR, int maxR, UINT *R, DWORD *Ntot)
{
	double Kn, Kp, af;
	MATRIX M, tM, cM;
	int i;
	UINT tR;
	DWORD tN, raf;

	Kn = (double)fvco / fref;
	M[0][0] = 0;
	M[0][1] = 1;
	M[1][0] = 1;
	M[1][1] = (long)Kn;
	Kp = modf(Kn, &Kn);
	memcpy(tM, M, sizeof(MATRIX));
    while ((Kp > 1e-6) && (M[0][1] < maxR)) 
	{
        Kn = 1 / Kp;
        tM[1][1] = (long)Kn;
        memcpy(cM, M, sizeof(MATRIX));
        M[0][0] = cM[0][0] * tM[0][0] + cM[0][1] * tM[1][0]; /*  M = cM * tM - matrix multiplication */
        M[0][1] = cM[0][0] * tM[0][1] + cM[0][1] * tM[1][1];
        M[1][0] = cM[1][0] * tM[0][0] + cM[1][1] * tM[1][0];
        M[1][1] = cM[1][0] * tM[0][1] + cM[1][1] * tM[1][1];
	    Kp = modf(Kn, &Kn);
    }
    if (M[0][1] > maxR)
		memcpy(M, cM, sizeof(MATRIX));
    *R = M[0][1];
    while (*R < minR) 
		*R += M[0][1];
    *Ntot = M[1][1] * *R / M[0][1];
    Kp = (double)*Ntot / *R;
    af = Kp * fref;
    raf = (long)(af + 0.5) - fvco;
	if (abs(raf) > 50)
		for (i = 1; i < 25; i++)
		{
            if (modf(Kp * i + 1e-10, &Kn) < 1e-6) 
			{
                Kn = 50 / i;
                if (raf > 0)
                    GetVcoParams(hRadio, (long)(af - Kn), fref, minR, maxR, &tR, &tN);
                else
                    GetVcoParams(hRadio, (long)(af + Kn + 1.0), fref, minR, maxR, &tR, &tN);
                fvco = (long)(tN / tR * fref + 0.5) - fvco;
                if (abs(raf) > abs(fvco)) 
				{
                    *R = tR;
                    *Ntot = tN;
                    raf = fvco;
                }
                break;
            }
        }
    return raf;
}



BOOL SetFreq1000(int hRadio, double freq, double *actfreq)
{
	static const double VcoOffsets[5] = {556.325e6, 249.125e6, 58.075e6, -249.125e6, -556.325e6};
	static const double FreqRanges[9] = {1.8e6, 30e6, 50e6, 118e6, 300e6, 513e6, 798e6, 1106e6, 21e9};
	static const double SmFilters[8] = {0.5e6, 0.95e6, 1.8e6, 3.5e6, 7.0e6, 14e6, 30e6, 118e6};

	static const BYTE l0data[2][9] = {
		{0x54, 0x54, 0x54, 0x58, 0x58, 0x98, 0xC0, 0x80, 0x40},
		{0x54, 0x54, 0x54, 0x54, 0x58, 0x98, 0xD0, 0x80, 0x40}};

	LPRADIOSETTINGS rs;
	DWORD Ntot;
	UINT R;
	double fvco, vcoofs;
	BYTE l0;
	int l0idx, range;
	BOOL PllOverride;
	int bufidx = 0;
	BYTE buf[32];

	if (!ValidateHandle(hRadio, &rs))
		return FALSE;

	PllOverride = (!(rs->riInfo.dwFeatures & RIF_USVERSION) &&
		((rs->riInfo.wHWVer == RHV_1500) || (rs->riInfo.wHWVer >= RHV_3100)));

	if (actfreq)
		*actfreq = freq;

	l0idx = (rs->riInfo.wHWVer < RHV_3000) ? 0 : 1;

	l0 = 0xf3;				/*  get current state of latch 0 */
	if (!McuTransfer(hRadio, 1, &l0, 1, &l0))
		return FALSE;

	for (range = 0; range < 9; range++)
		if (freq < FreqRanges[range])
			break;

	if (range > 4)
		vcoofs = VcoOffsets[range-4];
	else
		vcoofs = VcoOffsets[0];

	fvco = freq + vcoofs;

	/*  calculate PLL register values */

	if (rs->iCurMode == RMD_FMW)
	{
		R = rs->dwRefFreq / 20000;
		Ntot = (long)(fvco / 20e3 + 0.5);	/*  get closest freq to multiple of 20 kHz in FMW */
	} else
		GetVcoParams(hRadio, (long)(fvco + 0.5), rs->dwRefFreq,
								 rs->dwRefFreq / 20000, rs->dwRefFreq / 5000, &R, &Ntot);

	if (PllOverride)
		buf[bufidx++] = 0x58;	/*  activate -SETPLL line on non-US 1500s & 3100s */

	/*  set PLL C register */
	buf[bufidx++] = 0x6d;
	buf[bufidx++] = 0x2c;

	/*  calculate actual VCO frequency */
	fvco = (double)Ntot / R * rs->dwRefFreq - vcoofs;

	if (PllOverride)	/*  toggle -SETPLL line */
	{
		buf[bufidx++] = 0x59;
		buf[bufidx++] = 0x58;
	}

	/*  set PLL R register */
	R |= 0x4000;
	buf[bufidx++] = 0x6e;
	buf[bufidx++] = R & 0xff;
	buf[bufidx++] = R >> 8;

	if (PllOverride)	/*  toggle -SETPLL line */
	{
		buf[bufidx++] = 0x59;
		buf[bufidx++] = 0x58;
	}

	/*  set PLL A register */
	Ntot = 0x300000L + (Ntot & 63) + ((Ntot >> 6) << 8);
	buf[bufidx++] = 0x6f;
	buf[bufidx++] = Ntot & 0xff;
	buf[bufidx++] = (Ntot >> 8) & 0xff;
	buf[bufidx++] = Ntot >> 16;

	if (PllOverride)	/*  clear -SETPLL line */
		buf[bufidx++] = 0x59;

	if (rs->riInfo.dwFeatures & RIF_USVERSION)
		buf[bufidx++] = 0x68;	/*  initiate PLL programming if US version */

	/*  set mixer and band lines (latch 0) */
	buf[bufidx++] = 0xf0;
	buf[bufidx++] = (l0 & 0x23) | l0data[l0idx][range];

	if (!McuTransfer(hRadio, bufidx, buf, 0, NULL))
		return FALSE;

	if (rs->riInfo.dwFeatures & RIF_USVERSION)
	{
		/*  if US version, check to see if PLL/latch 0 combination was valid */
		l0 = 0x93;
		if (!McuTransfer(hRadio, 1, &l0, 1, &l0) || l0)
			return FALSE;		/*  invalid PLL params if l0 is non-zero */
	}

	/*  if Spectrum Monitor series, program HF filter board */
	if ((rs->riInfo.wHWVer >= RHV_3000) &&
		(rs->riInfo.wHWVer <= RHV_3100))
	{
		if (freq < 118e6)
		{
			for (range = 0; range < 8; range++)
				if (freq < SmFilters[range])
					break;

			if (range & 1)
				buf[0] = 0x54;
			else
				buf[0] = 0x55;

			if (range & 2)
				buf[1] = 0x52;
			else
				buf[1] = 0x53;

			if (range & 4)
				buf[2] = 0x77;
			else
				buf[2] = 0x76;
		}
		else
		{
			buf[0] = 0x77;

			if (freq >= 1106e6)
			{
				buf[1] = 0x53;
				buf[2] = 0x54;
			}
			else
				if (freq >= 798e6)
				{
					buf[1] = 0x52;
					buf[2] = 0x55;
				}
				else
				{
					buf[1] = 0x52;
					buf[2] = 0x54;
				}
		}
		if (!McuTransfer(hRadio, 3, buf, 0, NULL))
			return FALSE;
	}

	rs->ftActFreq = fvco;
	if (actfreq)
		*actfreq = fvco;

	return TRUE;
}

BOOL SetFreq2000(int hRadio, double freq, double *actfreq)
{
	/* WR2000 */
	static const BYTE ShfData[2][3][4] = {
		{ {0x04,0xa3,0x99,0x01}, {0x04,0x24,0x92,0x86}, {0x03,0xfc,0x92,0x4a} },
		{ {0x04,0xa3,0x81,0x8b}, {0x04,0x38,0x81,0x8c}, {0x03,0xe8,0x81,0x86} }
	};
	const double SHFFreq1= 2.65e9;
	const double SHFFreq2 = 2.55e9;

	static const double FreqRanges[16] = {0.5e6, 0.95e6, 1.8e6, 3.5e6, 7e6, 14e6, 30e6, 118e6, 309e6, 400e6, 513e6, 808e6, 808e6, 1114e6, 1200e6, 21e9};
	static const BYTE l0data[16] = {0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x80, 0x80, 0x80, 0x80, 0x40, 0x40};
	static const BYTE l1data[16] = {0x00, 0x00, 0x80, 0x40, 0x40, 0x40, 0x40, 0xc0, 0x20, 0x20, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0};

	LPRADIOSETTINGS rs;
	DWORD Ntot;
	UINT R;
	double fvco, vcoofs, tunedfreq;
	BYTE l0 = 0xf3, l1 = 0xf4;
	int range, newShfState;
	BYTE buf[24] = { 0, };
	int bufidx = 0;
	int c;

	if (!ValidateHandle(hRadio, &rs))
		return FALSE;

	if (actfreq)
		*actfreq = freq;

	tunedfreq = rs->ftFreqHz;
	newShfState = rs->iShfState;

	if (rs->riInfo.dwMaxFreqkHz > 2000000L)
	{
		if (freq > 2.6e9) /* WR2000 */
		{
			freq -= SHFFreq2;
			tunedfreq -= SHFFreq2;
			newShfState = 2;
		}
		else
		if (freq > 1.5e9) { /* WR2000 */
			freq = SHFFreq1 - freq;
			tunedfreq = SHFFreq1 - tunedfreq;
			newShfState = 1;
		}
		else
			newShfState = 0;
	}

	if (!McuTransfer(hRadio, 1, &l0, 1, &l0))	/*  get current state of latch 0 */
		return FALSE;
	if (!McuTransfer(hRadio, 1, &l1, 1, &l1))   /*  get current state of latch 1 */
		return FALSE;

	for (range = 0; range < sizeof(l0data); range++)
		if (tunedfreq < FreqRanges[range])
			break;

	if (tunedfreq < 400e6)
		vcoofs = 556.325e6;
	else if (tunedfreq < 808e6)
		vcoofs = 249.125e6;
	else if (tunedfreq < 1114e6)
		vcoofs = -249.125e6;
	else
		vcoofs = -556.325e6;

	fvco = freq + vcoofs;

	/*  calculate PLL register values */

	c = 0x2C;

#if 0 /* suppressed in WR2000 */
	if (rs->iCurMode == RMD_FMW)
	{
		R = rs->dwRefFreq / 20000;
		Ntot = (long)(fvco / 20e3 + 0.5);	/*  get closest freq to multiple of 20 kHz in FMW */
	} else
#endif /* WR2000 */
	GetVcoParams(hRadio, (long)(fvco + 0.5), rs->dwRefFreq, rs->dwRefFreq / 20000, rs->dwRefFreq / 5000, &R, &Ntot);

	/*  set PLL C register */
	/* WR2000 TODO: only if changed ? */
	buf[bufidx++] = 0x6d;
	buf[bufidx++] = c;
		
	/*  calculate actual VCO frequency */
	fvco = (double)Ntot / R * rs->dwRefFreq - vcoofs;

	/*  set PLL R register */
	R += 0x4000;
	/* WR2000 TODO: only if changed ? */
	buf[bufidx++] = 0x6e;
	buf[bufidx++] = R & 0xff;
	buf[bufidx++] = R >> 8;

	/*  set PLL A register */
	Ntot = 0x300000L + (Ntot & 63) + ((Ntot >> 6) << 8);
	/* WR2000 TODO: only if changed */
	buf[bufidx++] = 0x6f;
	buf[bufidx++] = Ntot & 0xff;
	buf[bufidx++] = (Ntot >> 8) & 0xff;
	buf[bufidx++] = Ntot >> 16;

	/*  set mixer and band lines (latch 0 & 1) */
	buf[bufidx++] = 0xf0;
	buf[bufidx++] = (l0 & 0x23) | l0data[range];
	buf[bufidx++] = 0xf1;
	buf[bufidx++] = (l1 & 0x1f) | l1data[range];

	if (!McuTransfer(hRadio, bufidx, buf, 0, NULL))
		return FALSE;

	if (rs->riInfo.dwFeatures & RIF_USVERSION)
	{
		/*  if US version, check to see if PLL/latch 0 combination was valid */
		l0 = 0x93;
		if (!McuTransfer(hRadio, 1, &l0, 1, &l0) || l0)
			return FALSE;	/*  invalid PLL params if l0 is non-zero */
	}
	
	/* WR2000 TODO store LastVco* now */
	
#define	Has5769 0 /* WR2000 TODO !!! in IdentifyRadio ? */
	/*  update microwave convertor if applicable */
	if ((rs->iShfState < 0) && (newShfState > 0)) {
		c = Has5769 ? 0xC6 : 0xC2;
		if ( rs->riInfo.dwMaxFreqkHz > 2600000 ) c ^= 2;
		SendI2CData(hRadio, c, 4, (LPBYTE)ShfData[Has5769][0]);
	}

	if (rs->iShfState != newShfState)
	{
		rs->iShfState = newShfState;
		c = Has5769 ? 0xC6 : 0xC2;
		if ( rs->riInfo.dwMaxFreqkHz > 2600000 ) c ^= 2;
		SendI2CData(hRadio, c, 4, (LPBYTE)ShfData[Has5769][newShfState]);
	}
	if (newShfState == 1)
		fvco = SHFFreq1 - fvco;
	else if (newShfState == 2)
		fvco += SHFFreq2;

	rs->ftActFreq = fvco;
	if (actfreq)
		*actfreq = fvco;

	return TRUE;
}

