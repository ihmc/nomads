/*
 * TcpIdleSender.java
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.kryomockets.util;

import us.ihmc.kryomockets.Connection;
import us.ihmc.kryomockets.Listener;

abstract public class TcpIdleSender extends Listener {
	boolean started;

	public void idle (Connection connection) {
		if (!started) {
			started = true;
			start();
		}
		Object object = next();
		if (object == null)
			connection.removeListener(this);
		else
			connection.sendMockets(object);
	}

	/** Called once, before the first send. Subclasses can override this method to send something so the receiving side expects
	 * subsequent objects. */
	protected void start () {
	}

	/** Returns the next object to send, or null if no more objects will be sent. */
	abstract protected Object next ();
}
