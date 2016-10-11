/**
 * CircularQueue.java
 * 
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * ver
 * sion 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

package us.ihmc.ds;

import java.io.Serializable;

import java.util.Enumeration;

/**
 * Implementation of a Circular Queue.
 *
 * @author: Maggie Breedy
 * @version $Revision: 1.5 $
 *
 */
public class CircularQueue implements Serializable
{
    public CircularQueue (int size)
    {
        _buffer = new Object[size];
        _size = size;
    }

    // Return true is the queue is empty.
    public boolean isEmpty()
    {
        return (_numElements == 0);
    }

    // Return true if the queue is full.
    public boolean isFull()
    {
        if (_numElements >= _size) {
            return true;
        }
        else {
            return false;
        }
    }

    // Put an object array into a queue.
    public boolean enqueue (Object object)
    {
        if (object == null) {
            throw new NullPointerException ("cannot enqueue null object");
        }

        if (isFull()) {
            return false;
        }
        // Add the object to the buffer.
        _buffer[_tail] = object;

        // increment the tail pointer.
        _tail = (_tail + 1) % _buffer.length;

        _numElements++;

        return true;
    }

    // Get an object array from a queue.
    public Object dequeue()
    {
        if (isEmpty()) {
            return null;
        }

        // Assign current to the head position
        int current = _head;

        // increment the head pointer
        _head = (_head + 1) % _buffer.length;

        _numElements--;

        return _buffer [current];
    }

    // Get the element at the head position
    public Object peek()
    {
        if (isEmpty()) {
            return null;
        }

        // Return the current element at the head position
        return _buffer [_head];
    }

    // Returns an enumeration of the elements in the queue
    public Enumeration elements()
    {
        return new Enumerator (new CircularQueue (_buffer, _head, _tail, _size, _numElements));
    }

    /**
     * Sets the maximum number of elements that this queue will allow.
     *
     * If the new size is less than the current size, just create a larger
     * buffer, copy everything into the new one and replace with it the current
     * one.
     *
     * Otherwise, if the current buffer fits already the desired size, just
     * leave it as it is. We won't allow to enqueue more elements than the
     * desired size, but will hold the elements that were already in there.
     */
    public void setSize (int size)
    {
        if (_buffer.length < size) {
            Object[] newBuffer = new Object[size];

            if (_head <= _tail) {
                System.arraycopy (_buffer, 0, newBuffer, 0, _buffer.length);

                // if (_tail == 0) means that _tail was ready to write at the head of
                // the current buffer, but given that we are creating a buffer with
                // more space, we have to make it continue the "queue" after the
                // elements that are already there.
                if (_tail == 0) {
                    _tail += _buffer.length;
                }
            }
            else {
                // if (_tail < _head) means that there are some spaces available
                // in the middle of the buffer (exactly between _tail and _head).
                // Then, we will put the new buffer space in the middle of
                // _tail and _head. In other words, create a bigger buffer and
                // shift the contents of what was in _head through the end of
                // the buffer towards the end of the new buffer.

                // copy the elements that were from the begining of the buffer
                // through _tail in the same position into the new buffer.
                System.arraycopy (_buffer, 0, newBuffer, 0, _tail + 1);

                // then copy the elements that were from _head through the end
                // of _buffer at the end of the new buffer.
                int nbLength = newBuffer.length;
                int bLenght = _buffer.length;
                int numElemsAtEnd = bLenght - _head;
                int newHead = nbLength - numElemsAtEnd;

                System.arraycopy (_buffer, _head, newBuffer, newHead, numElemsAtEnd);

                // correct the value of _head
                _head = newHead;
            }

            _buffer = newBuffer;
        }

        _size = size;
    }

    /**
     * @return the number of elements in the queue.
     */
    public int size()
    {
        return _numElements;
    }

    //for debugging
    private void print()
    {
        Enumeration en = this.elements();
        System.out.print("\n  The elements of the queue are: \n    ");
        while (en.hasMoreElements()) {
            System.out.print("[" + en.nextElement() + "] ");
        }
        System.out.println("");
    }

    // Private constructor
    private CircularQueue (Object[] buffer, int head, int tail, int size, int numElements)
    {
        _buffer = buffer;
        _head = head;
        _tail = tail;
        _size = size;
        _numElements = numElements;
    }

    // Private class for an enumeration of elements in the queue.
    private class Enumerator implements Enumeration
    {
        Enumerator (CircularQueue cq)
        {
            _cq = cq;
        }

        public boolean hasMoreElements()
        {
            return (!_cq.isEmpty());
        }

        public Object nextElement()
        {
            return _cq.dequeue();
        }

        private CircularQueue _cq;
    }

    // Class Variables
    protected int _head = 0;
    protected int _tail = 0;
    protected Object[] _buffer;

    /**
     * Indicates the maximum number of elements that can be put in the queue.
     * It may not necessarily match _buffer.length .
     */
    protected int _size = 0;
    protected int _numElements = 0;


    // /////////////////////////////////////////////////////////////////////////
    // MAIN METHOD. for testing purposes only. Can be deleted. /////////////////
    // /////////////////////////////////////////////////////////////////////////
    public static void main(String[] args)
    {
        CircularQueue cq = new CircularQueue(3);

        System.out.println("cq.enqueueing \"A\": " + cq.enqueue("A"));
        System.out.println("cq.enqueueing \"B\": " + cq.enqueue("B"));
        System.out.println("cq.enqueueing \"C\": " + cq.enqueue("C"));
        System.out.println("cq.enqueueing \"D\": " + cq.enqueue("D") + ". [must be false]");
        cq.print();

        System.out.println("\ncq : setting the size to 6");
        cq.setSize(6);
        cq.print();

        System.out.println("cq.enqueueing \"E\": " + cq.enqueue("E"));
        System.out.println("cq.enqueueing \"F\": " + cq.enqueue("F"));
        System.out.println("cq.enqueueing \"G\": " + cq.enqueue("G"));
        System.out.println("cq.enqueueing \"H\": " + cq.enqueue("H") + ". [must be false]");
        cq.print();

        System.out.println("\ncq : setting the size to 4");
        cq.setSize(4);
        cq.print();

        System.out.println("\ncq :: dequeueing 3 elements...");
        for (int i = 0; i < 3; i++) {
            System.out.println("cq.dequeueing :: [" + cq.dequeue() + "]");
        }
        System.out.println("");

        cq.print();

        System.out.println("cq.enqueueing \"I\": " + cq.enqueue("I"));
        System.out.println("cq.enqueueing \"J\": " + cq.enqueue("J") + ". [must be false]");
        System.out.println("cq.enqueueing \"K\": " + cq.enqueue("K") + ". [must be false]");

        cq.print();

        System.out.println("\ncq : setting the size to 7");
        cq.setSize(7);
        cq.print();

        System.out.println("cq.enqueueing \"L\": " + cq.enqueue("L"));
        System.out.println("cq.enqueueing \"M\": " + cq.enqueue("M"));

        cq.print();

        System.out.println("cq.enqueueing \"N\": " + cq.enqueue("N"));
        System.out.println("cq.enqueueing \"O\": " + cq.enqueue("O") + ". [must be false]");
        cq.print();
    }
}