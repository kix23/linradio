/* Kernel device driver for WiNRADiO receivers. */
/* (C) 1999-2000 <pab@users.sourceforge.net> */
/* Original driver architecture and ioctl API (C) 1997 Michael McCormack */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <asm/segment.h>
#include <asm/io.h>
/* #include <asm/segment.h> */
#include <asm/uaccess.h>

#include "wrlib/wrapi.h"
#include "radio_ioctl.h"

#define MAX_RADIOS 256
/* Handles returned by OpenRadioDevice(). 0 means not detected yet. */
static int wrs[MAX_RADIOS] = { 0, };
static char inuse[MAX_RADIOS];

static const int radio_major = 82;

/***** Utilities *****/

static int port_of_minor(int minor) {
  /* ISA */
  if ( minor>=0 && minor<8 ) return 0x180 + (minor<<3);
  /* Serial */
  if ( minor>=0x80 && minor<0x84 ) return (minor-0x80);
  printk("linradio: wrong minor %d\n", minor);
  return 0x180;
}

static char fpu_state[108];
static void fpu_save() {
  __asm__ __volatile__ ( " fsave %0; fwait\n"::"m"(fpu_state[0]) );
}
static void fpu_rstor() {
  __asm__ __volatile__ ( " frstor %0;\n"::"m"(fpu_state[0]) );
}

static void put_fs_long(long x, void *arg) {
  copy_to_user(arg, &x, sizeof(x));
}
static long get_fs_long(void *arg) {
  static long tmp;
  copy_from_user(&tmp, arg, sizeof(tmp));
  return tmp;
}

/***** open / release *****/

static int radio_open(struct inode *inode, struct file *file) {
  int num = MINOR(inode->i_rdev);
  if ( num<0 || num>=MAX_RADIOS ) return -ENODEV;
  if ( !wrs[num] ) {
    /* Unknown - detect */
    int port = port_of_minor(num);
    int wr;
    printk("linradio: port (ISA/ttyS)0x%X... ", port);
    fpu_save();
    wr = OpenRadioDevice(port);
    fpu_rstor();
    if ( wr ) {
      printk("%s\n", GetDescr(wr));
      wrs[num] = wr;
      inuse[num] = 0;
    }
    else {
      printk("no device\n");
    }
  }
  if ( !wrs[num] ) return -ENODEV;
  if ( inuse[num] ) return -EBUSY;
  inuse[num] = 1;
  MOD_INC_USE_COUNT;
  return 0;
}

static int radio_release(struct inode *inode, struct file *file) {
  int num = MINOR(inode->i_rdev);
  if ( !wrs[num] || !inuse[num] ) {
    printk("linradio: BUG, trying to release wrs[%d]=%d\n", num, wrs[num]);
    return -1;
  }
  inuse[num] = 0;
  MOD_DEC_USE_COUNT;
  return 0;
}

/***** ioctl *****/

static int radio_ioctl(struct inode *inode, struct file *file, 
		       unsigned int cmd, unsigned long arg_ul) {
  void *arg = (void*)arg_ul;
  unsigned int x;
  int retval;
  int num = MINOR(inode->i_rdev);
  int wr;
  
  if ( num<0 || num>=MAX_RADIOS ) return -ENODEV;
  wr = wrs[num];
  if ( !wr ) return -ENODEV;

  if ( (retval=verify_area(VERIFY_READ, arg, sizeof (long))) ||
       (retval=verify_area(VERIFY_WRITE, arg, sizeof (long))) ) {
    printk("bad address %x\n", (unsigned int)arg);
    return retval;
  }
  
  retval = 0;
  fpu_save();

  switch(cmd) {
  case RADIO_GET_POWER: put_fs_long(GetPower(wr), arg); break;
  case RADIO_SET_POWER: SetPower(wr, get_fs_long(arg)); break;
  case RADIO_GET_VOL: put_fs_long(GetVolume(wr), arg); break;
  case RADIO_GET_MAXVOL: put_fs_long(GetMaxVolume(wr), arg); break;
  case RADIO_SET_VOL: SetVolume(wr, get_fs_long(arg)); break;
  case RADIO_GET_MUTE: put_fs_long(GetMute(wr), arg); break;
  case RADIO_SET_MUTE: SetMute(wr, get_fs_long(arg)); break;
  case RADIO_GET_ATTN: put_fs_long(GetAtten(wr), arg); break;
  case RADIO_SET_ATTN: SetAtten(wr, get_fs_long(arg)); break;
  case RADIO_GET_MODE: put_fs_long(GetMode(wr), arg); break;
  case RADIO_SET_MODE: SetMode(wr, get_fs_long(arg)); break;
  case RADIO_GET_FREQ: put_fs_long(GetFrequency(wr), arg); break;
  case RADIO_SET_FREQ: SetFrequency(wr, get_fs_long(arg)); break;
  case RADIO_GET_BFO: put_fs_long(GetBFOOffset(wr), arg); break;
  case RADIO_SET_BFO: SetBFOOffset(wr, get_fs_long(arg)); break;
  case RADIO_GET_IFS: put_fs_long(GetIFShift(wr), arg); break;
  case RADIO_SET_IFS: SetIFShift(wr, get_fs_long(arg)); break;
  case RADIO_GET_SS: put_fs_long(GetSignalStrength(wr), arg); break;
  case RADIO_GET_AGC: put_fs_long(GetAGC(wr), arg); break;
  case RADIO_SET_AGC: SetAGC(wr, get_fs_long(arg)); break;
  case RADIO_GET_IFG: put_fs_long(GetIFGain(wr), arg); break;
  case RADIO_GET_MAXIFG: put_fs_long(GetMaxIFGain(wr), arg); break;
  case RADIO_SET_IFG: SetIFGain(wr, get_fs_long(arg)); break;
  case RADIO_GET_DESCR:
    {
      char *descr = GetDescr(wr);
      x = strlen(descr) + 1;
      retval = verify_area(VERIFY_WRITE,arg, x);
      if ( retval )
	printk("bad address %x\n", (unsigned int)arg);
      else 
	copy_to_user(arg, descr, x);
    }
    break;
  default: retval = -EINVAL; break;
  }
  fpu_rstor();
  if ( retval ) return retval;
  return 0;
}

/***** fs *****/

static struct file_operations radio_fops = {
  open:    radio_open,
  ioctl:   radio_ioctl,
  release: radio_release,
};

/***** init / cleanup *****/

int init_module(void) {
  if ( register_chrdev(radio_major, "linradio", &radio_fops) ) {
    printk("Unable to get major for device radio.\n");
    return -EIO;
  }

  printk("linradio (version "WRVERSION"), will detect on open()\n");
  
  yield_hook = &fpu_rstor;
  reenter_hook = &fpu_save;

  return 0;
}

void cleanup_module(void) {
  int num;
  for ( num=0; num<MAX_RADIOS; ++num )
    if ( wrs[num] ) CloseRadioDevice(wrs[num]);
  unregister_chrdev(radio_major, "linradio");
}
