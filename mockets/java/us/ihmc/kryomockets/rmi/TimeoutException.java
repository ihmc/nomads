/*
 * TimeoutException.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets.rmi;

/** Thrown when a method with a return value is invoked on a remote object and the response is not received with the
 * {@link RemoteObject#setResponseTimeout(int) response timeout}.
 * @see ObjectSpace#getRemoteObject(com.esotericsoftware.kryonet.Connection, int, Class...)
 * @author Nathan Sweet <misc@n4te.com> */
public class TimeoutException extends RuntimeException {
	public TimeoutException () {
		super();
	}

	public TimeoutException (String message, Throwable cause) {
		super(message, cause);
	}

	public TimeoutException (String message) {
		super(message);
	}

	public TimeoutException (Throwable cause) {
		super(cause);
	}
}
