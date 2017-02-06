# Perl stubs to the winradio loadable module driver.
# (C) 2000 <pab@users.sourceforge.net>

########## Modes

$RADIO_CW  = 0;
$RADIO_AM  = 1;
$RADIO_FMN = 2;
$RADIO_FMW = 3;
$RADIO_LSB = 4;
$RADIO_USB = 5;
$RADIO_FMM = 6;
$RADIO_FM6 = 7;

########## Reading

sub getlong {
#    $IOCTL_GETLONG = 0x80047700;
    $IOCTL_GETLONG = 0x80048C00;
    local($code) = @_;  shift(@_);
    local($fd) = @_;    shift(@_);
    $buf = pack("L", 0);
    ioctl($fd, $IOCTL_GETLONG+$code, $buf);
    unpack("L", $buf);
}

sub getpower  { getlong(0x00, @_); }
sub getmode   { getlong(0x02, @_); }
sub getmute   { getlong(0x04, @_); }
sub getattn   { getlong(0x06, @_); }
sub getvol    { getlong(0x08, @_); }
sub getfreq   { getlong(0x0A, @_); }
sub getbfo    { getlong(0x0C, @_); }
sub getss     { getlong(0x12, @_); }
sub getifs    { getlong(0x13, @_); }
sub getdescr  {
#    $IOCTL_GET256 = 0x81007700;
    $IOCTL_GET256 = 0x81008C00;
    local($fd) = @_;    shift(@_);
    $buf = pack("c256");
    ioctl($fd, $IOCTL_GET256+0x15, $buf);
    $buf;
}
sub getagc    { getlong(0x16, @_); }
sub getifg    { getlong(0x18, @_); }
sub getmaxvol { getlong(0x20, @_); }

sub printinfo {
    print "descr=", getdescr(@_), "\n";
    print "power=", getpower(@_), " freq=", getfreq(@_),
          " mode=", getmode(@_), " vol=", getvol(@_),
          " mute=", getmute(@_), " atten=", getattn(@_),
          " maxvol=", getmaxvol(@_), "\n";
    print "SS=", getss(@_), "\n";
}

########## Writing

sub setlong {
#    $IOCTL_SETLONG = 0x40047700;
    $IOCTL_SETLONG = 0x40048C00;
    local($code) = @_;  shift(@_);
    local($fd)  = @_;   shift(@_);
    local($val)  = @_;  shift(@_);
    $buf = pack("L", $val);
    ioctl($fd, $IOCTL_SETLONG+$code, $buf);
}

sub setpower  { setlong(0x01, @_); }
sub setmode   { setlong(0x03, @_); }
sub setmute   { setlong(0x05, @_); }
sub setattn   { setlong(0x07, @_); }
sub setvol    { setlong(0x09, @_); }
sub setfreq   { setlong(0x0B, @_); }
sub setbfo    { setlong(0x0D, @_); }
sub setifs    { setlong(0x14, @_); }
sub setagc    { setlong(0x17, @_); }
sub setifg    { setlong(0x19, @_); }
