#!/usr/bin/env python
#
# Simple message mockets packet dump utility
#
# Author: Mauro Tortonesi <mauro@ferrara.linux.it>
# Revision: $Revision: 1.14 $
# Last updated: $Date: 2006/11/16 10:14:31 $
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


class DeliveryPrerequisites(object):
    DP_FLAG_HAVE_RELSEQ   = 0x001
    DP_FLAG_HAVE_UNRELSEQ = 0x002
    DP_FLAG_HAVE_CONTROL  = 0x004
    
    def __init__(self, buf, off):
        (self.flags,) = struct.unpack ("B", buf[off:off+1])
        tmpoff = 1
        if self.flags & DeliveryPrerequisites.DP_FLAG_HAVE_RELSEQ:
            self.relTSN = struct.unpack ("!I", buf[tmpoff:tmpoff+4])
            tmpoff = tmpoff + 4
        if self.flags & DeliveryPrerequisites.DP_FLAG_HAVE_UNRELSEQ:
            self.unrelTSN = struct.unpack ("!I", buf[tmpoff:tmpoff+4])
            tmpoff = tmpoff + 4
        if self.flags & DeliveryPrerequisites.DP_FLAG_HAVE_CONTROL:
            self.ctrlTSN = struct.unpack ("!I", buf[tmpoff:tmpoff+4])
            tmpoff = tmpoff + 4
        self.len = tmpoff

    def __str__(self):
        l = [ '%s=%r' % (k, getattr(self, k))
              for k in ('relTSN', 'unrelTSN', 'ctrlTSN')
              if k in self.__dict__ ]
        return "%s [ %s ]" % (self.__class__.__name__, ', '.join(l))

    def getLength(self):
        return self.len
    
class StateCookie(object):
    def __init__(self, buf, off):
        (self.tstamp, self.lspan, 
         self.val_A, self.val_Z, 
         self.rsTSN_A, self.usTSN_A, self.ctlTSN_A, self.ruFID_A, self.uuFID_A, 
         self.rsTSN_Z, self.usTSN_Z, self.ctlTSN_Z, self.ruFID_Z, self.uuFID_Z,
         self.port_A, self.port_Z) = struct.unpack ("!2Q12I2H", buf[off:off+68])

    def __repr__(self):
        return "[ tstamp=%d life=%d val_A=%d val_Z=%d rsTSN_A=%d usTSN_A=%d ctlTSN_A=%d ruFID_A=%d uuFID_A=%d rsTSN_Z=%d usTSN_Z=%d ctlTSN_Z=%d ruFID_Z=%d uuFID_Z=%d port_A=%d port_Z=%d ]" \
               % (self.tstamp, self.lspan, 
                  self.val_A, self.val_Z, 
                  self.rsTSN_A, self.usTSN_A, self.ctlTSN_A, self.ruFID_A, self.uuFID_A, 
                  self.rsTSN_Z, self.usTSN_Z, self.ctlTSN_Z, self.ruFID_Z, self.uuFID_Z,
                  self.port_A, self.port_Z)

class ACKInfoBlock(object):
    def __init__(self, buf, off):
        (self.flags, self.len) = struct.unpack ("!BH", buf[off:off+3])
        
    _flagNames = {
        0x0001: 'ACK_INFO_FLAG_CONTROL_FLOW',
        0x0002: 'ACK_INFO_FLAG_RELIABLE_SEQUENCED_FLOW',
        0x0004: 'ACK_INFO_FLAG_RELIABLE_UNSEQUENCED_FLOW',
        0x0010: 'ACK_INFO_FLAG_TYPE_RANGES',
        0x0020: 'ACK_INFO_FLAG_TYPE_SINGLES'
        }

    ACK_INFO_FLAG_CONTROL_FLOW = 0x0001
    ACK_INFO_FLAG_RELIABLE_SEQUENCED_FLOW = 0x0002
    ACK_INFO_FLAG_RELIABLE_UNSEQUENCED_FLOW = 0x0004
    ACK_INFO_FLAG_TYPE_RANGES = 0x0010
    ACK_INFO_FLAG_TYPE_SINGLES = 0x0020

    def getLength(self):
        return self.len
    
    def readACKInfoBlock(buf, off):
        (flags,) = struct.unpack ("!B", buf[off:off+1])
        if flags & ACKInfoBlock.ACK_INFO_FLAG_TYPE_RANGES:
            return ACKInfoBlockRanges(buf, off)
        else:
            return ACKInfoBlockSingles(buf, off)
    readACKInfoBlock = staticmethod(readACKInfoBlock)


class ACKInfoBlockSingles(ACKInfoBlock):
    def __init__(self, buf, off):
        ACKInfoBlock.__init__(self, buf, off)
        self.singles = []
        offset = off + 3
        len = 3
        while offset < off + self.len:
            (tsn,) = struct.unpack ("!I", buf[offset:offset+4])
            self.singles.append(tsn)
            offset = offset + 4

    def __str__(self):
        if len(self.singles) > 0:
            return "[ Singles: %s ]" % ','.join([ '%d' % i for i in self.singles ]) 
        return ''        


class ACKInfoBlockRanges(ACKInfoBlock):
    def __init__(self, buf, off):
        ACKInfoBlock.__init__(self, buf, off)
        self.ranges = []
        offset = off + 3
        len = 3
        while offset < off + self.len:
            r = struct.unpack ("!II", buf[offset:offset+8])
            self.ranges.append(r)
            offset = offset + 8

    def __repr__(self):
        if len(self.ranges) > 0:
            return "[ Ranges: %s ]" % ', '.join([ '%d,%d' % i for i in self.ranges ]) 
        return ''


class ACKInformation(object):
    def __init__(self, buf, off, len):
        self.len = len
        self.blocks = []
        offset = off
        while offset < off + len:
            aib = ACKInfoBlock.readACKInfoBlock(buf, offset)            
            self.blocks.append(aib)
            offset = offset + aib.getLength()

    def __repr__(self):
        if len(self.blocks) > 0:
            return ' '.join([ str(i) for i in self.blocks ])
        return ''        
    
    def getLength(self):
        return self.len


class Chunk(object):
    def __init__(self, buf, off):
        (self.chk_type, self.chk_len) = struct.unpack ("!HH", buf[off:off+4])

    def __str__(self):
        l = [ '%s=%r' % (k, getattr(self, k))
              for k in self.__dict__
              if k != "chk_type" and k != "chk_len" ]
        return "%s [ %s ]" % (self.__class__.__name__, ', '.join(l))

    def getLength(self):
        return self.chk_len


class UnknownChunk(Chunk):
    def __init__(self, buf, off):
        Chunk.__init__(self, buf, off)
        self.buf = buf
        self.off = off
        
    def __str__(self):
        return "%s [ type=%d len=%d %s ]" % (self.__class__.__name__, self.chk_type, self.chk_len, repr(self.buf[self.off+4:self.off+self.chk_len]))
       

class CancelledPacketsChunk(Chunk):
    def __init__(self, buf, off):
        Chunk.__init__(self, buf, off)
        self.ackInfo = ACKInformation(buf, off+4, self.chk_len-4)
        
class SackChunk(Chunk):
    def __init__(self, buf, off):
        Chunk.__init__(self, buf, off)        
        (self.relseqCumAck, self.ctrlCumAck, self.relunseqCumAck) = struct.unpack ("!3I", buf[off+4:off+16])
        self.ackInfo = ACKInformation(buf, off+16, self.chk_len-16)
        
class HeartbeatChunk(Chunk):
    def __init__(self, buf, off):
        Chunk.__init__(self, buf, off)
        (self.tstamp,) = struct.unpack ("!Q", buf[off+4:off+12])
        
class DataChunk(Chunk):
    def __init__(self, buf, off):
        Chunk.__init__(self, buf, off)
        (self.tag_id,) = struct.unpack ("!H", buf[off+4:off+6])
        self.data_len = self.getLength() - 6

class InitChunk(Chunk):
    def __init__(self, buf, off):
        Chunk.__init__(self, buf, off)
        (self.validation, self.relTSN, self.unrelTSN, self.ctrlTSN, self.relID) = struct.unpack ("!5I", buf[off+4:off+24])

class InitAckChunk(Chunk):
    def __init__(self, buf, off):
        Chunk.__init__(self, buf, off)
        (self.validation, self.relTSN, self.unrelTSN, self.ctrlTSN, self.relID) = struct.unpack ("!5I", buf[off+4:off+24])
        self.cookie = StateCookie(buf, off+24)

class CookieEchoChunk(Chunk):
    def __init__(self, buf, off):
        Chunk.__init__(self, buf, off)
        self.cookie = StateCookie(buf, off+4)

class CookieAckChunk(Chunk):
    def __init__(self, buf, off):
        Chunk.__init__(self, buf, off)
        (self.port,) = struct.unpack ("!H", buf[off+4:off+6])

class ShutdownChunk(Chunk):
    def __init__(self, buf, off):
        Chunk.__init__(self, buf, off)

class ShutdownAckChunk(Chunk):
    def __init__(self, buf, off):
        Chunk.__init__(self, buf, off)

class ShutdownCompleteChunk(Chunk):
    def __init__(self, buf, off):
        Chunk.__init__(self, buf, off)

class AbortChunk(Chunk):
    def __init__(self, buf, off):
        Chunk.__init__(self, buf, off)


class MessagePacket(dpkt.Packet):
    """Message Mocket Transport Protocol."""

    #__byte_order__ = '<'
    __hdr__ = (
        ('v_fl', 'H', (1 << 12 | 4)),
        ('win_len', 'H', 0),
        ('validation', 'I', 1),
        ('seq_num', 'I', 0)
        )
    HEADER_LENGTH = 12

    _flags = {
        0x001: 'REL', # reliable
        0x002: 'SEQ', # sequenced
#       0x004: 'MSG', # message-oriented
        0x008: 'CTL', # control
        0x010: 'DPR', # delivery prerequisites
        0x020: '1ST',
        0x040: 'LST',
        0x080: 'MFR'
        }
        
    _chunks = {
        # CHUNK_CLASS_METADATA
        (0x1000 | 0x0001): SackChunk,
        (0x1000 | 0x0002): HeartbeatChunk,
        (0x1000 | 0x0003): CancelledPacketsChunk,
        # CHUNK_CLASS_DATA
        (0x2000 | 0x0001): DataChunk,
        # CHUNK_CLASS_STATECHANGE
        (0x4000 | 0x0001): InitChunk,
        (0x4000 | 0x0002): InitAckChunk,
        (0x4000 | 0x0003): CookieEchoChunk,
        (0x4000 | 0x0004): CookieAckChunk,
        (0x4000 | 0x0005): ShutdownChunk,
        (0x4000 | 0x0006): ShutdownAckChunk,
        (0x4000 | 0x0007): ShutdownCompleteChunk,
        (0x4000 | 0x0008): AbortChunk
        }
        
    def unpack_chunks(self, buf):
        off = self.payload_offset
        while off < len(buf):
            (type,) = struct.unpack ("!H", buf[off:off+2])
            #print "type = 0x%04x" % type
            if MessagePacket._chunks.has_key(type):                
                decode = MessagePacket._chunks[type]
            else:
                decode = UnknownChunk
            chk = decode(buf, off)
            self.chunks.append (chk)
            off = off + chk.getLength()
        
    def unpack(self, buf):
        dpkt.Packet.unpack(self, buf)
        #print "\nv_fl=%x, win_len=%d, validation=%d, seq_num=%d" % (self.v_fl, self.win_len, self.validation, self.seq_num)
        self.len = len(buf)
        self.chunks = []
        self.version = (self.v_fl >> 12) & 0xF        
        #print "version=%d" % self.version
        self.flags = [ MessagePacket._flags[x] for x in MessagePacket._flags.keys() if (self.v_fl & 0xFFF) & x ]
        #print "flags=%s" % ', '.join(self.flags)
        self.payload_offset = MessagePacket.HEADER_LENGTH 
        if self.v_fl & 0x010: # if we have delivery prereq
            self.dp = DeliveryPrerequisites(buf, MessagePacket.HEADER_LENGTH)
            self.payload_offset = self.payload_offset + self.dp.getLength()
        #print "payload_offset=%s" % self.payload_offset
        self.unpack_chunks(buf)

    def __str__(self):
        ret = [ "len %d" % self.len ]        
        if (len(self.flags)):
            ret.append(' '.join(self.flags))
        ret.append("win %d" % self.win_len)
        ret.append("val %d" % self.validation)
        ret.append("seq %d" % self.seq_num)
        if (len(self.chunks)):
            ret.append(' '.join([ str(chk) for chk in self.chunks ]))
        return ", ".join(ret)
   
def usage():
    print >> sys.stderr, 'usage: %s [-i device] [pattern]' % sys.argv[0]
    sys.exit(1)

def endpoint2str(ip, port):
    return "%s:%d" % (".".join([ "%d" % ord(ip[i]) for i in range(4)]), port)

def main():
    opts, args = getopt.getopt(sys.argv[1:], 'i:h')
    name = None
    for o, a in opts:
        if o == '-i': name = a
        else: usage()
        
    pc = pcap.pcap(name)
    filter = 'udp'
    if len(args):
        filter = filter + ' and ' + ' '.join(args) 
    #print 'filter = ', filter
    pc.setfilter(filter)
    pc.setnonblock()
    decode = { pcap.DLT_LOOP:   dpkt.loopback.Loopback,
               pcap.DLT_NULL:   dpkt.loopback.Loopback,
               pcap.DLT_EN10MB: dpkt.ethernet.Ethernet }[pc.datalink()]
    try:
        print 'listening on %s: %s' % (pc.name, pc.filter)
        for ts, pkt in pc:
            ipPacket = decode(pkt).data
            udpPacket = ipPacket.data
            print "%.03f %s %s %s %s" % \
                  (ts, \
                   endpoint2str(ipPacket.src, udpPacket.sport), '>', \
                   endpoint2str(ipPacket.dst, udpPacket.dport), \
                   MessagePacket(udpPacket.data))
    except KeyboardInterrupt:
        nrecv, ndrop, nifdrop = pc.stats()
        print '\n%d packets received by filter' % nrecv
        print '%d packets dropped by kernel' % ndrop

if __name__ == '__main__':
    main()

# vim: et ts=4 sw=4

