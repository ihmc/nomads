/**
 * Simple implementation of the semaphore synchronization primitive. 
 * Its counter can be initialized to any nonnegative value -- by default, it is zero.
 *
 * @author Mauro Tortonesi 
 */

package us.ihmc.mockets;

import java.util.logging.Logger;

public class Semaphore 
{
    
    public Semaphore() 
    {
        this (0, false);
    }
    
    public Semaphore(int i) 
    {
        this (i, false);
    }
    
    public Semaphore (int i, boolean log_debug_messages) 
    {
        if (i < 0) 
            throw new IllegalArgumentException (i + " < 0");
        
        _counter = i;
        _logger = null;
        
        if (log_debug_messages) {
            _logger = Logger.getLogger ("us.ihmc.mockets");
            _logger.info ("Semaphore: created new instance with counter = " + _counter);
        }
    }
    
    /**
     * Increments internal counter, possibly awakening a thread
     * wait()ing in acquire().
     */
    public synchronized void release() 
    {
        if (_logger != null) {
            _logger.info ("Semaphore: called release() with counter = " + _counter);
        }         
        
        if (_counter == 0) {
            this.notify();
        }
        ++_counter;
        
        if (_logger != null) {
            _logger.info ("Semaphore: exit from release() with counter = " + _counter);
        }         
    }
    
    /**
     * Decrements internal counter, blocking if the counter is already
     * zero. It throws an InterruptedException if we cannot acquire the lock
     * before the timeout expires.
     */
    public synchronized void acquire (long timeout) 
        throws InterruptedException 
    {
        if (_logger != null) {
            _logger.info ("Semaphore: called acquire(" + timeout + ") with counter = " + _counter);
        }         

        long start = System.currentTimeMillis();
        long tmout = timeout;
        long curr_time;
        
        do {
            this.wait (tmout);
            
            curr_time = System.currentTimeMillis();
            tmout = timeout - (curr_time - start);
        } while (_counter == 0 && curr_time < start + timeout);
        
        if (_counter == 0)
            throw new InterruptedException ("Couldn't acquire the lock in the given time interval. ");
        
        --_counter;
        
        if (_logger != null) {
            _logger.info ("Semaphore: returning from acquire(" + timeout + ") with counter = " + _counter);
        }         
    }
    
    /**
     * Decrements internal counter, blocking if the counter is already
     * zero.
     */
    public synchronized void acquire() 
        throws InterruptedException 
    {
        if (_logger != null) {
            _logger.info ("Semaphore: called acquire() with counter = " + _counter);
        }         

        do {
            this.wait();
        } while (_counter == 0);
        
        --_counter;
        
        if (_logger != null) {
            _logger.info ("Semaphore: returning from acquire() with counter = " + _counter);
        }         
    }
    
    private int _counter;
    private Logger _logger;
}
/*
 * vim: et ts=4 sw=4
 */

