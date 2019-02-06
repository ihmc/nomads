/*
 * StateMachine.cpp
 *
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include "StateMachine.h"

bool StateMachine::receivedInit (void)
{
    if (_state != S_CLOSED) {
        return false;
    }
    // No state change for receiving an Init
    return true;
}

bool StateMachine::abort (void)
{
    _m.lock();
    _state = S_CLOSED;
    _m.unlock();
    return true;
}

bool StateMachine::applicationAbort (void)
{
    _m.lock();
    _state = S_APPLICATION_ABORT;
    _m.unlock();
    return true;
}

bool StateMachine::associate (void)
{
    _m.lock();
    if (_state != S_CLOSED) {
        _m.unlock();
        return false;
    }
    _state = S_COOKIE_WAIT;
    _m.unlock();
    return true;
}

bool StateMachine::receivedCookieEcho (void)
{
    _m.lock();
    if (_state != S_CLOSED) {
        _m.unlock();
        return false;
    }
    _state = S_ESTABLISHED;
    _m.unlock();
    return true;
}

bool StateMachine::receivedInitAck (void)
{
    _m.lock();
    if (_state != S_COOKIE_WAIT) {
        _m.unlock();
        return false;
    }
    _state = S_COOKIE_ECHOED;
    _m.unlock();
    return true;
}

bool StateMachine::receivedCookieAck (void)
{
    _m.lock();
    if (_state != S_COOKIE_ECHOED) {
        _m.unlock();
        return false;
    }
    _state = S_ESTABLISHED;
    _m.unlock();
    return true;
}

bool StateMachine::shutdown (void)
{
    _m.lock();
    if (_state != S_ESTABLISHED) {
        _m.unlock();
        return false;
    }
    _state = S_SHUTDOWN_PENDING;
    _m.unlock();
    return true;
}

bool StateMachine::receivedShutdown (void)
{
    _m.lock();
    if ((_state == S_ESTABLISHED) || (_state == S_SUSPEND_PENDING) ||
            (_state == S_SUSPEND_SENT) || (_state == S_RESUME_SENT)) {
        _state = S_SHUTDOWN_RECEIVED;
        _m.unlock();
        return true;
    }
    else if (_state == S_SHUTDOWN_SENT) {
        _state = S_SHUTDOWN_ACK_SENT;
        _m.unlock();
        return true;
    }
    _m.unlock();
    return false;
}

bool StateMachine::outstandingQueueFlushed (void)
{
    _m.lock();
    if (_state == S_SHUTDOWN_PENDING) {
        _state = S_SHUTDOWN_SENT;
        _m.unlock();
        return true;
    }
    else if (_state == S_SHUTDOWN_RECEIVED) {
        _state = S_SHUTDOWN_ACK_SENT;
        _m.unlock();
        return true;
    }
    _m.unlock();
    return false;
}

bool StateMachine::receivedShutdownAck (void)
{
    _m.lock();
    if ((_state != S_SHUTDOWN_SENT) && (_state != S_SHUTDOWN_ACK_SENT)) {
        _m.unlock();
        return false;
    }
    _state = S_CLOSED;
    _m.unlock();
    return true;
}

bool StateMachine::receivedShutdownComplete (void)
{
    _m.lock();
    if (_state != S_SHUTDOWN_ACK_SENT) {
        _m.unlock();
        return false;
    }
    _state = S_CLOSED;
    _m.unlock();
    return true;
}

bool StateMachine::suspend (void)
{
    _m.lock();
    if (_state == S_ESTABLISHED) {
        _state = S_SUSPEND_PENDING;
        _m.unlock();
        return true;
    }
    _m.unlock();
    return false;
}

bool StateMachine::queueFlushedOrTimeout (void)
{
    _m.lock();
    if (_state == S_SUSPEND_PENDING) {
        _state = S_SUSPEND_SENT;
        _m.unlock();
        return true;
    }
    _m.unlock();
    return false;
}

bool StateMachine::receivedSuspend (void)
{
    _m.lock();
    // Check for simultaneous suspension before calling this method
    if ((_state == S_ESTABLISHED) || (_state == S_SUSPEND_PENDING) || (_state == S_SUSPEND_RECEIVED) || (_state == S_SUSPEND_SENT)) {
        _state = S_SUSPEND_RECEIVED;
        _m.unlock();
        return true;
    }
    _m.unlock();
    return false;
}

bool StateMachine::receivedSuspendAck (void)
{
    _m.lock();
    if (_state != S_SUSPEND_SENT) {
        _m.unlock();
        return false;
    }
    _state = S_SUSPENDED;
    _m.unlock();
    return true;
}

bool StateMachine::resumeFromSuspension (void)
{
    _m.lock();
    if (_state == S_CLOSED) {
        _state = S_SUSPENDED;
        _m.unlock();
        return true;
    }
    _m.unlock();
    return false;
}

bool StateMachine::resume (void)
{
    _m.lock();
    if (_state == S_SUSPENDED) {
        _state = S_RESUME_SENT;
        _m.unlock();
        return true;
    }
    else if (_state == S_RESUME_SENT) {
        _m.unlock();
        return true;
    }
    _m.unlock();
    return false;
}

bool StateMachine::receivedResume (void)
{
    _m.lock();
    if (_state == S_ESTABLISHED) {
        _m.unlock();
        return true;
    }
    if (_state == S_SUSPEND_RECEIVED) {
        _state = S_ESTABLISHED;
        _m.unlock();
        return true;
    }
    _m.unlock();
    return false;
}

bool StateMachine::receivedResumeAck (void)
{
    _m.lock();
    if (_state == S_ESTABLISHED) {
        _m.unlock();
        return true;
    }
    if (_state == S_RESUME_SENT) {
        _state = S_ESTABLISHED;
        _m.unlock();
        return true;
    }
    _m.unlock();
    return false;
}

bool StateMachine::suspendTimeoutExpired (void)
{
    _m.lock();
    if (_state == S_SUSPEND_SENT) {
        _state = S_ESTABLISHED;
        _m.unlock();
        return true;
    }
    _m.unlock();
    return false;
}

bool StateMachine::simpleConnect (void)
{
    _m.lock();
    if (_state != S_CLOSED) {
        _m.unlock();
        return false;
    }
    _state = S_SIMPLE_CONNECT_ACK_WAIT;
    _m.unlock();
    return true;
}

bool StateMachine::receivedSimpleConnectAck (void)
{
    _m.lock();
    if (_state == S_ESTABLISHED) {
        _m.unlock();
        return true;
    }
    if (_state == S_SIMPLE_CONNECT_ACK_WAIT) {
        _state = S_ESTABLISHED;
        _m.unlock();
        return true;
    }
    _m.unlock();
    return false;
}

const char * StateMachine::getCurrentStateAsString (void)
{
    switch (_state) {
        case S_UNDEFINED:
            return "UNDEFINED";
        case S_CLOSED:
            return "CLOSED";
        case S_COOKIE_WAIT:
            return "COOKIE_WAIT";
        case S_COOKIE_ECHOED:
            return "COOKIE_ECHOED";
        case S_ESTABLISHED:
            return "ESTABLISHED";
        case S_SHUTDOWN_PENDING:
            return "SHUTDOWN_PENDING";
        case S_SHUTDOWN_SENT:
            return "SHUTDOWN_SENT";
        case S_SHUTDOWN_RECEIVED:
            return "SHUTDOWN_RECEIVED";
        case S_SHUTDOWN_ACK_SENT:
            return "SHUTDOWN_ACK_SENT";
        case S_SUSPEND_PENDING:
            return "SUSPEND_PENDING";
        case S_SUSPEND_SENT:
            return "SUSPEND_SENT";
        case S_SUSPENDED:
            return "SUSPENDED";
        case S_SUSPEND_RECEIVED:
            return "SUSPEND_RECEIVED";
        case S_RESUME_SENT:
            return "RESUME_SENT";
        default:
            return "UNKNOWN";
    }
}
