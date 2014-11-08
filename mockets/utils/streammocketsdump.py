#!/usr/bin/env python
#
# Simple stream mockets packet dump utility
#
# Author: Mauro Tortonesi <mauro@ferrara.linux.it>
# Revision: $Revision: 1.3 $
# Last updated: $Date: 2005/09/21 14:43:25 $
#
# This program requires the pypcap and dpkt extension modules, which can be
# found at the following URLs respectively:
#
# http://monkey.org/~dugsong/pypcap/
# http://monkey.org/~dugsong/dpkt/
#
# You will also need to install libpcap (on Unix):
#
# http://www.tcpdump.org/
#
# or winpcap (on Windows):
#
# http://www.winpcap.org/
#

import getopt, sys, struct
import dpkt, pcap


class StreamPacket(dpkt.Packet):
    """Stream Mocket Transport Protocol."""

    __byte_order__ = '!'
    __hdr__ = (
        ('flags', 'H', 0),
        ('crtl_seqnum', 'B', 0),
        ('crtl_ack', 'B', 0),
        ('seqnum', 'I', 0)
        ('ack', 'I', 0)
        ('win_len', 'H', 0),
        ('payload_len', 'H', 0),
        )
    
    HEADER_LENGTH = 16

    _flags = {
        0x01: 'SYN',
        0x02: 'SYNACK',
        0x04: 'FIN',
        0x08: 'FINACK',
        0x10: 'SUSPEND',
        0x20: 'SUSPEND_ACK',
        0x40: 'RESUME',
        0x80: 'RESUME_ACK'
    }
        
    def unpack(self, buf):
        dpkt.Packet.unpack(self, buf)
        self.len = len(buf)
        self.flags_set = [ StreamPacket._flags[x] for x in StreamPacket._flags.keys() if (self.flags & 0xFFFF) & x ]
        #print "flags set=%s" % ', '.join(self.flags_set)

    def __str__(self):        
        return "len %d, flags: %s, crtl_seqnum %d, crtl_ack %d, seqnum %d, ack %d, win_len %d, payload_len %d " % \
               (self.len, ' '.join(self.flags) or "none", self.crtl_seqnum, self.crtl_ack, self.seqnum, self.ack, self.win_len, self.payload_len)

   
def usage():
    print >> sys.stderr, 'usage: %s [-i device] [pattern]' % sys.argv[0]
    sys.exit(1)


def main():
    opts, args = getopt.getopt(sys.argv[1:], 'i:h')
    name = None
    for o, a in opts:
        if o == '-i': name = a
        else: usage()
        
    pc = pcap.pcap(name)
    pc.setfilter('udp ' + ' '.join(args))
    decode = { pcap.DLT_LOOP:   dpkt.loopback.Loopback,
               pcap.DLT_NULL:   dpkt.loopback.Loopback,
               pcap.DLT_EN10MB: dpkt.ethernet.Ethernet }[pc.datalink()]
    try:
        print 'listening on %s: %s' % (pc.name, pc.filter)
        for ts, pkt in pc:
            udpPacket = decode(pkt).data.data
            print ts, udpPacket.sport, '>', udpPacket.dport, StreamPacket(udpPacket.data)
    except KeyboardInterrupt:
        nrecv, ndrop, nifdrop = pc.stats()
        print '\n%d packets received by filter' % nrecv
        print '%d packets dropped by kernel' % ndrop

if __name__ == '__main__':
    main()

# vim: et ts=4 sw=4

