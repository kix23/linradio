#ifndef __KERNEL__
#  include <string.h>
#  include <time.h>
#  include <stdio.h>
#else
#  include <linux/kernel.h>
#  include <linux/string.h>
#endif
#include "wrdef.h"
#include "wrio.h"

PRADIOSETTINGS RadioSettings[MAX_DEVICES+1] = { NULL, };
#ifdef __KERNEL__
RADIOSETTINGS RadioSettings_static[MAX_DEVICES+1];
#endif /* __KERNEL__ */

static void GetMcuVersion(int hRadio)
{
	char vs[80];
	int i = 0;

	if (!ValidateHandle(hRadio, NULL))
		return;

	WriteMcuByte(hRadio, 0x0e);
	do
		ReadMcuByte(hRadio, &vs[i++]);
	while (vs[i-1] && (i <= 75));
	vs[i] = 0;

	if (strstr(vs, "Vers 2.0.1,") || strstr(vs, "25M-XTAL"))
		RadioSettings[hRadio]->dwRefFreq = 25600000L;

	if (strstr(vs, "FCC"))
		RadioSettings[hRadio]->riInfo.dwFeatures |= RIF_USVERSION;
}

static void IdentifyRadio(int hRadio)
{
	LPRADIOSETTINGS rs = RadioSettings[hRadio];
	LPRADIOINFO ri = &rs->riInfo;
	BYTE rv;

	GetMcuVersion(hRadio);	/*  get MCU version (states US version and 25M xtal) */

	/*  get receiver version */
	WriteMcuByte(hRadio, 0x47);
	ReadMcuByte(hRadio, &rv);

	if ((rv > 237) || (rv < 10))
	{
		static BYTE Something[] = {0x56, 0x60, 0x6d, 0x2c, 0x6e, 0x00, 0x24, 0x6f, 0x1a, 0x77, 0x72, 0x5b, 0x67};

		/*  set frequency, mode and attenuator */
		McuTransfer(hRadio, sizeof(Something), Something, 0, NULL);

		Delay(100);
		WriteMcuByte(hRadio, 0x42);
		ReadMcuByte(hRadio, &rv);		/*  read signal strength */

		if (rv < 182)
			ri->wHWVer = RHV_1000b;		/*  1st generation 1000b */
		/*  restore settings */
		SetMode(hRadio, rs->iCurMode);
		SetFrequency(hRadio, rs->dwFreq);
		SetAtten(hRadio, rs->fCurAtten);
	}
	else if (rv > 190)
		ri->wHWVer = RHV_1000b;
	else if (rv > 121)
		ri->wHWVer = RHV_1500;
	else if (rv > 99)
		ri->wHWVer = RHV_3000;
	else if (rv > 52)
		ri->wHWVer = RHV_3100;
	else
	{
		BOOL SclLow, SdaLow;

		ri->wHWVer = RHV_1550;

		WriteMcuByte(hRadio, 0xf2);
		WriteMcuByte(hRadio, 0xff);	/*  activate IýC interface */
		Delay(100);
		SclLow = LockDetect(hRadio);
		WriteMcuByte(hRadio, 0xf2);
		WriteMcuByte(hRadio, 0xdf);
		Delay(100);
		SdaLow = LockDetect(hRadio);

		if (SclLow)
			if (SdaLow)
				ri->wHWVer = RHV_3200;
			else
				ri->wHWVer = RHV_3150;
		else
			if (SdaLow)
				ri->wHWVer = RHV_2000;
			else
			{
				static BYTE ShfData[] = {0x04, 0xa3, 0x99, 0x81};

				/*  detect IýC device to determine version */
				if (SendI2CData(hRadio, 0xc2, sizeof(ShfData), ShfData))
					ri->wHWVer = RHV_3500;
				else if (SendI2CData(hRadio, 0xc0, sizeof(ShfData), ShfData))
					ri->wHWVer = RHV_3700;
			}

		WriteMcuByte(hRadio, 0xf2);
		WriteMcuByte(hRadio, 0xbd);	/*  deactivate IýC interface */
	}

	/* ri->wHWVer = RHV_3500; */ /* WR2000: temporary */

	if (ri->wHWVer <= 0x10a)
		strcpy(ri->descr, "WR-1000");
	else
		{
			if ((ri->wHWVer >= 0x200) && (ri->wHWVer < 0x300))
				ri->wHWVer += 0x100;
			else
				if ((ri->wHWVer >= 0x300) && (ri->wHWVer < 0x400))
					ri->wHWVer -= 0x100;
			
			sprintf(ri->descr, "WR-%d%.2d0", ri->wHWVer >> 8, ri->wHWVer & 0xff);
		}
	if (ri->iHWInterface)
		strcat(ri->descr, "e");
	else
		strcat(ri->descr, "i");
	
	if (ri->dwFeatures & RIF_USVERSION)
		strcat(ri->descr, " (US version)");
}

BOOL SetFreq1000(int, double, double *);
BOOL SetFreq2000(int, double, double *);
BOOL SetMode1000(int, int);
BOOL SetMode2000(int, int);
BOOL SetBfo1000(int, int);
BOOL SetBfo1500(int, int);
BOOL SetBfo2000(int, int);
BOOL SetAgc1000(int, BOOL);
BOOL SetAgc2000(int, BOOL);
BOOL SetIfGain1000(int, int);
BOOL SetIfGain2000(int, int);
int GetSLevel1000a(int);
int GetSLevel1000b(int);
int GetSLevel2000(int);
int GetSLevel3000(int);

static void InitRadioSettings(int hRadio, BOOL fFullInit)
{
	LPRADIOSETTINGS rs = RadioSettings[hRadio];
	LPRADIOINFO ri = &rs->riInfo;

	static int ModeList[] = {0, 1, 2, 3, 4, 5, 6, 7};

	/*  initialize RadioSettings to defaults (1000b) */
	rs->lpSetFreqProc = SetFreq1000;
	rs->lpSetModeProc = SetMode1000;
	rs->lpSetBfoProc = SetBfo1000;
	rs->lpSetAgcProc = SetAgc1000;
	rs->lpSetIfGainProc = SetIfGain1000;
	rs->lpGetSLevelProc = GetSLevel1000b;

	rs->ftIfXOverFreq = 513e6;

	/*  initialize RadioInfo to defaults (1000b) */
	ri->dwSize = sizeof(RADIOINFO);
	ri->dwFeatures = 0;
	ri->wAPIVer = 0x232;	/*  version 2.50 */
	if (fFullInit)
		ri->wHWVer = RHV_1000b;	/*  for starters */
	ri->dwMinFreq = 500000L;
	ri->dwMaxFreq = 1300000000L;
	ri->iFreqRes = 100;
	ri->iNumModes = 4;
	ri->iMaxVolume = 31;
	ri->iMaxBFO = 3000;
	ri->iMaxFMScanRate = 50;
	ri->iMaxAMScanRate = 10;
	ri->iNumSources = 1;
	ri->iDeviceNum = hRadio;
	ri->iMaxIFShift = 0;
	ri->iDSPSources = 0;
	ri->dwWaveFormats = 0;
	ri->dwMaxFreqkHz = 1300000;
	ri->lpSupportedModes = (LPMODELIST)&ModeList;

	if (fFullInit)
		IdentifyRadio(hRadio);

	if ((ri->wHWVer >= RHV_3000) && (ri->wHWVer < RHV_2000))
	{
		/*  a Spectrum Monitor receiver (3000, 3100, 3150, 3500, 3700) */
		ri->dwMinFreq = 150000L;
		ri->dwMaxFreq = 1500000000L;
		ri->dwMaxFreqkHz = 1500000L;
		ri->iFreqRes = 1;
		ri->iNumModes = 6;
		ri->dwFeatures |= RIF_LSBUSB;
		rs->lpGetSLevelProc = GetSLevel3000;

		if (ri->wHWVer == RHV_3000)
		{
			ri->iMaxBFO = 2000;
			ri->iMaxIFShift = 2000;
		}
		else
		if (ri->wHWVer == RHV_3100)
		{
			ri->dwFeatures |= RIF_CWIFSHIFT;
			WriteMcuByte(hRadio, 0x70);		/*  clear -SETBFO line */
			WriteMcuByte(hRadio, 0x59);		/*  clear -SETPLL line */
			if (ri->dwFeatures & RIF_USVERSION)
			{
				WriteMcuByte(hRadio, 0x7b);
				WriteMcuByte(hRadio, 0x00);	/*  -SETPLL uses latch 0, bit 0 */
			}
			ri->iMaxBFO = 0;
			ri->iMaxIFShift = 3000;
			rs->lpSetBfoProc = SetBfo1500;
		}
		else
		{
			if (ri->wHWVer == RHV_3500)
			{
				ri->dwMaxFreq = 2100000000L;
				ri->dwMaxFreqkHz = 2600000L;
			} else
			if (ri->wHWVer == RHV_3700)
			{
				ri->dwMaxFreq = 2100000000L;
				ri->dwMaxFreqkHz = 4000000L;
			} else
			{
				ri->dwMaxFreq = 1600000000L;
				ri->dwMaxFreqkHz = 1600000L;
			}
		}
	} else
	if (ri->wHWVer >= RHV_1500)
	{
		ri->dwMinFreq = 150000L;
		ri->dwMaxFreq = 1500000000L;
		ri->dwMaxFreqkHz = 1500000L;
		ri->iFreqRes = 1;
		ri->iNumModes = 6;
		ri->iMaxBFO = 0;
		ri->iMaxIFShift = 3000;
		ri->dwFeatures |= RIF_LSBUSB | RIF_CWIFSHIFT;

		if (ri->wHWVer == RHV_1500)
		{
			WriteMcuByte(hRadio, 0x70);		/*  clear -SETBFO line */
			WriteMcuByte(hRadio, 0x59);		/*  clear -SETPLL line */
			if (ri->dwFeatures & RIF_USVERSION)
			{
				WriteMcuByte(hRadio, 0x7b);
				WriteMcuByte(hRadio, 0x00);	/*  -SETPLL uses latch 0, bit 0 */
			}
			rs->lpSetBfoProc = SetBfo1500;
		}
	}

	if (((ri->wHWVer > RHV_1500) && (ri->wHWVer < RHV_3000)) || (ri->wHWVer > RHV_3100))
	{
		/*  2000 series based receivers, set appropriate properties */
		rs->lpSetFreqProc = SetFreq2000;
		rs->lpSetModeProc = SetMode2000;
		rs->lpSetBfoProc = SetBfo2000;
		rs->lpSetAgcProc = SetAgc2000;
		rs->lpSetIfGainProc = SetIfGain2000;
		rs->lpGetSLevelProc = GetSLevel2000;

		rs->dwRefFreq = 25600000L;
		rs->ftIfXOverFreq = 808e6;

		WriteMcuByte(hRadio, 0x6b);

		if (ri->dwFeatures & RIF_USVERSION)
		{
			WriteMcuByte(hRadio, 0x7c);
			WriteMcuByte(hRadio, 0x12);
			WriteMcuByte(hRadio, 0x7b);
			WriteMcuByte(hRadio, 0x32);
		}

		if (ri->wHWVer > RHV_3150)
		{
			ri->dwFeatures |= RIF_AGC | RIF_IFGAIN;
			ri->iMaxIFGain = 100;
			ri->iNumModes = 8;
		}
		else
		{
			SetIfGain2000(hRadio, 100);
			SetAgc2000(hRadio, TRUE);
		}
	}

	if (fFullInit && (rs->dwRefFreq == 12800000L) && (ri->wHWVer > RHV_1000a))
	{
		/*  detect presence of 25MHz reference crystal (uses PLL lock detect) */
		rs->lpSetFreqProc(hRadio, 1100e6, NULL);
		Delay(100);
		if ((GetMcuStatus(hRadio) & 4) >> 2)
			rs->dwRefFreq = 25600000L;
		SetFrequency(hRadio, rs->dwFreq);
	}

	if (ri->wHWVer == RHV_1000a)
		rs->lpGetSLevelProc = GetSLevel1000a;
}

static void InitializeRadio(int hRadio)
{
	int i;
	BOOL oMute;

	for (i = 0; i < 8; i++)
	{
		long timeout;
		BYTE a, b;

		timeout = GetTickCount() + 200;	/*  200 ms */
		do	/*  make sure MCU is ready */
		{
			WriteMcuByte(hRadio, 13);
			if (ReadMcuByte(hRadio, &a) && ReadMcuByte(hRadio, &b) &&
					(a == 0x55) && (b == 0xaa))
				break;
		} while (GetTickCount()-timeout<0);
	}

	oMute = RadioSettings[hRadio]->fCurMute;
	RadioSettings[hRadio]->riInfo.wHWVer = RHV_1000b;
    /* Prevents SetPower() from programming the pll */
	SetPower(hRadio, TRUE);
	Delay(300);

	RadioSettings[hRadio]->fLastMute = FALSE;
	SetMute(hRadio, TRUE);
	InitRadioSettings(hRadio, TRUE);
	SetBFOOffset(hRadio, RadioSettings[hRadio]->iCurBfo);
	SetIFShift(hRadio, RadioSettings[hRadio]->iCurIfShift);
	SetAtten(hRadio, RadioSettings[hRadio]->fCurAtten);
	SetMute(hRadio, oMute);
	SetMode(hRadio, RadioSettings[hRadio]->iCurMode);
	SetFrequency(hRadio, RadioSettings[hRadio]->dwFreq);

	WriteMcuByte(hRadio, 6);
	WriteMcuByte(hRadio, 3);
}

/*  ReadSettings retreives the settings from the receiver's MCU that were */
/*  stored during CloseRadioDevice. This is used to shortcut the */
/*  identifcation routine. */

static BOOL ReadSettings(int hRadio)
{
	LPRADIOSETTINGS rs = RadioSettings[hRadio];
	BYTE checksum = 0, buf[16];
	int i;

	/*  read settings from MCU */
	for (i = 0; i < 16; i++)
	{
		WriteMcuByte(hRadio, 0x12);
		WriteMcuByte(hRadio, 0xb0 + i);
		ReadMcuByte(hRadio, &buf[i]);
		if (i < 15)
			checksum += buf[i];
	}

	/*  compare checksums to see if valid information in the MCU */
	if (checksum != buf[15])
		return FALSE;

	/*  store settings into RadioSettings structure */
	/*  set frequency */
	rs->dwFreq = (long)buf[3] << 24 | (long)buf[2] << 16 | (long)buf[1] << 8 | buf[0];
	if (rs->dwFreq & 0x80000000l)	/*  if MSB set, freq = low 31 bits x 10 */
		rs->ftFreqHz = 10.0 * (rs->dwFreq ^ RFQ_X10);
	else
		rs->ftFreqHz = rs->dwFreq;
	rs->iFreqErr = 0;
	rs->ftActFreq = rs->ftFreqHz;
	/*  mode */
	rs->iCurMode = buf[4];
	/*  flags */
	rs->fCurAtten = buf[5] & 0x01;
	rs->fCurMute = buf[5] & 0x02;
	rs->fLastMute = buf[5] & 0x04;
	rs->fCurAgc = buf[5] & 0x08;
	if (buf[5] & 0x40)
		rs->dwRefFreq = 25600000L;
	/*  hardware version */
	rs->riInfo.wHWVer = (WORD)buf[14] << 8 | buf[13];

	/*  get full receiver settings according to hardware version */
	InitRadioSettings(hRadio, FALSE);

	/*  fill in rest of the settings */
	if (buf[5] & 0x80)
		rs->riInfo.dwFeatures |= RIF_USVERSION;
	/*  BFO or IF shift */
	if ((rs->riInfo.wHWVer <= RHV_1000b) || ((rs->iCurMode == RMD_CW) &&
		!(rs->riInfo.dwFeatures & RIF_CWIFSHIFT)))
	{
		rs->iCurBfo = (int)buf[7] << 8 | buf[6];
		rs->iCurIfShift = 0;
	} else
	{
		rs->iCurIfShift = (int)buf[7] << 8 | buf[6];
		rs->iCurBfo = 0;
	}
	/*  IF gain */
	rs->iCurIfGain = buf[8];

	return TRUE;
}

static int InitGetVolume(int hRadio)
{
	BYTE b;

	WriteMcuByte(hRadio, 0x89);
	ReadMcuByte(hRadio, &b);
	return b;
}

static BOOL InitGetPower(int hRadio)
{
	if (RadioSettings[hRadio]->riInfo.iHWInterface != RHI_SERIAL)
	{
		BYTE b;

		WriteMcuByte(hRadio, 10);
		ReadMcuByte(hRadio, &b);
		return b;
	}
	else
		return TRUE;
}

BOOL ResetRadio(int hRadio)
{
	LPRADIOSETTINGS rs = RadioSettings[hRadio];
	BYTE b;

	rs->dwRefFreq = 12800000L;

	ReadMcuByte(hRadio, &b);	/*  empty read port just in case there is data there */

	if (WriteMcuByte(hRadio, 7) && ReadMcuByte(hRadio, &b) && (b == 1))
	{
		/*  already initialized previously */
		rs->fCurPower = InitGetPower(hRadio);
		rs->iCurVolume = InitGetVolume(hRadio);
		if (ReadSettings(hRadio))
			return TRUE;	/*  nothing else to do! */
	}
	/*  need to perform a reset... also need to see if it exists! */

	if (!PerformReset(hRadio))
		return FALSE;

	/*  initialize RadioSettings structure */
	rs->ftFreqHz = 10e6;
	rs->ftActFreq = 10e6;
	rs->dwFreq = 10000000;
	rs->iCurMode = RMD_AM;
	rs->iFreqErr = 0;
	rs->iCurVolume = 5;
	rs->fCurAtten = FALSE;
	rs->fCurMute = FALSE;
	rs->fCurPower = TRUE;
	rs->iCurBfo = 0;
	rs->iCurIfShift = 0;
	rs->fInitVolume = TRUE;
	rs->fLastMute = FALSE;

	InitializeRadio(hRadio);

	return TRUE;
}
