/*
 * KryoNetException.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets;

public class KryoNetException extends RuntimeException {
	public KryoNetException () {
		super();
	}

	public KryoNetException (String message, Throwable cause) {
		super(message, cause);
	}

	public KryoNetException (String message) {
		super(message);
	}

	public KryoNetException (Throwable cause) {
		super(cause);
	}
}
