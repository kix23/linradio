#ifdef __KERNEL__
#  include <linux/kernel.h>
#else
#  include <stdlib.h>
#endif
#include <math.h>
#include "wrdef.h"
#include "wrio.h"

static inline double inl_pow(double x, double y) { return exp(y*log(x)); }

BOOL SetBfo1000(int hRadio, int bfo) 	/*  set analog BFO */
{
	static const struct _BFOPARAMS
	{
		double L; double Cp; double Cs;
	} BfoParams[2] = {{212.46, 560.14, 47}, {234.5, 498.1, 82}};

	double V, Cv, Ct;
	WORD DAC;
	int idx = (RadioSettings[hRadio]->riInfo.wHWVer == RHV_3000) ? 1 : 0;

	Ct = 1e18 / (inl_pow(2 * M_PI * (455e3 + bfo), 2) * BfoParams[idx].L);
	Cv = (BfoParams[idx].Cs * (Ct - BfoParams[idx].Cp)) /
		(BfoParams[idx].Cs - Ct + BfoParams[idx].Cp);
	V = exp(((50 - Cv) / 41) * M_LN10);
	DAC = (WORD)(V * 6553.5 + 0.5);

	return (
		WriteMcuByte(hRadio, 0x0b) &&
		WriteMcuByte(hRadio, DAC >> 8) &&
		WriteMcuByte(hRadio, DAC & 0xff)
		);
}

static BOOL SetBfoPll(int hRadio, BYTE c, WORD R, DWORD N, BYTE SetPll, BYTE ClrPll)
{
	LPRADIOSETTINGS rs = RadioSettings[hRadio];
	BOOL IsUS = rs->riInfo.dwFeatures & RIF_USVERSION;
	int bufidx = 0;
	BYTE buf[16];

	/*  set PLL C register */

	if (IsUS)
		buf[bufidx++] = 0x78;
	else
	{
		buf[bufidx++] = SetPll;
		buf[bufidx++] = 0x6d;
	}

	buf[bufidx++] = c; /* WR2000 TODO: AGC here */

	/*  set PLL R register */

	if (IsUS)
		buf[bufidx++] = 0x79;
	else
	{
		buf[bufidx++] = ClrPll;
		buf[bufidx++] = SetPll;
		buf[bufidx++] = 0x6e;
	}

	buf[bufidx++] = R & 0xff;
	buf[bufidx++] = R >> 8;

	/*  Set PLL A register */

	if (IsUS)
		buf[bufidx++] = 0x7a;
	else
	{
		buf[bufidx++] = ClrPll;
		buf[bufidx++] = SetPll;
		buf[bufidx++] = 0x6f;
	}

	buf[bufidx++] = N & 0xff;
	buf[bufidx++] = (N >> 8) & 0xff;
	buf[bufidx++] = N >> 16;

	if (!IsUS)
		buf[bufidx++] = ClrPll;

	return McuTransfer(hRadio, bufidx, buf, 0, NULL);
}

BOOL SetBfo1500(int hRadio, int iBfo)	/*  set digital BFO */
{
	LPRADIOSETTINGS rs = RadioSettings[hRadio];
	double Cv, Ct, f;
	DWORD d, N, bN=0;
	UINT R, A, bR=0;

	/*  calculate BFO's PLL parameters */

	f = 32.0 * (4.55e5 + iBfo);
	Cv = 2e9;
	for (R = rs->dwRefFreq / 5000; R >= rs->dwRefFreq / 10000; R--)
	{
		d = (DWORD)(f * R / rs->dwRefFreq + 0.5);
		N = d >> 6;
		A = d & 63;
		if (N > A)
		{
			Ct = abs((double)rs->dwRefFreq * d / R - f);
			if (Ct < Cv)
			{
				Cv = Ct;
				bN = d;
				bR = R;
				if (Cv < 0.5)
					break;
			}
		}
	}

	return SetBfoPll(hRadio,
		((rs->iCurMode == RMD_CW) || (rs->iCurMode == RMD_LSB) || (rs->iCurMode == RMD_USB)) ? 0x40 : 0x42,
		bR | 0x4000, 0x300000L + (bN & 63) + ((bN >> 6) << 8), 0x71, 0x70);
}

BOOL SetBfo2000(int hRadio, int iBfo)
{
	LPRADIOSETTINGS rs = RadioSettings[hRadio];
	BYTE c = rs->fCurAgc ? 0xc0 : 0xc1;
	DWORD d, N, bN=0, f, bD=(1UL<<31)-1;
	UINT R, A, bR=0;

	/* WR2000 TODO: mask iCurMode */
	if ((rs->iCurMode != RMD_CW) && (rs->iCurMode != RMD_LSB) && (rs->iCurMode != RMD_USB))
		c |= 2;

	/*  calculate BFO's PLL parameters */

	f = 32.0 * (4.55e5 + iBfo);

	/* WR2000 now loops on f instead of rs->dwRefFreq ??? */
	for (R = f / 5000; R >= f / 20000; R--)
	{
		d = (DWORD)((double)rs->dwRefFreq * R / f + 0.5);
		N = d >> 6;
		A = d & 63;
		if (N > A)
		{
			d = (DWORD)abs((double)f * d / R - rs->dwRefFreq);
			if (d < bD)
			{
				bD = d;
				bN = N;
				bR = R;
				if (d < 10)
					break;
			}
		}
	}
	N = bN;
	R = bR;

	return SetBfoPll(hRadio, c, R | 0x4000,
									 0x300000L + (N & 63) + ((N >> 6) << 8), 0x6c, 0x6b);
}
