proc rgb_display {r g b {win {}}} {

    global displayWinCount
    set ph [image create photo]
    rgb_to_photo $r $g $b $ph

    if {"$win"==""} {
	if {![info exists displayWinCount]} {
	    set displayWinCount 0
	}
	incr displayWinCount
	set win .disp$displayWinCount
    }

    set w [byte_get_width $r]
    set h [byte_get_height $r]
    if {![winfo exists $win]} {
	toplevel $win
	canvas $win.c -width [expr $w+2] -height [expr $h+2]
	pack $win.c
	$win.c config -width $w -height $h
	$win.c create image 0 0 -anchor nw -image $ph -tags photo
    } else {
	image delete [lindex [$win.c itemconfigure photo -image] 4]
	$win.c delete photo
	$win.c config -width $w -height $h
	$win.c create image 0 0 -anchor nw -image $ph -tags photo
	scan [wm geometry $win] %dx%d+%d+%d w0 h0 x1 y1
	wm geometry $win "${w}x$h+${x1}+${y1}"
    }
    bind $win <Button-3> "image delete $ph; destroy $win"
    bind $win <q>  "image delete $ph; destroy $win"
    return $win
}

proc byte_display {y {win {}}} {
    global displayWinCount


    set ph [image create photo]
    byte_to_photo $y $ph

    if {"$win"==""} {
	if {![info exists displayWinCount]} {
	    set displayWinCount 0
	}
	incr displayWinCount
	set win .disp$displayWinCount
    }

    set w [byte_get_width $y]
    set h [byte_get_height $y]
    if {![winfo exists $win]} {
	toplevel $win
	canvas $win.c -width [expr $w+2] -height [expr $h+2]
	pack $win.c
	$win.c config -width $w -height $h
	$win.c create image 0 0 -anchor nw -image $ph -tags photo
    } else {
	image delete [lindex [$win.c itemconfigure photo -image] 4]
	$win.c delete photo
	$win.c config -width $w -height $h
	$win.c create image 0 0 -anchor nw -image $ph -tags photo
	scan [wm geometry $win] %dx%d+%d+%d w0 h0 x1 y1
	wm geometry $win "${w}x$h+${x1}+${y1}"
    }
    bind $win <Button-3> "image delete $ph; destroy $win"
    bind $win <q>  "image delete $ph; destroy $win"
    return $win
}
