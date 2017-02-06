#!../tcl/wishradio

# Simple control panel
# (C) 1999-2000 <pab@users.sourceforge.net>

winradio wr 1 # /dev/ttyS1
wr -p 1 -f 96e6 -m FMW -v 50 -f 105.5e6
set freq 105.5

proc tune { } { global freq; wr -f [expr $freq * 1e6] }

entry .freq -textvariable freq -width 10 -font {Helvetica 24}
bind .freq <Return> tune
bind .freq <KP_Enter> tune

button .up -text "+" -command { set freq [expr $freq + 0.5]; tune }
button .down -text "-" -command { set freq [expr $freq - 0.5]; tune }
pack .freq .up .down -side left -fill y

set mode FMW
menubutton .mode -menu .mode.menu -textvariable mode -relief raised -width 3
menu .mode.menu -tearoff 0
foreach i { CW LSB USB AM FMN FMW } {
    .mode.menu add command -label $i -command "set mode $i; wr -m $i"
}
pack .mode -side left -fill y
.mode.menu activate FMW
