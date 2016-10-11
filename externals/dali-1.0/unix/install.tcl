if {[lsearch [package names] Tk] == -1} {
    set usetk 0
} else {
    set usetk 1
}

proc Error {msg} {
    global usetk

    if {$usetk} {
        tk_messageBox -type ok -default ok -icon error -message \
	    "$msg"
	exit
    } else {
	puts stderr $msg
	puts stderr "Type <Enter to continue"
	gets stdin
	exit
    }
}


proc ShowInfo {msg} {
    global usetk

    if {$usetk} {
        tk_messageBox -type ok -default ok -icon info -message \
	    "$msg"
	exit
    } else {
	puts stderr $msg
	puts stderr "Type <Enter to continue"
	gets stdin
	exit
    }
}



set targetDir [info library]/dali1.0
if ![file writable [info library]] {
    Error "You do not have permission to write to [info library]"
}

if ![file exists $targetDir] {
    puts "Making directory $targetDir"
    if [catch {file mkdir $targetDir} err] {
        Error "Error making installation directory $targetDir. Error was: $err"
    }
}

foreach i [concat [glob lib/*[info sharedlibextension]] pkgIndex.tcl] {
   # Copy all DLLs and pkgIndex.tcl into targetDir
   puts "Copying $i to $targetDir"
   file copy -force $i $targetDir
}


ShowInfo "Installation complete"
