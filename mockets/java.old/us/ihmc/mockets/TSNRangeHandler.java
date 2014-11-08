/**
 * The TSNRangeHandler class takes care of 
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.io.PrintStream;

import java.util.logging.Logger;

class TSNRangeHandler implements PacketOperationListener
{
    TSNRangeHandler (boolean control, boolean sequenced) 
    {
        // make sure sequenced is false if control is true
        assert !control || !sequenced;

        _logger = Logger.getLogger ("us.ihmc.mockets");

        _firstNode = null;
        _lastNode = _firstNode;
        
        _control = control;
        _sequenced = sequenced;

        _numOfRanges = 0;
    }

    private void insertBefore (Range r, Node nextNode)
    {
        assert nextNode != null : "nextNode must not be null.";        
        assert _numOfRanges >= 0;
        
        Node prevNode = nextNode._prev;
        Node n = new Node (prevNode, nextNode, r);
        nextNode._prev = n;
        if (prevNode == null) {
            _firstNode = n;
        } else {
            prevNode._next = n;
        }

        ++_numOfRanges;
    }
    
    private void insertBack (Range r) 
    {
        assert _numOfRanges >= 0;

        Node n = new Node (_lastNode, null, r);
        if (_lastNode != null) {
            _lastNode._next = n;
        } else {
            assert _firstNode == null;
            _firstNode = n;
        }
        _lastNode = n;
        
        ++_numOfRanges;
    }

    private void insertFront (Range r) 
    {
        assert _numOfRanges >= 0;
        
        Node n = new Node (null, _firstNode, r);
        if (_firstNode != null) {
            _firstNode._prev = n;
        } else {
            assert _lastNode == null;
            _lastNode = n;
        }
        _firstNode = n;

        ++_numOfRanges;
    }

    private void removeNode (Node n) 
    {
        assert n != null : "n must not be null.";
        assert _numOfRanges > 0;
        
        Node prevNode = n._prev;
        Node nextNode = n._next;

        if (prevNode == null) {
            _firstNode = nextNode;
        } else {
            prevNode._next = nextNode;
        }
        
        if (nextNode == null) {
            _lastNode = prevNode;
        } else {
            nextNode._prev = prevNode;
        }
        
        --_numOfRanges;
    }
    
    /* returns true if the packet was processed, false if it was simply 
     * discarded because already present */
    public synchronized boolean packetProcessed (long seqNum) 
    {
        if (_firstNode == null) {
            // the list is empty, we can append a new range at the back
            insertBack (new Range(seqNum));
            return true;
        } else if (SequentialArithmetic.add(seqNum, 1) == _firstNode._data._begin) {
            // this sequence number immediately preceeds the TSN range of the first node
            --_firstNode._data._begin;
            return true;
        } else if (seqNum == SequentialArithmetic.add(_lastNode._data._end, 1)) {
            // this sequence number immediately follows the TSN range of the last node
            ++_lastNode._data._end;
            return true;
        } else if (SequentialArithmetic.lessThan(seqNum, _firstNode._data._begin)) {
            // this sequence number preceeds the first range node, so we have to create 
            // a new range and insert it at the beginning of the list
            insertFront (new Range(seqNum));
            return true;
        } else if (SequentialArithmetic.greaterThan(seqNum, _lastNode._data._end)) {
            // this sequence number follows the last range node, so we have to create 
            // a new range and insert it at the end of the list
            insertBack (new Range(seqNum));
            return true;
        } else {
            // find the right spot for this sequence number
            Node currNode = _firstNode;
            Node nextNode = _firstNode._next;
            while (currNode != null &&
                   SequentialArithmetic.lessThan(seqNum, currNode._data._end)) {
                boolean added = false;
                if (seqNum == SequentialArithmetic.add(currNode._data._end, 1)) {
                    // the sequence number immediately follows that of the current range node
                    ++currNode._data._end;
                    added = true;
                } else if (nextNode != null) {
                    if (SequentialArithmetic.add(seqNum, 1) == nextNode._data._begin) {
                        // the sequence number immediately preceeds the next range node
                        --nextNode._data._begin;
                        added = true;
                    } else if (SequentialArithmetic.greaterThan (seqNum, currNode._data._end) &&
                               SequentialArithmetic.lessThan (seqNum, nextNode._data._begin)) {
                        // the sequence number belongs to a new range that is in between the current and next range nodes
                        insertBefore (new Range(seqNum), nextNode);
                        return true;
                    }
                }
                
                // exit from the loop when the new TSN has been inserted in the range list
                if (added) {
                    // check to see if we need to do a merge
                    if (nextNode != null &&
                        SequentialArithmetic.add(currNode._data._end, 1) == nextNode._data._begin) {
                        // merge current and next range nodes
                        currNode._data._end = nextNode._data._end;
                        // we can safely use removeNode here
                        removeNode (nextNode);
                    }
                    return true;
                }
                
                // update currNode and nextNode for next loop cicle
                currNode = nextNode;
                if (nextNode != null) {
                    nextNode = nextNode._next;
                }
            }
        }
        
        return false;
    }
    
    synchronized void fillACKInformation (ACKInformation ai) 
    {
        int singlesCount = 0;
        int rangesCount = 0;
        
        if (_numOfRanges > 0) {
            ACKInfoBlockRanges ranges = new ACKInfoBlockRanges (_control, _sequenced);
            ACKInfoBlockSingles singles = new ACKInfoBlockSingles (_control, _sequenced);
            
            for (Node n = _firstNode; n != null; n = n._next) {
                Range r = n._data;
                if (r._begin == r._end) {
                    //_logger.info ("Adding single " + r._begin);
                    singles.addSingle (r._begin);
                    ++singlesCount;
                } else {
                    //_logger.info ("Adding range (" + r._begin + "," + r._end + ")");
                    ACKInfoRange air = new ACKInfoRange (r._begin, r._end);
                    ranges.addRange (air);
                    ++rangesCount;
                }
            }

            if (singlesCount > 0) {                
                assert singles != null;
                ai.addACKInfoBlock (singles);
            }
            if (rangesCount > 0) {
                assert ranges != null;
                ai.addACKInfoBlock (ranges);
            }
        }
    }

    void _dump (PrintStream os)
    {
        os.println ("_numOfRanges: " + _numOfRanges);
        for (Node ptr = _firstNode; ptr != null; ptr = ptr._next)
            ptr._data._dump (os);
    }

    Range peek()
    {
        if (_firstNode == null)
            return null;

        return _firstNode._data;
    }

    void removeFirstRange()
    {
        if (_firstNode != null) {
            removeNode (_firstNode);
        }
    }


    class Range
    {
        Range (long sequenceNumber) 
        {
            if (sequenceNumber < 0)
                throw new IllegalArgumentException ("Ranges endpoints must be non negative.");
            
            _begin = sequenceNumber;
            _end = sequenceNumber;
        }

        void _dump (PrintStream os) 
        {
            os.println ("range (" + _begin + "," + _end + ")");
        }

        long begin()
        {
            return _begin;
        }
        
        long end()
        {
            return _end;
        }
        
        private long _begin;
        private long _end;
    }

    private class Node
    {
        Node (Range data) 
        {
            _prev = null;
            _next = null;
            _data = data;
        }

        Node (Node prev, Node next, Range data) 
        {
            _prev = prev;
            _next = next;
            _data = data;
        }
        
        Node _next;
        Node _prev;
        Range _data;
    }

    private Logger _logger;
    private Node _firstNode;
    private Node _lastNode;
    private int _numOfRanges;
    private boolean _control;
    private boolean _sequenced;
}
/*
 * vim: et ts=4 sw=4
 */

