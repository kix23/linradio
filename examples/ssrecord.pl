#!/usr/bin/perl

require "winradio.pl";

open(WR, "/dev/winradioS0");

setpower(WR, 1);
setfreq(WR, 94000000);
setmode(WR, $RADIO_FMW);
setmute(WR, 0);
setattn(WR, 0);
setvol(WR, 20);

printinfo(WR);

while (1) {
    $date = `date`;
    chop $date;
    print $date, " ", getss(WR), "\n";
    sleep(60);
}
