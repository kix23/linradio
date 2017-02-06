/*
 * This is ugly. I don't know how to call the serial driver properly
 * (it doesn't export that many symbols). Directly accessing the UART
 * doesn't even improve performance. -- PB */

/* -------------------------------------------------------------------- */
/*  */
/*  PC-specific COMs routpines */
/*  */
/*  These have to be replaced to support a different platform or */
/*    architecture (or different serial drivers). */
/*  */
/* -------------------------------------------------------------------- */
/*  */
/*  Note: */
/*    This driver only supports one serial connection at one time. */
/*    The defined functions allow multiple serial connections by using */
/*    a different port for each hRadio instance. */
/*  */

// #define USE_INTERRUPT

#if defined(__linux__) && !defined(__KERNEL__) || defined(__FreeBSD__)
#  include <stdio.h>
#  include <sys/time.h>
#  include <sys/types.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <termios.h>
#  include <errno.h>
#  include <string.h>
#endif /* __linux__ && !__KERNEL__ || __FreeBSD__ */

#if defined(__linux__) && defined(__KERNEL__)
#  include <linux/config.h>
#  include <linux/version.h>
#  include <linux/types.h>
#  include <linux/delay.h>
#  include <asm/io.h>
#  include <linux/ioport.h>
#  include <linux/serial_reg.h>
#  ifdef USE_INTERRUPT
#    include <linux/interrupt.h>
#  endif
#  define COM_THR		0	// Transmit Holding Register
#  define COM_RBR		0	// Receiver Buffer Register
#  define COM_DLL 	0	// Divisor Latch Low
#  define COM_DLH		1	// Divisor Latch High
#  define COM_IER		1	// Interrupt Enable Register
#  define COM_IIR		2	// Interrupt Identification Register
#  define COM_LCR		3	// Line Control Register
#  define COM_MCR		4	// Modem Control Register
#  define COM_LSR		5	// Line Status Register
#  define COM_MSR		6	// Modem Status Register
   
#  define COM_TIMEOUT		0x80
#  define COM_TXEMPTY		0x20
#  define COM_BREAK		0x10
#  define COM_FRAME		0x08
#  define COM_PARITY		0x04
#  define COM_OVERRUN		0x02
#  define COM_DATAREADY	0x01
   
#  define COM_CD		0x0080
#  define COM_RI		0x0040
#  define COM_DSR		0x0020
#  define COM_CTS		0x0010
#endif /* __linux__ && __KERNEL__ */

#include "wrdef.h"
#include "wrio.h"

/* --------------------------------------------------------------------- */
/*  */
/*  ReadSerialByte reads a byte from the serial buffer if one exists. */
/*  Normally, the hRadio will be used to specify which buffer to read from. */
/*  */

#if defined(USE_INTERRUPT) && defined(__KERNEL__)
#define SERBUFSIZE 32
static struct serbuf_t {
	int head, tail, count;
	unsigned char buf[SERBUFSIZE];
} serbuf[1+MAX_DEVICES];

static void wrserial_interrupt(int irq, void *dev_id, struct pt_regs *regs) {
	int hRadio = (int)dev_id;
	struct serbuf_t *sb = serbuf+hRadio;
	int base = RadioSettings[hRadio]->wIoAddr;
	if ( ! (inb_p(base+COM_LSR)&1) ) return;
	sb->buf[sb->tail++] = inb_p(base+COM_RBR);
	sb->tail &= 31;
	if ( ++sb->count == 24 ) outb_p(9, base+COM_MCR);
}
#endif /* defined(USE_INTERRUPT) && defined(__KERNEL__) */

BOOL ReadSerialByte(int hRadio, PBYTE c, unsigned long ms)
{
#ifndef __KERNEL__
  /* Try to read; if failed, select, then read. Assumes O_NONBLOCK. */
	fd_set fds;
	struct timeval tv;
	int fd = RadioSettings[hRadio]->fd;
	if ( read(fd, c, 1) == 1 ) return TRUE;
	if ( errno != EAGAIN ) return FALSE;
	if ( ms == 0 ) return FALSE;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec = 0;
	tv.tv_usec = ms*1000;
	select(fd+1, &fds, NULL, NULL, &tv);
	if ( !FD_ISSET(fd, &fds) ) return FALSE;
	if ( read(fd, c, 1) != 1 ) return FALSE;
	return TRUE;
#else /* __KERNEL__ */
	int base = RadioSettings[hRadio]->wIoAddr;
	long timeout = GetTickCount()+ms;

#ifndef USE_INTERRUPT
	while ( ! (inb_p(base+COM_LSR)&1) ) {
	  if ( GetTickCount()-timeout >= 0 ) return FALSE;
		udelay(10);
	}
	*c = inb_p(base+COM_RBR);
	return TRUE;
#else /* USE_INTERRUPT */
	struct serbuf_t *sb = serbuf+hRadio;
	while ( sb->count ) { // Stricly increasing, so no cli() required
	  if ( GetTickCount()-timeout > 0 ) return FALSE;
		mdelay(1);
	}
	cli();
	*c = sb->buf[sb->head++];
	sb->head &= 31;
	if (--sb->count == 12) outb_p(11, base+COM_MCR);    // raise RTS
	sti();
	return TRUE;
#endif /* USE_INTERRUPT */

#endif /* __KERNEL__ */
}

BOOL SendSerialByte(int hRadio, BYTE c)
{
#ifndef __KERNEL__
	return (write(RadioSettings[hRadio]->fd, &c, 1)==1) ? TRUE : FALSE;
#else /* __KERNEL__ */
	int base = RadioSettings[hRadio]->wIoAddr;
	long timeout = GetTickCount()+100;
	while ( ! (inb_p(base+COM_MSR)&COM_CTS) ) {
		if ( GetTickCount()-timeout > 0 ) return FALSE;
		mdelay(1);
	}
	outb_p(c, base+COM_THR);
	return TRUE;
#endif /* __KERNEL__ */
}

void SetBaudRate(int hRadio, DWORD dwBaudRate)
{
#ifndef __KERNEL__
	struct termios tio;
	int b;
	if ( tcgetattr(RadioSettings[hRadio]->fd, &tio) ) perror("tcgetattr");
	switch ( dwBaudRate ) {
	case 9600: b = B9600; break;
	case 38400: b = B38400; break;
	case 115200: b = B115200; break;
	default: b = B9600; break;
	}
	cfsetispeed(&tio, b);
	cfsetospeed(&tio, b);
	if ( tcsetattr(RadioSettings[hRadio]->fd, TCSANOW, &tio) ) perror("tcsetattr");
#else /* __KERNEL__ */
	int base = RadioSettings[hRadio]->wIoAddr;
	unsigned long div = 115200L / dwBaudRate;
	outb_p(0x83, base+COM_LCR);
	outb_p(div&0xff, base+COM_DLL);
	outb_p(div>>8, base+COM_DLH);
	outb_p(0x3, base+COM_LCR);
#endif /* __KERNEL__ */
}


BOOL OpenSerialPort(int hRadio)
{
	/*  this function does no checking to see if the port is opened elsewhere */
	/*  especially within this source (check RadioSettings array) */

	/*  The iIrq value can be ignored if no interrupt handler is used */
	/*  or required (ie. if the serial driver provides buffering). */
#ifndef __KERNEL__
	struct termios tio;
	static char dev[] = "/dev/ttySx";
	int fd;
	
	dev[strlen(dev)-1] = '0'+(RadioSettings[hRadio]->iPort);
	fd = RadioSettings[hRadio]->fd = open(dev, O_RDWR|O_NONBLOCK);
	if ( fd < 0 ) return FALSE;
	
	if ( tcgetattr(fd, &tio) ) perror("tcgetattr");
	tio.c_cflag = B9600 | CLOCAL | CS8 | CREAD | CRTSCTS;
	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	tio.c_cc[VTIME] = 0;
	tio.c_cc[VMIN] = 1;
	if ( tcsetattr(fd, TCSANOW, &tio) ) perror("tcsetattr");

	RadioSettings[hRadio]->riInfo.iHWInterface = RHI_SERIAL;

	SetBaudRate(hRadio, 9600);

	return TRUE;
#else /* __KERNEL__ */
	/* From serialP.h */
	static int serialirq[4] = { 4, 3, 4, 3 };
	static int serialaddr[4] = { 0x3F8, 0x2F8, 0x3E8, 0x2E8 };
	RADIOSETTINGS *rs = RadioSettings[hRadio];

	int base = serialaddr[rs->iPort];
	int irq = serialirq[rs->iPort];

	rs->wIoAddr = base;
	rs->iIrq = irq;
	rs->riInfo.iHWInterface = RHI_SERIAL;

#ifdef USE_INTERRUPT
	serbuf[hRadio].head = serbuf[hRadio].tail = serbuf[hRadio].count = 0;
	if ( request_irq(irq, wrserial_interrupt, SA_INTERRUPT,
			 "linradio", (void*)hRadio) ) {
		TRACE(("request_irq failed"));
		return FALSE;
	}
#endif /* USE_INTERRUPT */
	request_region(base, 8, "linradio");

	SetBaudRate(hRadio, 9600);

	outb_p(8, base+COM_MCR);
	inb_p(base+COM_RBR);
	inb_p(base+COM_IIR);
	inb_p(base+COM_MSR);
	outb_p(0, base+COM_IER);

	outb_p(1, base+COM_IER);  // issue read interrupts
	outb_p(11, base+COM_MCR);

	return TRUE;
#endif /* __KERNEL__ */
}

/* ----------------------------------- */
/*  */
/*  The CloseSerialPort function releases any resources allocated with */
/*  OpenSerialPort. */
/*  */
void CloseSerialPort(int hRadio)
{
#ifndef __KERNEL__
	close(RadioSettings[hRadio]->fd);
#else /* __KERNEL__ */
	release_region(RadioSettings[hRadio]->wIoAddr, 8);
#  ifdef USE_INTERRUPT
	free_irq(RadioSettings[hRadio]->iIrq, (void*)hRadio);
#  endif /* USE_INTERRUPT */
#endif /* __KERNEL__ */
}
