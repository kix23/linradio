#!../tcl/wishradio

# TK control panel for winradio receivers.
# (C) 1999-2000 <pab@users.sourceforge.net>

# Defaults and preferences
set new_begin "88 MHz"
set new_end "108 MHz"
set scanwidth 800
set wrconfig ""
set scandelay 5
set ifgain 0

if [catch { set wrstaterc $env(WRSTATERC) }] \
          { set wrstaterc $env(HOME)/.wrstaterc }
if [catch { set wrfreqdb $env(WRFREQDB) }] \
          { set wrfreqdb $env(HOME)/.wrfreqdb }

if [catch { catch {source $wrstaterc};
            if { $argc > 0 } { set port $argv };
            winradio wr $port } err] {
    puts "Bad configuration ($err)"
    set port -1
    foreach p { 0x180 0 1 } {
	if [catch { winradio wr $p} err] {} { set port $p; break }
	puts "Port $p failed ($err)."
    }
    if { $port == -1 } {
	puts "####"
        puts "#### Couldn't find a radio. Will run in demo mode.";
	puts "####"
	proc wr { args } { return "0" }
    }
}

catch { eval wr $wrconfig }

set hwdescr "[wr version]"
puts "Found a '$hwdescr' on port $port"

set width 800

wm title . "WR control panel - $hwdescr on port $port"

########################### TOP PANEL ###########################

set top .top
frame $top -borderwidth 4
pack $top -side top -fill x

########################### TOP PANEL, CONTROLS ###########################

##### Power, mute, local

# Boolean state, straight from the radio
proc cb_flag { flag button } {
    if [ wr $flag ] { wr -$flag 0 } { wr -$flag 1 }
    if [ wr $flag ] { $button configure -relief sunken } \
	            { $button configure -relief raised }
}
proc make_flagbut { button label flag } {
    button $button -text $label -command "cb_flag $flag $button"
    if [ wr $flag ] { $button configure -relief sunken }
}

set gb $top.genbuttons
frame $gb
pack $gb -side left -fill y -padx 2

make_flagbut $gb.power Power p
make_flagbut $gb.mute Mute u
make_flagbut $gb.local Local a
make_flagbut $gb.agc AGC A

button $gb.exit -text Exit -command save_exit
pack $gb.power $gb.mute $gb.local $gb.agc $gb.exit -fill x -side top

##### Volume scale

scale $top.vol -orient vertical -from 100 -to 0 -command "wr -v" \
	-showvalue 0
pack $top.vol -side left -fill y
$top.vol set [wr v]

########################### TOP PANEL, TUNER ###########################

set tuner $top.tuner
frame $tuner
pack $tuner -side left -fill y

##### Freq

# float <-> frequencies(with units)
proc displayfreq { f } {
    if { $f < 10e6 } { return [format "%.2f kHz" [expr $f * 1e-3]] }
    return [format "%.5f MHz" [expr $f * 1e-6]]
    if { $f < 30e6 } { return "[expr $f * 1e-3] kHz" }
    return "[expr round($f * 1e-2) * 1e-4] MHz"
}
proc displayfrequ { f } {
    if { $f < 30e6 } { return [expr $f * 1e-3] }
    return [expr $f * 1e-6]
}
proc parsefreq { f } {
    set f [string tolower $f]
    set k 1    
    if { [string first g "$f"] != -1 } { set k 1e9 }
    if { [string first m "$f"] != -1 } { set k 1e6 }
    if { [string first k "$f"] != -1 } { set k 1e3 }
    set f [string trimright $f " kmhz"]
    set f [expr $f * $k]
    if { $f < 1500 } { set f [expr $f * 1e6] }
    if { $f < 150000 } { set f [expr $f * 1e3] }
    return $f
}

set freq $tuner.freq
entry $freq -font {Helvetica 24 bold} -width 14 \
	-textvariable freqtext -background white
set freqtext [displayfreq [wr f]]
bind $freq <1> { set freqtext "" }
pack $freq -side top

# set bfo $tuner.bfo
# entry $bfo -textvariable bfotext -background white -width 5
# set bfotext 0
# bind $bfo <Return> { puts $bfotext; wr -i $bfotext }
# pack $bfo -side top

##### Modes

set modes { AM FMN FMW CW LSB USB }
proc cb_mode { mode } {
    global modes bmode_AM bmode_FMN bmode_FMW bmode_CW bmode_LSB bmode_USB
    wr -m $mode
    foreach i $modes {
	set bvar bmode_$i
	if { $i == $mode } { set relief sunken } {set relief raised }
	eval $$bvar configure -relief $relief
    }
}

frame $tuner.modes1
foreach i { AM FMN FMW } {
  set b $tuner.modes1.m$i
  set bmode_$i $b
  button $b -text $i -width 4 -command "cb_mode $i"
  pack $b -side left
}
frame $tuner.modes2
foreach i { CW LSB USB } {
  set b $tuner.modes2.m$i
  set bmode_$i $b
  button $b -text $i -width 4 -command "cb_mode $i"
  pack $b -side left
}
pack $tuner.modes1 $tuner.modes2 -side top
cb_mode [wr m]

# Adjust mode with keyboard
bind all <Control-KeyPress-m> { cb_mode AM }
bind all <Control-KeyPress-n> { cb_mode FMN }
bind all <Control-KeyPress-w> { cb_mode FMW }
bind all <Control-KeyPress-c> { cb_mode CW }
bind all <Control-KeyPress-l> { cb_mode LSB }
bind all <Control-KeyPress-u> { cb_mode USB }

##### Label

label $tuner.label -font {Helvetica 16} -text "WR-Kit"
pack $tuner.label -side top

########################### TOP/RIGHT PANEL ###########################

set db $top.db

frame $db -borderwidth 2
pack $db -side left -fill both -expand 1 -padx 4

#label $db.label -text "This place intentionally left blank" -background white
#pack $db.label -expand 1

########################### SCOPE FRAME ###############################

# spectrum scope not initialized yet
set scan_begin 150e3
set scan_end 1.5e9

set ss .bottom
frame $ss -borderwidth 2 -relief raised
pack $ss -side top -pady 2 -pady 2 -fill x

########################### SCOPE CONTROLS ############################

set ssc $ss.controls
frame $ssc
pack $ssc -side top -padx 2 -pady 2 -fill x

entry $ssc.begin -textvariable new_begin -width 14 -background white
entry $ssc.end -textvariable new_end -width 14 -background white
bind $ssc.begin <1> { set new_begin "" }
bind $ssc.end <1> { set new_end "" }

# Main scan function
set sc_active 0
proc ssw_setup { begin end } {
    global scan_begin scan_end ssw scanwidth new_begin new_end curmax
    if { $end < [expr $begin + 1000] } { set end [expr $begin + 1000] }
    if { $begin < 150e3 } { set begin 150e3 }
    if { $end > 1.5e9 } { set end 1.5e9 }
    set scan_begin $begin
    set scan_end $end
    set new_begin [displayfreq $begin]
    set new_end [displayfreq $end]
    $ssw addtag del all; $ssw delete del
    set curmax { }

    set step [expr ( $end - $begin ) / $scanwidth]
    set rule 1
    foreach r { 1e3 5e3 1e4 5e4 1e5 5e5 1e6 5e6 1e7 5e7 } {
	if { [expr $step * 300] > $r } { set rule $r }
    }
    for { set f [expr floor($begin/$rule) * $rule] } \
	    { $f < $end } { set f [expr $f + $rule] } {
	set i [markfreq ruler grey $f]
	$ssw create text $i 32 -anchor s -tags ruler -fill grey \
		-text [displayfrequ [expr $begin + $step * $i]]
    }
}

proc showdb { } {
    global db ssw scanwidth scanheight scan_begin scan_end curmax
    global scanbw scandelay sc_active
    for { set i [$db.list size] } { [incr i -1] >= 0 } { } {
	set l [$db.list get $i]
	if [isfreq $l] {
	    set f [insparse $l]
	    set x [markfreq ruler red $f]
#	    $ssw create text $x 20 -anchor s -tags ruler -fill red \
#		    -text [displayfrequ $f]
	}
    }
}

proc scan { begin end } {
    global ssw scanwidth scanheight scan_begin scan_end curmax
    global scanbw scandelay sc_active

    # Kill current thread, if any
    set sc_active 0
    after [expr $scandelay * 2]

    set step [expr ( $end - $begin ) / $scanwidth]
    if { $begin != $scan_begin || $end != $scan_end } {
	ssw_setup $begin $end
	set curmax { }
    }
    global sc_line sc_linemax sc_max sc_oldfreq sc_oldmode
    set sc_line { }
    set sc_linemax { }
    set sc_max { }
    set sc_oldfreq [wr f]
    set sc_oldmode [wr m]
    case $scanbw in {
	{ 2.5kHz } { set mode CW }
	{ 6kHz } { set mode AM }
	{ 17kHz } { set mode FMN }
	{ 230kHz } { set mode FMW }
	{ Auto } {
	    set mode FMW
	    # min peak width in pixels FMN:2 AM:6 CW:12
	    if { $step < 30e3 } { set mode FMN}
	    if { $step < 2e3 } { set mode AM }
	    if { $step < 400 } { set mode CW }
	}
	{ Current } { set mode [wr m] }
    }
    $ssw delete scaninfo
    $ssw create text 1 1 -anchor nw -tags "ruler scaninfo" \
	    -text "Sweep mode: $mode    pixel step: $step Hz"
    wr -m $mode -f $begin
    set sc_active 1
    after $scandelay "scanstep $step $begin 0 0"
    showdb
}

proc scanstep { step f i pos } {
    global scanheight curmax ssw scanwidth scandelay ssc
    global sc_line sc_linemax sc_max sc_oldfreq sc_oldmode sc_active

    set ss [wr ss]
    set y [expr $scanheight - 5 - $ss]
    append sc_line " $i $y"

    set curm 0
    catch { set curm [lindex $curmax $pos] }
    if { $ss > $curm } { set curm $ss }
    append sc_max " $curm"
    append sc_linemax " $i [expr $scanheight - 5 - $curm]"

    $ssw delete scanner
    if { $i < $scanwidth && $sc_active } {
	$ssw create line $i $scanheight $i $y -tag scanner
	set f [expr $f + $step * 2]
	set i [expr $i + 2]
	incr pos
	wr -f $f
	after $scandelay "scanstep $step $f $i $pos"
    } {
	set sc_active 0
	$ssc.scan configure -relief raised
	wr -f $sc_oldfreq -m $sc_oldmode
	setfreq $sc_oldfreq
	$ssw delete spect
	eval $ssw create line $sc_linemax -fill red -tags spect
	eval $ssw create line $sc_line -fill black -tags spect
	set curmax $sc_max
    }
}

# Scan/refresh commands
proc scan_request { } {
    global ssc new_begin new_end
    $ssc.scan configure -relief sunken
    scan [parsefreq $new_begin] [parsefreq $new_end]
}
bind $ssc.end <Return> scan_request
bind $ssc.end <KP_Enter> scan_request

button $ssc.scan -text "Sweep" -command {
    if { $sc_active } { set sc_active 0  } {
	$ssc.scan configure -relief sunken
	scan [parsefreq $new_begin] [parsefreq $new_end] 
    }
}

# Move range up/down/out
proc scan_shift { k } {
    global new_begin new_end
    set fb [parsefreq $new_begin]
    set fe [parsefreq $new_end]
    set delta [expr $fe - $fb]
    set new_begin [displayfreq [expr $fb + $delta * $k]]
    set new_end [displayfreq [expr $fe + $delta * $k]]
}
button $ssc.left -text "<<" -command "scan_shift -1"
button $ssc.right -text ">>" -command "scan_shift 1"
button $ssc.zoomout -text "<>" -command {
    set d [expr $scan_end - $scan_begin]
    set b $scan_begin
    set e $scan_end
    ssw_setup [expr $scan_begin - $d] \
              [expr $scan_end + $d]
    $ssw create rect [xoffreq $b] 32 [xoffreq $e] $scanheight \
	    -tags "ruler rangerect" -fill green -outline ""
    $ssw lower rangerect
  
}

# Resize the scrollable spectrum window
proc ssresize { k } {
    global ssw scanwidth scanheight curspect curmax scan_begin scan_end
    set scanwidth [expr int($scanwidth * $k)]
    $ssw configure -scrollregion "0 0 $scanwidth $scanheight"
    $ssw addtag del all; $ssw delete del
    set curspect ""
    set curmax ""
    set scan_begin 0
    set scan_end 1
}
button $ssc.enlarge -text {<[  ]>} -command "ssresize 2"
button $ssc.shrink -text {[><]} -command "ssresize 0.5"

# Adjust scanning delay
label $ssc.delayl -text "Sweep delay:"
entry $ssc.delay -width 2 -background white -textvariable scandelay

label $ssc.ifgainl -text "IF Gain:"
entry $ssc.ifgain -width 4 -background white -textvariable ifgain
bind $ssc.ifgain <Return> { wr -ifg $ifgain }
bind $ssc.ifgain <KP_Enter> { wr -ifg $ifgain }

# Adjust scanning bandwidth
set scanbw Auto
tk_optionMenu $ssc.bw scanbw Auto 2.5kHz 6kHz 17kHz 230kHz Current
$ssc.bw configure -width 6

pack $ssc.begin $ssc.end -side left
pack $ssc.scan -side left
pack $ssc.bw -side left
pack $ssc.left $ssc.zoomout $ssc.right -side left
pack $ssc.enlarge $ssc.shrink -side left
pack $ssc.delayl $ssc.delay -side left
pack $ssc.ifgainl $ssc.ifgain -side left

#button $ssc.ss -text "SS" -command { puts "[expr [wr ss] * 120 / 254]" }
#pack $ssc.ss -side left

########################### SCOPE DISPLAY #########################

set scanheight 192

set ssw $ss.graph
set ssfs $ss.fscroll
scrollbar $ssfs -orient horiz -command "$ssw xview"
canvas $ssw -width $width -height $scanheight -background white \
	-scrollregion "0 0 $scanwidth $scanheight" \
	-xscrollcommand "$ssfs set" -cursor arrow \
	-borderwidth 2 -relief sunken -cursor crosshair
pack $ssw $ssfs -side top -fill x

################## Initial message ######################
set x [expr $width / 2]
$ssw create text $x 80 -tags ruler -font {Helvetica 12} -justify center -text\
"This program is free software; you can redistribute it and/or modify it under the terms of the
GNU General Public License as published by the Free Software Foundation; Version 2, June 1991.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program;
if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA"
##########################################################

# Convert canvas coordinates -> freq
proc freqofx { x } {
    global scanwidth scan_begin scan_end ssw curspect
    set step [expr ( $scan_end - $scan_begin ) / $scanwidth]
    return [expr $scan_begin + $step * $x]
}
proc xoffreq { f } {
    global scan_begin scan_end scanwidth
    set step [expr ( $scan_end - $scan_begin ) / $scanwidth]
    return [expr ($f - $scan_begin) / $step]
}

# Draw a vertical line at some freq
proc markfreq { tag color f } {
    global ssw scanheight
    set i [xoffreq $f]
    $ssw create line $i 32 $i $scanheight -fill $color -tags $tag
    return $i
}

proc setfreq { f } {
    global ssw freqtext
    $ssw delete curfreq
    markfreq curfreq blue $f
    set freqtext [displayfreq $f]
    wr -f $f
}

# Tune by clicking/dragging B1
bind $ssw <1> { setfreq [freqofx [$ssw canvasx %x]] }
bind $ssw <B1-Motion> { setfreq [freqofx [$ssw canvasx %x]] }
bind $freq <Return> { setfreq [parsefreq $freqtext] }
bind $freq <KP_Enter> { setfreq [parsefreq $freqtext] }

# Adjust tuning with keyboard
proc deltafreq { df } { setfreq [expr [wr f] + $df] }
bind all <KeyPress-Prior>      { deltafreq +50e3 }
bind all <KeyPress-Next>       { deltafreq -50e3 }
bind all <KeyPress-Up>         { deltafreq +1e3 }
bind all <KeyPress-Down>       { deltafreq -1e3 }
bind all <Shift-Up>            { deltafreq +10 }
bind all <Shift-Down>          { deltafreq -10 }
bind all <KP_Add>              { deltafreq +1 }
bind all <KP_Subtract>         { deltafreq -1 }

# Scroll by dragging B2
bind $ssw <2> { $ssw scan mark [expr %x / 4] [expr %y / 4] }
bind $ssw <B2-Motion> { $ssw scan dragto [expr %x / 4] [expr %y / 4] }

# Select region by dragging B3
bind $ssw <3> {
    $ssw delete rangerect 
    set rangemark [$ssw canvasx %x]
    set new_begin [displayfreq [freqofx $rangemark]]
}
bind $ssw <B3-Motion> {
    $ssw delete rangerect
    $ssw create rect $rangemark 32 [$ssw canvasx %x] $scanheight \
	    -tags "ruler rangerect" -fill green -outline ""
    $ssw lower rangerect
}
bind $ssw <B3-ButtonRelease> {
    set f [freqofx [$ssw canvasx %x]]
    if { $f > [parsefreq $new_begin] } { set new_end [displayfreq $f] } {
	$ssw delete rangerect 
	set new_begin [displayfreq $scan_begin]
	set new_end [displayfreq $scan_end]
    }
}

########################################################################

scrollbar $db.scroll -command "$db.list yview"
listbox $db.list -setgrid 1 -yscroll "$db.scroll set" -background white \
	-font fixed -height 7
pack $db.list -expand 1 -fill x -side left
pack $db.scroll -fill y -side left

proc insparse { l } {
    set l [string tolower $l]
    set m [string first m $l]
    set k [string first k $l]
    if { $k < 0 || $m >= 0 && $m < $k } {
	return [expr [string trim [string range $l 0 $m] " `-m"] * 1e6]
    } {
	return [expr [string trim [string range $l 0 $k] " `-k"] * 1e3]
    }
}
proc dbaddraw { text } {
    global db
    set f [insparse $text]
    set i 0
    set l [$db.list size]
    while { $i < $l && $f > [insparse [$db.list get $i]] } \
	    { incr i }
    $db.list insert $i $text
    $db.list selection clear 0 end; $db.list selection set $i
    $db.list see $i
}
proc dbaddband { begin end name comment } {
    dbaddraw [format "%-14.14s- %-14.14s    | %-20.20s | %s" \
	    [displayfreq $begin] [displayfreq $end] $name $comment]
}
proc dbaddfreq { freq mode kind name comment } {
    dbaddraw [format "  `- %-14.14s| %-3.3s | %-6.6s | %-20.20s | %s" \
	    [displayfreq $freq] $mode $kind $name $comment]
}

proc isfreq { l } { return [string match "  `- *" $l] }
proc dbselect { i } {
    global db
    $db.list selection clear 0 end; $db.list selection set $i
    $db.list see $i
    set l [$db.list get $i]
    if [isfreq $l] { setfreq [parsefreq [string range $l 5 18]]
	             cb_mode [string trim [string range $l 21 23]] } \
		   { ssw_setup [parsefreq [string range $l 0 13]] \
		               [parsefreq [string range $l 16 29]] }
}

bind $db.list <1> { dbselect [$db.list nearest %y] }

bind all <Alt-a> { dbaddfreq [wr f] [wr m] ? ? ? }
bind all <Alt-b> { dbaddband $scan_begin $scan_end ? ? }
bind all <Alt-d> \
	{ set i [$db.list curselection]; $db.list delete $i; dbselect $i }
bind all <Alt-Prior> { $db.list yview scroll -1 pages }
bind all <Alt-Next> { $db.list yview scroll 1 pages }
bind all <Alt-Home> {
    set f [wr f]
    set i [$db.list size]
    while { $i > 0 && ( [incr i -1; set e [$db.list get $i]; isfreq $e] ||
                        $f < [insparse $e] ) } { }
    if { $i >= 0 } { dbselect $i }
}
bind all <Alt-End> { catch { dbselect [expr [$db.list curselection] + 1] } }
bind all <Alt-Up> {
    set i [$db.list curselection]
    set fb [isfreq [$db.list get $i]]
    while { $i >= 0 && [incr i -1; isfreq [$db.list get $i]] != $fb } { }
    if { $i >= 0 } { dbselect $i }
}
bind all <Alt-Shift-Up> {
    set i [$db.list curselection]
    set l [$db.list get $i]
    if [isfreq $l] {
	set kind [string range $l 27 32]
	while { $i >= 0 && 
 	  [string range [$db.list get [incr i -1]] 27 32] != $kind } { }
	if { $i >= 0 } { dbselect $i }
    }
}
bind all <Alt-Down> {
    set i [$db.list curselection]
    set sz [$db.list size]
    set fb [isfreq [$db.list get $i]]
    while { $i < $sz && [incr i; isfreq [$db.list get $i]] != $fb } { }
    if { $i < $sz } { dbselect $i }
}
bind all <Alt-Shift-Down> {
    set i [$db.list curselection]
    set l [$db.list get $i]
    set sz [$db.list size]
    if [isfreq $l] {
	set kind [string range $l 27 32]
	while { $i < $sz && 
 	        [string range [$db.list get [incr i]] 27 32] != $kind } { }
		if { $i < $sz } { dbselect $i }
	    }
}

proc dbsave { } {
    global db wrfreqdb
    puts -nonewline "Saving database... "
    set f [open $wrfreqdb.tmp w]
    for { set i [expr [$db.list size] - 1] } { $i >= 0 } { incr i -1 } {
	puts $f "dbaddraw \{[$db.list get $i]\}"
    }
    close $f
    file rename -force $wrfreqdb.tmp $wrfreqdb
    puts "done"
}

proc dbload { } {
    global db wrfreqdb
    puts -nonewline "Loading database from $wrfreqdb... "
    $db.list delete 0 end
    source "$wrfreqdb"
    puts "done"
}

bind . <Alt-s> { dbsave }

proc dbedit { i } {
    global db dbe_sel
    set dbe_sel $i
    set l [$db.list get $i]
    set e .dbedit
    toplevel $e
    if [isfreq $l] {
	global dbe_freq dbe_mode dbe_kind dbe_name dbe_comm
	set dbe_freq [displayfreq [parsefreq [string range $l 5 18]]]
	set dbe_mode [string trim [string range $l 21 23]]
	set dbe_kind [string trim [string range $l 27 32]]
	set dbe_name [string trim [string range $l 36 55]]
	set dbe_comm [string range $l 59 end]
	entry $e.freq -textvariable dbe_freq -background white -width 15
	entry $e.mode -textvariable dbe_mode -background white -width 4
	entry $e.kind -textvariable dbe_kind -background white -width 7
	entry $e.name -textvariable dbe_name -background white -width 21
	entry $e.comm -textvariable dbe_comm -background white -width 21
	set validate {
	    $db.list delete $dbe_sel
	    dbaddfreq [parsefreq $dbe_freq] $dbe_mode $dbe_kind \
		    $dbe_name $dbe_comm
	    destroy .dbedit
	}
	button $e.ok -text "OK" -command $validate
	button $e.cancel -text "Cancel" -command "destroy $e"
	foreach w "$e.freq $e.mode $e.kind $e.name $e.comm" \
		{ bind $w <Return> $validate; bind $w <KP_Enter> $validate }
	pack $e.freq $e.mode $e.kind $e.name $e.comm $e.ok $e.cancel -side top
	focus $e.kind
    } {
	global dbe_begin dbe_end dbe_name dbe_comm
	set dbe_begin [displayfreq [parsefreq [string range $l 0 13]]]
	set dbe_end [displayfreq [parsefreq [string range $l 16 29]]]
	set dbe_name [string trim [string range $l 36 55]]
	set dbe_comm [string range $l 59 end]
	entry $e.begin -textvariable dbe_begin -background white -width 15
	entry $e.end -textvariable dbe_end -background white -width 15
	entry $e.name -textvariable dbe_name -background white -width 21
	entry $e.comm -textvariable dbe_comm -background white -width 21
	set validate {
	    $db.list delete $dbe_sel
	    dbaddband [parsefreq $dbe_begin] [parsefreq $dbe_end] \
		    $dbe_name $dbe_comm
	    destroy .dbedit
	}
	button $e.ok -text "OK" -command $validate
	button $e.cancel -text "Cancel" -command "destroy $e"
	foreach w "$e.begin $e.end $e.name $e.comm" \
		{ bind $w <Return> $validate; bind $w <KP_Enter> $validate }
	pack $e.begin $e.end $e.name $e.comm $e.ok $e.cancel -side top
	focus $e.name
    }
}

bind . <Alt-e> { dbedit [$db.list curselection] }

if [catch dbload err] {
    puts "failed ($err), creating sample DB (see 'README' about editing)"
    catch { file rename -force $wrfreqdb $wrfreqdb.bak
            puts "** UNREADABLE DATABASE RENAMED TO $wrfreqdb.bak" }
    dbaddband 540e3 1700e3 "AM broadcast" ""
    dbaddband 50e6 54e6 "HAM 6m" ""
    dbaddband 88e6 108e6 "FM broadcast" ""
      dbaddfreq 96e6 FMW audio FM ""
      dbaddfreq 98e6 FMW audio FM ""
    dbaddband 144e6 148e6 "HAM 2m" ""
    dbaddband 430e6 440e6 "HAM 70cm" ""
    dbaddband 470e6 825e6 "TV 14-72" ""
      dbaddfreq 541.75e6 AM audio TV ""
      dbaddfreq 557.75e6 AM audio TV ""
    dbaddband 150e3 540e3 "SAMPLE DATABASE" "SEE 'README' ABOUT EDITING"
}

##### Exit

proc save_exit { } {
    global wrstaterc new_begin new_end scanwidth scandelay port
    set f [open $wrstaterc w]
    puts $f "set wrconfig \"[wr]\""
    foreach v { port new_begin new_end scanwidth scandelay } \
	    { puts $f "set $v \"[expr $$v]\"" }
    close $f
    dbsave
    exit
}
