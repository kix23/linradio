#!./tclradio

# Read commands from standard input, e.g. "-p 1 -f 98e6 -m FMW"

winradio wr 0x180

while 1 {
    gets stdin cmd
    puts [eval wr $cmd]
}
