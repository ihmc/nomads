#!/usr/bin/env python
#
# Helper script to simulate exponential blackouts with NistNET
#
# Author: Mauro Tortonesi <mauro.tortonesi@unife.it>
# Revision: $Revision$
# Last updated: $Date$
#

import getopt, sys

from os import system
from random import seed, expovariate
from time import sleep, time


def usage():
    return 'Usage is: %s [--mean-off=REALNUM] [--mean-on=REALNUM]' % sys.argv[0]


def main():
    # initialize defaults
    meanoff = 1.0 # default mean off is 1 second
    meanon = 20.0 # default mean on is 20 seconds
    
    opts, args = getopt.getopt(sys.argv[1:], 'h', ['mean-off=', 'mean-on='])
    name = None
    for o, a in opts:
        if o == '-h': 
            print usage()
            sys.exit(0)
        elif o == '--mean-off': 
            meanoff = float(a)
        elif o == '--mean-on': 
            meanon = float(a)
        else: 
            print "Error parsing command-line option: unknown/unrecognized option %s" % o 
            #print >> sys.stderr, usage()
            sys.exit(1)
    if len(args):
        print "Error parsing command-line arguments: no arguments allowed" 
        #print >> sys.stderr, usage()
        sys.exit(2)
        
    seed()
    
    while 1:
        print time()
        system("cnistnet -a 10.2.34.150 192.168.5.10 --drop 0%; cnistnet -a 192.168.5.10 10.2.34.150 --drop 0%")
        sleep(expovariate(1.0/meanon))
        print time()
        system("cnistnet -a 10.2.34.150 192.168.5.10 --drop 100%; cnistnet -a 192.168.5.10 10.2.34.150 --drop 100%")
        sleep(expovariate(1.0/meanoff))

if __name__ == '__main__':
    main()

# vim: et ts=4 sw=4

