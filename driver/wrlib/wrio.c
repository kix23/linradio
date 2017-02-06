#if defined(__linux__) && !defined(__KERNEL__)
#  include <stdio.h>
#  include <unistd.h>
#  include <time.h>
#  include <sys/time.h>
#  include <asm/io.h>
#endif /* __linux__ && !__KERNEL */

#if defined(__linux__) && defined(__KERNEL__)
#  include <linux/config.h>
/* #  include <linux/module.h> */
#  include <linux/version.h>
#  include <linux/types.h>
#  include <linux/delay.h>
#  include <asm/io.h>
#endif /* __KERNEL__ */

#ifdef __FreeBSD__
#  include <stdio.h>
#  include <unistd.h>
#  include <time.h>
#  include <sys/time.h>
#  include <machine/cpufunc.h>
#endif /* __FreeBSD__ */

#include "wrdef.h"
#include "wrio.h"
#include "wrserial.h"

long GetTickCount() {
#ifndef __KERNEL__
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000 + tv.tv_usec/1000;
#else
	return jiffies * (1000/HZ);
#endif
}

/* ---------------------------------------------------------------------- */
/*  */
/*  This file contains platform specific code (PC in this case) for */
/*  accessing WiNRADiO ISA devices. The serial part is generic (see */
/*  wrserial.c to modify access to serial WiNRADiOs in different platforms). */
/*  */
/*  The only functions that may require modification to either remove or */
/*  modify the access to the ISA devices are: */
/*      WriteMcuByte, ReadMcuByte, GetMcuStatus and PerformReset. */
/*  */

/* ---------------------------------------------------------------------- */
/*  Main function for sending a byte to the receiver's MCU */
/* ---------------------------------------------------------------------- */

BOOL WriteMcuByte(int hRadio, BYTE c)
{
        long timeout;

	TRACE(("(%02X)", c));

	if (RadioSettings[hRadio]->riInfo.iHWInterface == RHI_SERIAL)
	{
		/*  send byte to serial device (does not require modification) */

		BYTE ack;

		while (ReadSerialByte(hRadio, &ack, 0)) ;	/*  flush read queue */

		if (!SendSerialByte(hRadio, c)) {
			TRACE(("!"));
			return FALSE;
		}

		/*  have to wait until the acknowledge byte arrives */
		if ( !ReadSerialByte(hRadio, &ack, 200) ) {
			TRACE(("|"));
			return FALSE;
		}
		ack = ~ack & 0xff;
		if (ack == c) { /*  received byte should be inverse of byte sent */
		  TRACE(("."));
		  return TRUE;
		}
		else {
			TRACE(("%02X?", ack));
			return FALSE;
		}
	}
	else /* ISA */
	{
		/*  send byte to ISA device (may require modification or removal) */

		WORD ioAddr = RadioSettings[hRadio]->wIoAddr;

		if (inb(ioAddr + 1) & 0x01) /*  empty read port if full */
			inb(ioAddr);

		/*  MCU should be available within 10ms - timeout in 100ms */
		timeout = GetTickCount() + 100;
		while (inb(ioAddr + 1) & 0x02)
		{
		  if ( GetTickCount()-timeout > 0 ) {
		    TRACE(("send timeout\n"));
		    return FALSE;
		  }
		}

		/*  send byte to MCU when the write port is empty */
		outb(c, ioAddr);
	}
	return TRUE;
}

/* ---------------------------------------------------------------------- */
/*  Main function for reading a byte from the receiver's MCU */
/* ---------------------------------------------------------------------- */

BOOL ReadMcuByte(int hRadio, PBYTE c)
{
	long timeout;

	if (RadioSettings[hRadio]->riInfo.iHWInterface == RHI_SERIAL)
	{
		/*  read byte from serial device (does not require modification) */

		/*  have to wait until a byte is available in the queue */
		if (!ReadSerialByte(hRadio, c, 200)) {
			TRACE(("[!]"));
			return FALSE;
		}
	}
	else /* ISA */
	{
		/*  read byte from ISA device (may require modification or removal) */

		WORD ioAddr = RadioSettings[hRadio]->wIoAddr;

		/*  byte should be available within 10ms - timeout in 100ms */
		timeout = GetTickCount() + 100;
		while (!(inb(ioAddr + 1) & 0x01))
		{
		  if ( GetTickCount()-timeout > 0 ) {
		    TRACE(("recv timeout\n"));
		    return FALSE;
		  }
		}

		/*  read byte from MCU when read port is full */
		*c = inb(ioAddr);
	}
	TRACE(("[%02X]", *c));

	return TRUE;
}

BOOL McuTransfer(int hRadio, int tc, PBYTE tb, int rc, PBYTE rb)
{
	/*  send all bytes in buffer to MCU */
	while (tc--)
		if (!WriteMcuByte(hRadio, *tb++))
			return FALSE;

	/*  fill buffer with bytes read from the MCU */
	while (rc--)
		if (!ReadMcuByte(hRadio, rb++))
			return FALSE;

	return TRUE;
}

BYTE GetMcuStatus(int hRadio)
{
	if (RadioSettings[hRadio]->riInfo.iHWInterface == RHI_SERIAL)
		return 0x20;	/*  PLL is locked and DSP doesn't exist */
	else
		return inb(RadioSettings[hRadio]->wIoAddr + 1);
}

BOOL ValidateHandle(int hRadio, LPRADIOSETTINGS *lpRadioSettings)
{
	if ((hRadio > 0) && (hRadio <= MAX_DEVICES))
	{
		if (lpRadioSettings)
			*lpRadioSettings = RadioSettings[hRadio];
		return (RadioSettings[hRadio] != NULL);
	}
	return FALSE;
}

BOOL PerformReset(int hRadio)
{
	TRACE(("Reset\n"));

	if (RadioSettings[hRadio]->riInfo.iHWInterface == RHI_SERIAL)
	{
	  BYTE a, b;
		/*  initiates comms with serial devices (no mods required) */

		WriteMcuByte(hRadio, 0);	/*  sychronise comms */
		WriteMcuByte(hRadio, 0);
		WriteMcuByte(hRadio, 0);
		WriteMcuByte(hRadio, 0);
		/*  increase baud rate */
#ifdef WRSERIAL_115200 /* PB 115200 supported on all models ? */
		WriteMcuByte(hRadio, 0xae);
		WriteMcuByte(hRadio, 1);
		SetBaudRate(hRadio, 115200l);
#endif
#ifdef WRSERIAL_38400
		WriteMcuByte(hRadio, 0xae);
		WriteMcuByte(hRadio, 3);
		SetBaudRate(hRadio, 38400l);
#endif
		/*  establish existance of WiNRADiO */
		if (!(WriteMcuByte(hRadio, 0x0d) && ReadMcuByte(hRadio, &a) &&
			ReadMcuByte(hRadio, &b) && (a == 0x55) && (b == 0xaa)))
			{
				Delay(200);
				WriteMcuByte(hRadio, 0);
				WriteMcuByte(hRadio, 0);
				WriteMcuByte(hRadio, 0);
				WriteMcuByte(hRadio, 0);
				
				if (!(WriteMcuByte(hRadio, 0x0d) && ReadMcuByte(hRadio, &a) &&
							ReadMcuByte(hRadio, &b) && (a == 0x55) && (b == 0xaa))) {
					return FALSE;
				}
			}
		return TRUE;
	}
	else /* ISA */
	{
	  BYTE a;
		/*  reset the ISA WiNRADiO device (may require modification or removal) */

		inb(RadioSettings[hRadio]->wIoAddr);	/*  clear outut buffer */
		inb(RadioSettings[hRadio]->wIoAddr);

		outb(1, RadioSettings[hRadio]->wIoAddr + 1);	/*  activate reset line */

		Delay(100);

		outb(0, RadioSettings[hRadio]->wIoAddr + 1);	/*  unreset MCU */

		Delay(100);

		if (ReadMcuByte(hRadio, &a) && (a == 0x55))
			return TRUE;
		return (ReadMcuByte(hRadio, &a) && (a == 0x55));
	}
}

#ifdef __KERNEL__
void (*yield_hook)() = NULL;
void (*reenter_hook)() = NULL;
#endif /* __KERNEL */

void Delay(int ms)
{
#ifndef __KERNEL__
  usleep(ms*1000);
#else
  mdelay(ms);
#endif
#ifdef notdef_tricky
  if ( yield_hook ) (*yield_hook)();
  schedule_timeout(ms*HZ/1000);
  if ( reenter_hook ) (*reenter_hook)();
#endif
}

BOOL LockDetect(int hRadio)
{
	LPRADIOINFO ri = &RadioSettings[hRadio]->riInfo;
	BYTE b;

	if (ri->iHWInterface != RHI_ISA)
	{
		if ((ri->wHWVer <= RHV_1500) || ((ri->wHWVer >= RHV_3000) && (ri->wHWVer <= RHV_3100)))
			return TRUE;	/*  these externals do not support lock detect */

		WriteMcuByte(hRadio, 0x43);
		ReadMcuByte(hRadio, &b);
		return (b < 127);
	}
	else
		return (GetMcuStatus(hRadio) & 4) != 0;
}

#define I2C_ENABLE	0x04
#define I2C_SCL		0x10
#define I2C_SDA 	0x80

static BOOL WriteToL2(int hRadio, BYTE b)
{
	return WriteMcuByte(hRadio, 0xf2) && WriteMcuByte(hRadio, b);
}

static BOOL SendI2CByte(int hRadio, BYTE b, BYTE raw)
{
	BOOL result;
	int i = 0x80;	/*  start with MSB */
	BYTE t;

	/*  clear SCL and SDA lines */
	WriteToL2(hRadio, raw);

	do
	{
		t = (b & i) ? raw | I2C_SDA : raw;
		WriteToL2(hRadio, t);
		WriteToL2(hRadio, t | I2C_SCL);
		WriteToL2(hRadio, t);
		i >>= 1;
	} while (i);

	/*  send ACK bit to wait for acknowledgement */
	WriteToL2(hRadio, raw | I2C_SDA);
	WriteToL2(hRadio, raw | I2C_SDA | I2C_SCL);

	/*  if SDA is not low, then error sending data */
	result = LockDetect(hRadio);

	WriteToL2(hRadio, raw | I2C_SDA);	/*  clear SCL */

#if 0 /* WR2000 */
	/*  SDA becomes high when the IýC receiver has acknowledged */

	timeout = GetTickCount() + 200;	/*  wait up to 200 ms */
	do {
		if ( GetTickCount()-timeout > 0 ) {
			TRACE(("i2c timeout\n"));
			return FALSE;
		}
	} while (!LockDetect(hRadio));
#endif /* WR2000 */

	return result;
}

BOOL SendI2CData(int hRadio, int iAddr, int cbSize, PBYTE lpData)
{
	BYTE raw;
	BOOL result = FALSE;

	WriteMcuByte(hRadio, 0xf5);
	ReadMcuByte(hRadio, &raw);
	raw &= 0x4b;

	WriteToL2(hRadio, raw | I2C_SCL | I2C_SDA);	/*  set SDA and SCL */

	raw |= I2C_ENABLE;	/*  enable IýC bus for duration of procedure */
	WriteToL2(hRadio, raw | I2C_SCL | I2C_SDA);

	WriteToL2(hRadio, raw | I2C_SCL);	/*  initiate START condition */

	/*  send IýC address */
	result = SendI2CByte(hRadio, iAddr, raw);

	/*  send IýC data */
	while ((cbSize-- > 0) && (result))
		result = SendI2CByte(hRadio, *lpData++, raw);

	WriteToL2(hRadio, raw | I2C_SCL);	/*  clear SDA and set SCL */
	WriteToL2(hRadio, raw | I2C_SCL | I2C_SDA);	/*  send STOP condition */

	WriteToL2(hRadio, (raw & 0x8b) | I2C_SCL | I2C_SDA);

	return result;
}
