#!/bin/bash

batchfile=`pwd`
batchfile+=/
batchfile+=nmsproxyshell.txt
../../cpp/linux/NMSProxyShell -load $batchfile
