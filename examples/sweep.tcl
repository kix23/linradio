#!../tcl/tclradio

winradio wr 1

wr -p 1 -m FMN -u 0 -a 0 -v 10

set f 88e6

while 1 {
  puts $f
  wr -f $f
  set f [expr $f + 10e3]
}
