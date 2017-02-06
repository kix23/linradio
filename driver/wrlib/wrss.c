#include <math.h>
#include "wrdef.h"
#include "wrio.h"

static inline double inl_pow(double x, double y) { return exp(y*log(x)); }

/* int GetSignalStrength1000a(int hRadio);	// 1st gen 1000 only */
/* int GetSignalStrength1000b(int hRadio);	// 2nd gen 1000 series and later */
/* int GetSignalStrength3000(int hRadio);	// Spectrum Monitor series */

int GetSLevel1000a(int hRadio)
{
	double v;
    BYTE ssm;

    if (RadioSettings[hRadio]->iCurMode & 2) /*  FM-N and FM-W: bit 1 is set */
	{         
		if (RadioSettings[hRadio]->iCurMode == RMD_FMW)
			WriteMcuByte(hRadio, 0x10);
		else
			WriteMcuByte(hRadio, 0x11);
	    ReadMcuByte(hRadio, &ssm);
        ssm -= 60;
        if (ssm <= 8) 
			v = 1.2 * ssm;
        else 
			if (ssm <= 65) 
				v = 0.54 * ssm + 5.28;
			else 
				if (ssm <= 110) 
					v = 0.40 * ssm + 16;
				else 
					v = 1.2 * ssm - 72;
    } 
	else 
	{             /*  AM and SSB */
		WriteMcuByte(hRadio, 0x42);
		ReadMcuByte(hRadio, &ssm);
        ssm = 256 - ssm;
        if (ssm <= 20) 
			v = 0.775 * ssm;
        else 
			if (ssm <= 130) 
				v = 0.25 * ssm + 10.5;
			else 
				if (ssm <= 170) 
					v = 0.52 * ssm - 26;
				else 
					if (ssm <= 190) 
						v = 1.1 * ssm - 125;
					else 
						v = 1.7 * ssm - 236;
    }
    if (v < 0) 
		v = 0;
    if (v > 120) 
		v = 120;
    return (int)(v + 0.5);
}


typedef struct _SS_DATA {
	DWORD	f;
	int		AMslope;
	int		AMmin;
	int		FMWslope;
	int		FMWmin;
	int		FMNadc;
	int		FMNslope1;
	int		FMNslope2;
	int		FMNmin;
	int		FMNco;
} SS_DATA;

#define NUMENTRIES	35

SS_DATA SSData[NUMENTRIES] = {
    {   1000000, -550,  -940, -365, -104, 103, 120, 260, -103, -50}, 
    {   2000000, -550,  -940, -365, -104, 70, 90, 225, -130, -68}, 
    {  20000000, -575, -1050, -365, -104, 76, 100, 210, -135, -75}, 
    {  49999999, -550, -1055, -365, -104, 93, 125, 210, -127, -70}, 
    {  50000000, -550, -1055, -365, -104, 96, 130, 250, -125, -60}, 
    { 100000000, -575, -1065, -367, -102, 85, 110, 210, -130, -75}, 
    { 150000000, -585, -1025, -375, -101, 80, 107, 205, -133, -75}, 
    { 200000000, -590, -1043, -375, -102, 80, 110, 210, -135, -75}, 
    { 250000000, -575, -1046, -360, -103, 78, 105, 210, -135, -75}, 
    { 299999999, -565,  -994, -360, -99, 74, 100, 190, -133, -75}, 
    { 300000000, -605,  -910, -390, -89, 55, 75, 180, -127, -75}, 
    { 350000000, -600,  -900, -390, -88, 53, 70, 185, -128, -80}, 
    { 400000000, -600,  -910, -390, -89, 53, 70, 185, -128, -80}, 
    { 450000000, -600,  -940, -390, -90, 56, 75, 180, -130, -80}, 
    { 500000000, -580,  -970, -370, -94, 60, 80, 185, -133, -80}, 
    { 512999999, -590,  -970, -370, -94, 67, 90, 170, -131, -80}, 
    { 513000000, -540, -1020, -300, -100, 77, 85, 200, -128, -80}, 
    { 550000000, -540, -1020, -300, -100, 74, 85, 200, -130, -80}, 
    { 600000000, -560, -1020, -310, -100, 79, 90, 180, -129, -85}, 
    { 650000000, -575, -1010, -310, -99, 80, 90, 180, -127, -85}, 
    { 700000000, -580, -1040, -310, -100, 79, 90, 180, -130, -85}, 
    { 750000000, -580, -1060, -300, -101, 82, 90, 180, -128, -85}, 
    { 797999999, -570, -1020, -285, -100, 78, 90, 175, -130, -85}, 
    { 798000000, -500, -1010, -288, -101, 77, 90, 165, -131, -85}, 
    { 850000000, -505,  -985, -285, -97, 69, 90, 185, -131, -70}, 
    { 900000000, -510,  -970, -285, -95, 68, 90, 185, -132, -70}, 
    { 950000000, -500,  -960, -285, -95, 77, 90, 185, -124, -70}, 
    {1000000000, -495,  -991, -280, -98, 90, 100, 180, -119, -70}, 
    {1050000000, -490, -1020, -280, -101, 87, 100, 185, -124, -70}, 
    {1105999999, -480, -1000, -280, -100, 88, 100, 190, -123, -70}, 
    {1106000000, -450, -1019, -260, -102, 91, 100, 160, -123, -80}, 
    {1150000000, -460,  -990, -260, -100, 85, 90, 150, -122, -80}, 
	{1200000000, -465,  -963, -260, -96, 86, 90, 160, -118, -75},
	{1250000000, -465,  -915, -265, -93, 80, 90, 140, -118, -75},
	{1300000000, -482,  -919, -270, -90, 75, 90, 165, -121, -65}};

int GetSLevel1000b(int hRadio)
{
	double	ratio = 0, x, slope, min, adc, slope2, co;
	long	v;
	BYTE	N;
	int		i, m = RadioSettings[hRadio]->iCurMode;
	double	f = RadioSettings[hRadio]->ftFreqHz;

	for (i = 0; i < NUMENTRIES; i++)
		if (f <= SSData[i].f) 
			break;

	if (i > 0)
		ratio = (f - SSData[i-1].f) / (SSData[i].f - SSData[i-1].f);

	if (m == RMD_FMN) 
	{	
		if ((f >= SSData[i].f) || !i) 		/*  No interpolation required  */
		{
			min = SSData[i].FMNmin;
			adc = SSData[i].FMNadc;
			slope = SSData[i].FMNslope1 / 100;
			slope2 = SSData[i].FMNslope2 / 100;
			co = SSData[i].FMNco;
		} 
		else 
		{
			min = SSData[i-1].FMNmin + (ratio * (SSData[i].FMNmin - SSData[i-1].FMNmin));
			adc = SSData[i-1].FMNadc + (ratio * (SSData[i].FMNadc - SSData[i-1].FMNadc));
			slope = (SSData[i-1].FMNslope1 + (ratio * (SSData[i].FMNslope1 - SSData[i-1].FMNslope1))) / 100;
			slope2 = (SSData[i-1].FMNslope2 + (ratio * (SSData[i].FMNslope2 - SSData[i-1].FMNslope2))) / 100;
			co = SSData[i-1].FMNco + (ratio * (SSData[i].FMNco - SSData[i-1].FMNco));
		}
		WriteMcuByte(hRadio, 0x41);
		ReadMcuByte(hRadio, &N);
		x = min + slope * (N - adc);
		if (x >= co) 
			x = co - ((co - min) * slope2 / slope) + (slope2 * (N - adc));
		v = (long)(x - min + 0.5);
	} 
	else 
	{    /*  AM, SSB & FMW  */
		if (m == RMD_FMW) 
		{
			if ((f >= SSData[i].f) || !i) 		/*  No interpolation required  */
			{
				slope = SSData[i].FMWslope;
				/* min = SSData[i].FMWmin; */
			} 
			else 
			{
				slope = SSData[i-1].FMWslope + (ratio * (SSData[i].FMWslope - SSData[i-1].FMWslope));
				/* min = SSData[i-1].FMWmin + (ratio * (SSData[i].FMWmin - SSData[i-1].FMWmin)); */
			}
		} 
		else 
		{
			if ((f >= SSData[i].f) || !i) 		/*  No interpolation required  */
			{
				slope = SSData[i].AMslope;
				/* min = SSData[i].AMmin / 10; */
			} 
			else 
			{
				slope = SSData[i-1].AMslope + (ratio * (SSData[i].AMslope - SSData[i-1].AMslope));
				/* min = (SSData[i-1].AMmin + (ratio * (SSData[i].AMmin - SSData[i-1].AMmin))) / 10; */
			}
		}
		WriteMcuByte(hRadio, 0x42);
		ReadMcuByte(hRadio, &N);
		if (N < 19) N = 19;
		v = (long)(slope * (0.236 + log10(1 + log10(log10(log10(N))))) /*- min*/ + 0.5);
	}
	if (v < 0) v = 0;
	if (v > 120) v = 120;
	return v;
}


DWORD const CAL_FREQ[] = {1000, 2000, 30000, 130000, 299000, 300250, 402000, 500120, 514000, 601000, 700000,
							825300, 1020300, 1105000, 1106000, 1200000, 1300000, 1350000, 1450000};
int const CAL_FMW[] = {-160, -160, -160, -160, -170, -270, -270, -280, 290, -290,
							-290, -260, -260, -260, -170, -150, -150, -150, -150};
int const CAL_AM[] = {-320, -320, -320, -320, -330, -420, -420, -440, -450, -470,
							-460, -420, -410, -420, -320, -300, -300, -300, -300};

static double Interpolate(DWORD x1, int y1, DWORD x2, int y2, DWORD x)
{
	if (x1 != x2)
		return (double)(y2 - y1) * (x - x1) / (x2 - x1) + y1;
	else
		return 0;
}

static int FindInTable(DWORD f)
{
	if (f < 2000) 
		return 1;			/*  Use 1 and 2MHz below 2MHz */
	else if (f < 30000) 
		return 2;
	else if (f < 130000)
		return 3;
	else if (f < 300000) 
		return 4;
	else if (f < 402000) 
		return 6;			/*  Never use pair 299 & 300.25 / different sub-bands */
	else if (f < 513000) 
		return 7;
	else if (f < 601000) 
		return 9;			/*  Never use pair 500 & 514 / different sub-bands */
	else if (f < 798000) 
		return 10;
	else if (f < 1020300) 
		return 12;			/*  Never use pair 700 & 825 / different sub-bands */
	else if (f < 1106000) 
		return 13;
	else if (f < 1200000) 
		return 15;			/*  Never use pair 1105 & 1106 / different sub-bands */
	else if (f < 1300000) 
		return 16;
	else if (f < 1350000)
		return 17;
	else 
		return 18;
}

int GetSLevel3000(int hRadio)	/*  Spectrum Monitor series */
{
	double s;
	LPRADIOSETTINGS rs = RadioSettings[hRadio];

	if (rs->iCurMode == RMD_FMN)
	{
		s = GetSLevel1000b(hRadio) - 20;
	}
	else
	{
		DWORD freq = (DWORD)(rs->ftFreqHz / 1000 + 0.5);
		int i = FindInTable(freq);
		BYTE agc;
		double slope;

		WriteMcuByte(hRadio, 0x42);
		ReadMcuByte(hRadio, &agc);
		if (agc < 20) agc = 20;

		if (rs->iCurMode == RMD_FMW)
			slope = Interpolate(CAL_FREQ[i], CAL_FMW[i], CAL_FREQ[i+1], CAL_FMW[i+1], freq);
		else
			slope = Interpolate(CAL_FREQ[i], CAL_AM[i], CAL_FREQ[i+1], CAL_AM[i+1], freq);

		s = slope * (0.236 + log10(1 + log10(log10(log10(agc)))));
	}
	if (s < 0) s = 0;
	if (s > 120) s = 120;

	return (int)(s + 0.5);
}

/* WR2000 TODO: update this */
int GetSLevel2000(int hRadio)
{
	LPRADIOSETTINGS rs = RadioSettings[hRadio];
	BOOL IsMidBand = (rs->ftFreqHz >= 400e6) && (rs->ftFreqHz < 1114e6);
	BYTE b;

	double v;

	WriteMcuByte(hRadio, (rs->iCurMode == RMD_FMN) || (rs->iCurMode == RMD_FM6) ? 0x41 : 0x42);
	ReadMcuByte(hRadio, &b);

	if ((rs->iCurMode == RMD_FMW) || (rs->iCurMode == RMD_FMM))
	{
		if (IsMidBand)
			v = inl_pow((double)b / 100, -1.3) * 27 - 25;
		else
			v = 105 - 8 * inl_pow((double)b - 30, 0.5);
	}
	else if ((rs->iCurMode == RMD_FMN) || (rs->iCurMode == RMD_FM6))
	{
		if (IsMidBand)
			v = exp(((double)b - 90) / 10) * 2.5 - 2.5;
		else
			v = exp(((double)b - 60) / 24) * 2.5 - 2.5;
	}
	else
	{
		if (IsMidBand)
			v = inl_pow((double)b / 90, -3.2) * 20 - 20;
		else
			v = inl_pow((double)b / 260, -1.5) * 20 - 22;

		if (rs->iCurMode != RMD_AM)
			v *= 2;
	}

	if (v < 0) v = 0;
	if (v > 120) v = 120;

	return (int)(v + 0.5);
}
