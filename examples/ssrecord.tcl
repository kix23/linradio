#!../tcl/tclradio

# Print signal strength on 94MHz FMW every minute

winradio wr 0
wr -m FMW -f 94e6 -v 30
while { 1 } { puts "[exec date] [wr ss]"; after 60000 }
