/*
 * Mocket.java
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
 *
 * The main class for a client application to use the Mockets communication library.
 * Similar in functionality to a socket. Used by a client to establish a connection
 * to a server and then communicate with the server.
 */

package us.ihmc.mockets;

import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.util.Enumeration;
import java.util.Vector;

public class Mocket
{
    /**
     * Creates a new, unconnected Mocket endpoint.
     */
    public Mocket()
        throws IOException
    {
        init (null);
    }

    /**
     * Creates a new, unconnected Mocket endpoint and uses
     * the specified file to load the configuration for the Mocket.
     *
     * @param configFile the path to the configuration file that should be loaded
     */
    public Mocket (String configFile)
        throws IOException
    {
        init (configFile);
    }

    /**
     * Creates a new, unconnected Mocket DTLS endpoint and uses
     * the specified file to load the configuration for the Mocket.
     *
     * @param configFile the path to the configuration file that should be loaded
     * @param pathToCertificate the path to the certificate file that should be loaded
     * @param pathToPrivateKey the path to the Private Key file that should be loaded
     */
    public Mocket (String configFile, String pathToCertificate, String pathToPrivateKey)
            throws IOException
    {
        initDtls (configFile, pathToCertificate, pathToPrivateKey);
    }

    /**
     * The class used to send messages over Mockets.
     * Obtained by calling <code>getSender</code> on an instance of Mocket.
     * Note: messages may also be sent using the Mocket class directly, this class makes it
     * more convenient by having default or configurable values for some of the parameters.
     * 
     */
    public class Sender
    {
        /**
         * Basic send function. Given a byte array to be sent this function enqueues the data for
         * transmission using the type of flow defined in the constructor of class <code>Sender</code>.
         * <p>
         * Several communication parameter are not specified, the default value is going to be used
         * for the following parameters:
         *  tag = 0,
         *  priority = 5,
         *  enqueue timeout = 0 or a value set using the method <code>setDefaultEnqueueTimeout</code>,
         *  retry timeout = 0 or a value set using the method <code>setDefaultRetryTimeout</code>.
         *  
         * @param buffer    the buffer to be sent. The buffer is going to be sent entirely.
         * @return          <ul class="return">
         *                  <li><code>0</code> if the message is successfully enqueued to be sent.</li>
         *                  <li><code>-1</code> if the connection is not in the state ESTABLISHED,
         *                  it is not established yet or one of the peers called <code>close</code>;</li>
         *                  <li><code>-2</code> if adding the data chunk to a packet fails;</li>
         *                  <li><code>-3</code> if inserting into <code>pendingPacketQueue</code> fails;</li>
         *                  <li><code>-10</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                  <li><code>-11</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
         *                  </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Mocket#getSender(boolean, boolean) 
         */
        public int send (byte[] buffer)
            throws IOException, IllegalArgumentException
        {
           return sendNative (buffer, 0, buffer.length);
        }

        /**
         * Basic send function. Given a byte array this function extracts <code>length</code> bytes
         * starting at <code>offset</code> and enqueues the data for transmission. The delivery will
         * use the type of flow defined in the constructor of class <code>Sender</code>.
         * <p>
         * Several communication parameter are not specified, the default value is going to be used
         * for the following parameters:
         *  tag = 0,
         *  priority = 5,
         *  enqueue timeout = 0 or a value set using the method <code>setDefaultEnqueueTimeout</code>,
         *  retry timeout = 0 or a value set using the method <code>setDefaultRetryTimeout</code>.
         * 
         * @param buffer    the buffer to be sent.
         * @param offset    the position in the buffer we start to send from.
         *                  E.g. offset=10, we send buffer starting at the 10th byte in the array
         * @param length    the length for which we send.
         *                  E.g. length=50 we send 50 bytes of the buffer, starting at offset.
         *                  If offset=10, length=50 we send bytes from position 10 in the
         *                  buffer through position 60.
         * @return          <ul class="return">
         *                  <li><code>0</code> if the message is successfully enqueued to be sent.</li>
         *                  <li><code>-1</code> if the connection is not in the state ESTABLISHED,
         *                  it is not established yet or one of the peers called <code>close</code>;</li>
         *                  <li><code>-2</code> if adding the data chunk to a packet fails;</li>
         *                  <li><code>-3</code> if inserting into <code>pendingPacketQueue</code> fails;</li>
         *                  <li><code>-10</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                  <li><code>-11</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
         *                  </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @throws java.lang.IndexOutOfBoundsException  if the parameters <code>offset</code> or 
         *                                              <code>length</code> are illegal for <code>buffer</code>.
         * @see Mocket#getSender(boolean, boolean) 
         */
        public int send (byte []buffer, int offset, int length)
            throws IOException,IllegalArgumentException,IndexOutOfBoundsException
        {
            if (offset < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset + length) > buffer.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            return sendNative (buffer, offset, length);
        }

        /**
         * Enqueues data for transmission using the specified parameters.
         * This send function enqueues the data for transmission and allows to specify a tag and a priority
         * for the data to be sent. Data will be delivered using the type of flow defined in the constructor
         * of class <code>Sender</code>.
         * <p>
         * Several communication parameter are not specified, the default value is going to be used
         * for the following parameters:
         *  enqueue timeout = 0 or a value set using the method <code>setDefaultEnqueueTimeout</code>,
         *  retry timeout = 0 or a value set using the method <code>setDefaultRetryTimeout</code>.
         *  
         * @param buffer    the buffer to be sent. The buffer is going to be sent entirely.
         * @param iTag      integer value that can be used to mark a flow of messages belonging to
         *                  the same type. The value has to be >0.
         * @param sPriority used to assign a priority different than the default one (higher or lower).
         *                  The value has to be >0. Note that the priority of a message will grow every
         *                  time it is skipped in favor of a higher priority messages that gets sent first.
         *                  This mechanism is implemented to avoid message starvation. The range of priority values is 0-255.
         * @return          <ul class="return">
         *                  <li><code>0</code> if the message is successfully enqueued to be sent;</li>
         *                  <li><code>-1</code> if the connection is not in the state ESTABLISHED,
         *                  it is not established yet or one of the peers called <code>close</code>;</li>
         *                  <li><code>-2</code> if adding the data chunk to a packet fails;</li>
         *                  <li><code>-3</code> if inserting into <code>pendingPacketQueue</code> fails;</li>
         *                  <li><code>-10</code> if the parameters <code>iTag</code> or <code>sPriority</code>
         *                  are illegal (less then zero);</li>
         *                  <li><code>-11</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                  <li><code>-12</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
         *                  </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see "<code>replace</code> and <code>cancel</code> to take advantage of <code>iTag</code>."
         * @see Mocket#getSender(boolean, boolean) 
         */
        public int send (byte[] buffer, int iTag, short sPriority)
            throws IOException,IllegalArgumentException
        {
            return sendTagPrio(buffer, 0, buffer.length, iTag, sPriority);
        }

        /**
         * Enqueues data for transmission using the specified parameters.
         * Given a byte array this send function extracts <code>length</code> bytes starting at 
         * <code>offset</code> and enqueues the data for transmission. This send function allows to
         * specify a tag and a priority for the data to be sent.
         * Data will be delivered using the type of flow defined in the constructor of class <code>Sender</code>.
         * <p>
         * Several communication parameter are not specified, the default value is going to be used
         * for the following parameters:
         *  enqueue timeout = 0 or a value set using the method <code>setDefaultEnqueueTimeout</code>,
         *  retry timeout = 0 or a value set using the method <code>setDefaultRetryTimeout</code>.
         *  
         * @param buffer    the buffer to be sent.
         * @param offset    the position in the buffer we start to send from.
         *                  E.g. offset=10, we send buffer starting at the 10th byte in the array
         * @param length    the length for which we send.
         *                  E.g. length=50 we send 50 bytes of the buffer, starting at offset.
         *                  If offset=10, length=50 we send bytes from position 10 in the
         *                  buffer through position 60.
         * @param iTag      integer value that can be used to mark a flow of messages belonging to
         *                  the same type. The value has to be >0.
         * @param sPriority used to assign a priority different than the default one (higher or lower).
         *                  The value has to be >0. Note that the priority of a message will grow every
         *                  time it is skipped in favor of a higher priority messages that gets sent first.
         *                  This mechanism is implemented to avoid message starvation. The range of priority values is 0-255.
         * @return          <ul class="return">
         *                  <li><code>0</code> if the message is successfully enqueued to be sent;</li>
         *                  <li><code>-1</code> if the connection is not in the state ESTABLISHED,
         *                  it is not established yet or one of the peers called <code>close</code>;</li>
         *                  <li><code>-2</code> if adding the data chunk to a packet fails;</li>
         *                  <li><code>-3</code> if inserting into <code>pendingPacketQueue</code> fails;</li>
         *                  <li><code>-10</code> if the parameters <code>iTag</code> or <code>sPriority</code>
         *                  are illegal (less then zero);</li>
         *                  <li><code>-11</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                  <li><code>-12</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
         *                  </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @throws java.lang.IndexOutOfBoundsException  if the parameters <code>offset</code> or 
         *                                              <code>length</code> are illegal for <code>buffer</code>.
         * @see "<code>replace</code> and <code>cancel</code> to take advantage of <code>iTag</code>."
         * @see Mocket#getSender(boolean, boolean) 
         */
        public int send (byte []buffer, int offset, int length, int iTag, short sPriority)
            throws IOException,IllegalArgumentException,IndexOutOfBoundsException
        {
            if (offset < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset + length) > buffer.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            return sendTagPrio(buffer, offset, length, iTag, sPriority);
        }

        /**
         * Enqueues data for transmission using the specified parameters.
         * This send function enqueues the byte array <code>buffer</code> for transmission, and allows to specify
         * a parameter of type <code>Param</code> that will contain information about tag, priority, enqueueTimeout
         * and retryTimeout.
         * 
         * @param buffer    the buffer to be sent. The buffer is going to be sent entirely.
         * @param pParams   an instance of class <code>Param</code> that contains: tag, priority, enqueueTimeout, retryTimeout. 
         * @return          <ul class="return">
         *                  <li><code>0</code> if the message is successfully enqueued to be sent;</li>
         *                  <li><code>-1</code> if the connection is not in the state ESTABLISHED,
         *                  it is not established yet or one of the peers called <code>close</code>;</li>
         *                  <li><code>-2</code> if add the data chunk to a packet failed;</li>
         *                  <li><code>-3</code> if insert into <code>pendingPacketQueue</code> failed;</li>
         *                  <li><code>-10</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                  <li><code>-11</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null;</li>
         *                  <li><code>-12</code> if the parameter <code>pParams</code> is invalid, <code>pParams</code> is null.</li>
         *                  </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Params
         * @see Mocket#getSender(boolean, boolean) 
         */
        public int send (byte[] buffer, Params pParams)
            throws IOException,IllegalArgumentException
        {
            return sendParams(buffer, 0, buffer.length, pParams);
        }

        /**
         * Enqueues data for transmission using the specified parameters.
         * Given a byte array this send function extracts <code>length</code> bytes starting at <code>offset</code>
         * and enqueues the data for transmission. This send function allows to specify a parameter of type
         * <code>Param</code> that will contain information about tag, priority, enqueueTimeout and retryTimeout.
         * 
         * @param buffer    the buffer to be sent.
         * @param offset    the position in the buffer we start to send from.
         *                  E.g. offset=10, we send buffer starting at the 10th byte in the array
         * @param length    the length for which we send.
         *                  E.g. length=50 we send 50 bytes of the buffer, starting at offset.
         *                  If offset=10, length=50 we send bytes from position 10 in the
         *                  buffer through position 60.
         * @param pParams   an instance of class <code>Param</code> that contains: tag, priority, enqueueTimeout, retryTimeout.
         * @return          <ul class="return">
         *                  <li><code>0</code> if the message is successfully enqueued to be sent;</li>
         *                  <li><code>-1</code> if the connection is not in the state ESTABLISHED,
         *                  it is not established yet or one of the peers called <code>close</code>;</li>
         *                  <li><code>-2</code> if add the data chunk to a packet failed;</li>
         *                  <li><code>-3</code> if insert into <code>pendingPacketQueue</code> failed;</li>
         *                  <li><code>-10</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                  <li><code>-11</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null;</li>
         *                  <li><code>-12</code> if the parameter <code>pParams</code> is invalid, <code>pParams</code> is null.</li>
         *                  </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @throws java.lang.IndexOutOfBoundsException  if the parameters <code>offset</code> or 
         *                                              <code>length</code> are illegal for <code>buffer</code>.
         * @see Params
         * @see Mocket#getSender(boolean, boolean) 
         */
         public int send (byte []buffer, int offset, int length, Params pParams)
            throws IOException,IllegalArgumentException,IndexOutOfBoundsException
        {
            if (offset < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset + length) > buffer.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            return sendParams(buffer, offset, length, pParams);
        }

        /**
         * Enqueues data for transmission using the specified parameters.
         * This send function enqueues the byte array <code>buffer</code> for transmission allowing to specify
         * a tag, a priority, the enqueue timeout and the retry timeout for the data to be sent.
         * Given a byte array to be sent this function delivers the data using the type of flow defined in the
         * constructor of class <code>Sender</code>.
         * 
         * @param buffer            the buffer to be sent. The buffer is going to be sent entirely.
         * @param iTag              integer value that can be used to mark a flow of messages belonging to
         *                          the same type. The value has to be >0.
         * @param sPriority         used to assign a priority different than the default one (higher or lower).
         *                          The value has to be >0. Note that the priority of a message will grow every
         *                          time it is skipped in favor of a higher priority messages that gets sent first.
         *                          This mechanism is implemented to avoid message starvation. The range of priority
         *                          values is 0-255.
         * @param lEnqueueTimeout   indicates the length of time in milliseconds for which the method will wait
         *                          if there is no room in the outgoing buffer. A zero value indicates wait forever.
         * @param lRetryTimeout     indicates the length of time for which the transmitter will retransmit the packet
         *                          to ensure successful delivery. A zero value indicates retry with no time limit.
         *                          Note that this parameter makes sense only if the flow is reliable, otherwise the
         *                          behavior is to transmit and forget about the packet.
         * @return                  <ul class="return">
         *                          <li><code>0</code> if the message is successfully enqueued to be sent;</li>
         *                          <li><code>-1</code> if the connection is not in the state ESTABLISHED,
         *                          it is not established yet or one of the peers called <code>close</code>;</li>
         *                          <li><code>-2</code> if add the data chunk to a packet failed;</li>
         *                          <li><code>-3</code> if insert into <code>pendingPacketQueue</code> failed;</li>
         *                          <li><code>-10</code> if the parameters are illegal (less then zero);</li>
         *                          <li><code>-11</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                          <li><code>-12</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
         *                          </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Sender#setDefaultEnqueueTimeout(long) 
         * @see Sender#setDefaultRetryTimeout(long) 
         * @see "<code>replace</code> and <code>cancel</code> to take advantage of <code>iTag</code>."
         * @see Mocket#getSender(boolean, boolean) 
         */
        public int send (byte[] buffer, int iTag, short sPriority, long lEnqueueTimeout, long lRetryTimeout)
            throws IOException,IllegalArgumentException
        {
            return sendTagPrioEnqueRetr (buffer, 0, buffer.length, iTag, sPriority, lEnqueueTimeout, lRetryTimeout);
        }

        /**
         * Enqueues data for transmission using the specified parameters.
         * Given a byte array this send function extracts <code>length</code> bytes starting at 
         * <code>offset</code> and enqueues the data for transmission. This send function also allows to specify
         * a tag, a priority, the enqueue timeout and the retry timeout for the data to be sent.
         * The data to be sent will be delivered using the type of flow defined in the constructor of class <code>Sender</code>.
         * 
         * @param buffer            the buffer to be sent.
         * @param offset            the position in the buffer we start to send from.
         *                          E.g. offset=10, we send buffer starting at the 10th byte in the array
         * @param length            the length for which we send.
         *                          E.g. length=50 we send 50 bytes of the buffer, starting at offset.
         *                          If offset=10, length=50 we send bytes from position 10 in the
         *                          buffer through position 60.
         * @param iTag              integer value that can be used to mark a flow of messages belonging to
         *                          the same type. The value has to be >0.
         * @param Priority          used to assign a priority different than the default one (higher or lower).
         *                          The value has to be >0. Note that the priority of a message will grow every
         *                          time it is skipped in favor of a higher priority messages that gets sent first.
         *                          This mechanism is implemented to avoid message starvation. The range of priority
         *                          values is 0-255.
         * @param EnqueueTimeout    indicates the length of time in milliseconds for which the method will wait
         *                          if there is no room in the outgoing buffer. A zero value indicates wait forever.
         * @param RetryTimeout      indicates the length of time for which the transmitter will retransmit the packet
         *                          to ensure successful delivery. A zero value indicates retry with no time limit.
         *                          Note that this parameter makes sense only if the flow is reliable, otherwise the
         *                          behavior is to transmit and forget about the packet.
         * @return                  <ul class="return">
         *                          <li><code>0</code> if the message is successfully enqueued to be sent;</li>
         *                          <li><code>-1</code> if the connection is not in the state ESTABLISHED,
         *                          it is not established yet or one of the peers called <code>close</code>;</li>
         *                          <li><code>-2</code> if add the data chunk to a packet failed;</li>
         *                          <li><code>-3</code> if insert into <code>pendingPacketQueue</code> failed;</li>
         *                          <li><code>-10</code> if the parameters are illegal (less then zero);</li>
         *                          <li><code>-11</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                          <li><code>-12</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
         *                          </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @throws java.lang.IndexOutOfBoundsException  if the parameters <code>offset</code> or 
         *                                              <code>length</code> are illegal for <code>buffer</code>.
         * @see Sender#setDefaultEnqueueTimeout(long) 
         * @see Sender#setDefaultRetryTimeout(long) 
         * @see "<code>replace</code> and <code>cancel</code> to take advantage of <code>iTag</code>."
         * @see Mocket#getSender(boolean, boolean) 
         */
        public int send (byte []buffer, int offset, int length, int iTag, short Priority, long EnqueueTimeout, long RetryTimeout)
            throws IOException,IllegalArgumentException,IndexOutOfBoundsException
        {
            if (offset < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset + length) > buffer.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            return sendTagPrioEnqueRetr (buffer, offset, length, iTag, Priority, EnqueueTimeout, RetryTimeout);
        }

        public int gsend (byte[] buf1, byte[] buf2)
            throws IOException,IllegalArgumentException
        {
            return gsend2Args(buf1, 0, buf1.length, buf2, 0, buf2.length);
        }

        public int gsend (byte[] buf1, int offset1, int length1, byte[] buf2, int offset2, int length2)
            throws IOException,IllegalArgumentException,IndexOutOfBoundsException
        {
            if (offset1 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length1 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset1 + length1) > buf1.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (offset2 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length2 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset2 + length2) > buf2.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            return gsend2Args(buf1, offset1, length1, buf2, offset2, length2);
        }

        public int gsend (byte[] buf1, byte[] buf2, byte[] buf3)
            throws IOException,IllegalArgumentException
        {
            return gsend3Args(buf1, 0, buf1.length, buf2, 0, buf2.length, buf3, 0, buf3.length);
        }

        public int gsend (byte[] buf1, int offset1, int length1, byte[] buf2, int offset2, int length2, byte[] buf3, int offset3, int length3)
            throws IOException,IllegalArgumentException,IndexOutOfBoundsException
        {
            if (offset1 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length1 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset1 + length1) > buf1.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (offset2 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length2 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset2 + length2) > buf2.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
             if (offset3 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length3 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset3 + length3) > buf3.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            return gsend3Args(buf1, offset1, length1, buf2, offset2, length2, buf3, offset3, length3);
        }

        public int gsend (byte[] buf1, byte[] buf2, byte[] buf3, byte[] buf4)
            throws IOException,IllegalArgumentException
        {
            return gsend4Args(buf1, 0, buf1.length, buf2, 0, buf2.length, buf3, 0, buf3.length, buf4, 0, buf4.length);
        }

        public int gsend (byte[] buf1, int offset1, int length1, byte[] buf2, int offset2, int length2, byte[] buf3, int offset3,
                int length3, byte[] buf4, int offset4, int length4)
            throws IOException,IllegalArgumentException,IndexOutOfBoundsException
        {
            if (offset1 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length1 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset1 + length1) > buf1.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (offset2 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length2 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset2 + length2) > buf2.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
             if (offset3 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length3 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset3 + length3) > buf3.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
             if (offset4 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length4 < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset4 + length4) > buf4.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            return gsend4Args(buf1, offset1, length1, buf2, offset2, length2, buf3, offset3, length3, buf4, offset4, length4);
        }

        /**
         * First cancels any previously enqueued messages that have been tagged with the specified <code>OldTag</code>
         * value and then transmits the new message using the specified parameters. Note that there may be no old
         * messages to cancel, in which case this call behaves just like a <code>send</code>.
         * <p>
         * Several communication parameter are not specified, the default value is going to be used
         * for the following parameters:
         *  priority = 5,
         *  enqueue timeout = 0 or a value set using the method <code>setDefaultEnqueueTimeout</code>,
         *  retry timeout = 0 or a value set using the method <code>setDefaultRetryTimeout</code>.
         * 
         * @param buffer    the buffer to be sent. The buffer is going to be sent entirely.
         * @param oldTag    the tag of the messages to be replaced.
         * @param newTag    tag of the new message to be enqueued. The values has to be >0.
         * @return          <ul class="return">
         *                  <li><code>0</code> if the message is successfully enqueued to be sent and any messages tagged
         *                  with <code>oldTag</code> is successfully removed from the queues.</li>
         *                  <li><code>-1</code> if the retrieve of the instance of <code>Transmitter</code> did not work,
         *                  the value is null.</li>
         *                  <li><code>-2</code> if the cancellation of messages marked with <code>oldTag</code> did not work.</li>
         *                  <li><code>-3</code>  if the send operation on the new message did not work.</li>
         *                  <li><code>-10</code> if the parameters are illegal (less then zero);</li>
         *                  <li><code>-11</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                  <li><code>-12</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
         *                  </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Sender#cancel(boolean, boolean, int) 
         * @see Sender#send(boolean, boolean, byte[], int, int, int, short, long, long) 
         * @see Mocket#getSender(boolean, boolean) 
         */
        public int replace (byte[] buffer, int oldTag, int newTag)
            throws IOException,IllegalArgumentException
        {
            return replaceNative (buffer, 0, buffer.length, oldTag, newTag);
        }

        /**
         * First cancels any previously enqueued messages that have been tagged with the specified <code>OldTag</code>
         * value and then transmits the new message using the specified parameters. Note that there may be no old
         * messages to cancel, in which case this call behaves just like a <code>send</code>.
         * <p>
         * Several communication parameter are not specified, the default value is going to be used
         * for the following parameters:
         *  priority = 5,
         *  enqueue timeout = 0 or a value set using the method <code>setDefaultEnqueueTimeout</code>,
         *  retry timeout = 0 or a value set using the method <code>setDefaultRetryTimeout</code>.
         * 
         * @param buffer    the buffer to be sent.
         * @param offset    the position in the buffer we start to send from.
         *                  E.g. offset=10, we send buffer starting at the 10th byte in the array
         * @param length    the length for which we send.
         *                  E.g. length=50 we send 50 bytes of the buffer, starting at offset.
         *                  If offset=10, length=50 we send bytes from position 10 in the
         *                  buffer through position 60.
         * @param oldTag    tag of the messages to replace. Messages tagged with <code>oldTag</code> will be canceled.
         * @param newTag    tag of the new message to be enqueued. The value has to be >0.
         * @return          <ul class="return">
         *                  <li><code>0</code> if the message is successfully enqueued to be sent and any messages tagged
         *                  with <code>oldTag</code> is successfully removed from the queues;</li>
         *                  <li><code>-1</code> if the retrieve of the instance of <code>Transmitter</code> did not work,
         *                  the value is null;</li>
         *                  <li><code>-2</code> if the cancellation of messages marked with <code>oldTag</code> did not work;</li>
         *                  <li><code>-3</code>  if the send operation on the new message did not work;</li>
         *                  <li><code>-10</code> if the parameters are illegal (less then zero);</li>
         *                  <li><code>-11</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                  <li><code>-12</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
         *                  </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @throws java.lang.IndexOutOfBoundsException  if the parameters <code>offset</code> or 
         *                                              <code>length</code> are illegal for <code>buffer</code>.
         * @see Sender#cancel(boolean, boolean, int) 
         * @see Sender#send(boolean, boolean, byte[], int, int, int, short, long, long) 
         * @see Mocket#getSender(boolean, boolean) 
         */
        public int replace (byte []buffer, int offset, int length, int oldTag, int newTag)
            throws IOException,IllegalArgumentException,IndexOutOfBoundsException
        {
            if (offset < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset + length) > buffer.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            return replaceNative (buffer, offset, length, oldTag, newTag);
        }

        /**
         * First cancels any previously enqueued messages that have been tagged with the specified <code>OldTag</code>
         * value and then transmits the new message using the specified parameters. Note that there may be no old
         * messages to cancel, in which case this call behaves just like a <code>send</code>.
         * 
         * @param buffer    the buffer to be sent. The buffer is going to be sent entirely.
         * @param oldTag    the tag of the messages to be replaced.
         * @param params    an instance of class <code>Param</code> that contains: tag, priority, enqueueTimeout, retryTimeout.
         * @return          <ul class="return">
         *                  <li><code>0</code> if the message is successfully enqueued to be sent and any messages tagged
         *                  with <code>oldTag</code> is successfully removed from the queues;</li>
         *                  <li><code>-1</code> if the retrieve of the instance of <code>Transmitter</code> did not work,
         *                  the value is null;</li>
         *                  <li><code>-2</code> if the cancellation of messages marked with <code>oldTag</code> did not work;</li>
         *                  <li><code>-3</code>  if the send operation on the new message did not work;</li>
         *                  <li><code>-10</code> if the parameter <code>oldTag</code> is illegal (less then zero);</li>
         *                  <li><code>-11</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                  <li><code>-12</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null;</li>
         *                  <li><code>-13</code> if the parameter <code>params</code> is invalid, <code>params</code> is null.</li>
         *                  </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Sender#cancel(boolean, boolean, int) 
         * @see Sender#send(boolean, boolean, byte[], int, int, int, short, long, long) 
         * @see Mocket#getSender(boolean, boolean) 
         */
        public int replace (byte[] buffer, int oldTag, Params params)
            throws IOException,IllegalArgumentException
        {
            return replaceTagPar (buffer, 0, buffer.length, oldTag, params);
        }

        /**
         * First cancels any previously enqueued messages that have been tagged with the specified <code>OldTag</code>
         * value and then transmits the new message using the specified parameters. Note that there may be no old
         * messages to cancel, in which case this call behaves just like a <code>send</code>.
         * 
         * @param buffer    the buffer to be sent.
         * @param offset    the position in the buffer we start to send from.
         *                  E.g. offset=10, we send buffer starting at the 10th byte in the array.
         * @param length    the length for which we send.
         *                  E.g. length=50 we send 50 bytes of the buffer, starting at offset.
         *                  If offset=10, length=50 we send bytes from position 10 in the buffer through position 60.
         * @param oldTag    the tag of the messages to be replaced.
         * @param params    an instance of class <code>Param</code> that contains: tag, priority, enqueueTimeout, retryTimeout.
         * @return          <ul class="return">
         *                  <li><code>0</code> if the message is successfully enqueued to be sent and any messages tagged
         *                  with <code>oldTag</code> is successfully removed from the queues;</li>
         *                  <li><code>-1</code> if the retrieve of the instance of <code>Transmitter</code> did not work,
         *                  the value is null;</li>
         *                  <li><code>-2</code> if the cancellation of messages marked with <code>oldTag</code> did not work;</li>
         *                  <li><code>-3</code>  if the send operation on the new message did not work;</li>
         *                  <li><code>-10</code> if the parameter <code>oldTag</code> is illegal (less then zero);</li>
         *                  <li><code>-11</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                  <li><code>-12</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null;</li>
         *                  <li><code>-13</code> if the parameter <code>params</code> is invalid, <code>params</code> is null.</li>
         *                  </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @throws java.lang.IndexOutOfBoundsException  if the parameters <code>offset</code> or 
         *                                              <code>length</code> are illegal for <code>buffer</code>.
         * @see Sender#cancel(boolean, boolean, int) 
         * @see Sender#send(boolean, boolean, byte[], int, int, int, short, long, long)
         * @see Mocket#getSender(boolean, boolean) 
         */
        public int replace (byte []buffer, int offset, int length, int oldTag, Params params)
            throws IOException,IllegalArgumentException,IndexOutOfBoundsException
        {
            if (offset < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset + length) > buffer.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            return replaceTagPar (buffer, offset, length, oldTag, params);
        }

        /**
         * First cancels any previously enqueued messages that have been tagged with the specified <code>OldTag</code>
         * value and then transmits the new message using the specified parameters. Note that there may be no old
         * messages to cancel, in which case this call behaves just like a <code>send</code>.
         * 
         * @param buffer            the buffer to be sent. The buffer is going to be sent entirely.
         * @param oldTag            the tag of the messages to be replaced.
         * @param newTag            tag of the new message to be enqueued. The value has to be >0.
         * @param priority          used to assign a priority different than the default one (higher or lower).
         *                          The value has to be >0. Note that the priority of a message will grow every
         *                          time it is skipped in favor of a higher priority messages that gets sent first.
         *                          This mechanism is implemented to avoid message starvation. The range of priority
         *                          values is 0-255.
         * @param enqueueTimeout    indicates the length of time in milliseconds for which the method will wait
         *                          if there is no room in the outgoing buffer. A zero value indicates wait forever.
         * @param retryTimeout      indicates the length of time for which the transmitter will retransmit the packet
         *                          to ensure successful delivery. A zero value indicates retry with no time limit.
         *                          Note that this parameter makes sense only if the flow is reliable, otherwise the
         *                          behavior is to transmit and forget about the packet.
         * @return                  <ul class="return">
         *                          <li><code>0</code> if the message is successfully enqueued to be sent and any messages tagged
         *                          with <code>oldTag</code> is successfully removed from the queues;</li>
         *                          <li><code>-1</code> if the retrieve of the instance of <code>Transmitter</code> did not work,
         *                          the value is null;</li>
         *                          <li><code>-2</code> if the cancellation of messages marked with <code>oldTag</code> did not work;</li>
         *                          <li><code>-3</code>  if the send operation on the new message did not work;</li>
         *                          <li><code>-10</code> if the parameters are illegal (less then zero);</li>
         *                          <li><code>-11</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                          <li><code>-12</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
         *                          </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Sender#cancel(boolean, boolean, int) 
         * @see Sender#send(boolean, boolean, byte[], int, int, int, short, long, long)
         * @see Sender#setDefaultEnqueueTimeout(long) 
         * @see Sender#setDefaultRetryTimeout(long) 
         * @see Mocket#getSender(boolean, boolean) 
         */
        public int replace (byte[] buffer, int oldTag, int newTag, short priority, long enqueueTimeout, long retryTimeout)
            throws IOException,IllegalArgumentException
        {
            return replaceTagPrioEnqueRetr (buffer, 0, buffer.length, oldTag, newTag, priority, enqueueTimeout, retryTimeout);
        }

        /**
         * First cancels any previously enqueued messages that have been tagged with the specified <code>OldTag</code>
         * value and then transmits the new message using the specified parameters. Note that there may be no old
         * messages to cancel, in which case this call behaves just like a <code>send</code>.
         * 
         * @param buffer            the buffer to be sent.
         * @param offset            the position in the buffer we start to send from.
         *                          E.g. offset=10, we send buffer starting at the 10th byte in the array.
         * @param length            the length for which we send.
         *                          E.g. length=50 we send 50 bytes of the buffer, starting at offset.
         *                          If offset=10, length=50 we send bytes from position 10 in the buffer through position 60.
         * @param oldTag            the tag of the messages to be replaced.
         * @param newTag            tag of the new message to be enqueued. The value has to be >0.
         * @param priority          used to assign a priority different than the default one (higher or lower).
         *                          The value has to be >0. Note that the priority of a message will grow every
         *                          time it is skipped in favor of a higher priority messages that gets sent first.
         *                          This mechanism is implemented to avoid message starvation. The range of priority
         *                          values is 0-255.
         * @param enqueueTimeout    indicates the length of time in milliseconds for which the method will wait
         *                          if there is no room in the outgoing buffer. A zero value indicates wait forever.
         * @param retryTimeout      indicates the length of time for which the transmitter will retransmit the packet
         *                          to ensure successful delivery. A zero value indicates retry with no time limit.
         *                          Note that this parameter makes sense only if the flow is reliable, otherwise the
         *                          behavior is to transmit and forget about the packet.
         * @return                  <ul class="return">
         *                          <li><code>0</code> if the message is successfully enqueued to be sent and any messages tagged
         *                          with <code>oldTag</code> is successfully removed from the queues;</li>
         *                          <li><code>-1</code> if the retrieve of the instance of <code>Transmitter</code> did not work,
         *                          the value is null;</li>
         *                          <li><code>-2</code> if the cancellation of messages marked with <code>oldTag</code> did not work;</li>
         *                          <li><code>-3</code>  if the send operation on the new message did not work;</li>
         *                          <li><code>-10</code> if the parameters are illegal (less then zero);</li>
         *                          <li><code>-11</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null;</li>
         *                          <li><code>-12</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
         *                          </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Sender#cancel(boolean, boolean, int) 
         * @see Sender#send(boolean, boolean, byte[], int, int, int, short, long, long)
         * @see Sender#setDefaultEnqueueTimeout(long) 
         * @see Sender#setDefaultRetryTimeout(long) 
         * @see Mocket#getSender(boolean, boolean) 
         */
        public int replace (byte []buffer, int offset, int length, int oldTag, int newTag, short priority, long enqueueTimeout, long retryTimeout)
            throws IOException,IllegalArgumentException,IndexOutOfBoundsException
        {
            if (offset < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if (length < 0) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            if ((offset + length) > buffer.length) {
                throw new java.lang.IndexOutOfBoundsException();
            }
            return replaceTagPrioEnqueRetr (buffer, offset, length, oldTag, newTag, priority, enqueueTimeout, retryTimeout);
        }

        /**
         * Cancels (deletes) previously enqueued messages that have been tagged with the specified tag.
         * Note that the messages may be pending transmission (which applies to all flows)
         * or may have already been transmitted but not yet acknowledged (which only applies to reliable flows)
         * 
         * @param   tagId the tag of the messages to be removed.
         * @return  <ul class="return">
         *          <li>number of messages removed form the queues;</li>
         *          <li><code>-1</code> if cancel on pending packet queue (queue of messages waiting to be sent) fails;</li>
         *          <li><code>-2</code> if cancel on unacknowledged packet queue fails (only for reliable flow);</li>
         *          <li><code>-10</code> if the parameter <code>tagId</code> is illegal (less then zero);</li>
         *          <li><code>-11</code> if <code>MessageSender</code> is not initialized, <code>MessageSender</code> is null.</li>
         *          </ul>
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Mocket#getSender(boolean, boolean) 
         */
        public native int cancel (int tagId) throws IOException,IllegalArgumentException;

        /**
         * Sets the default timeout for enqueuing data into the outgoing queue. If the outgoing buffer is full,
         * a call to send will block until the timeout expires.
         * 0 is the default value. Specifying a timeout value of 0 disables the timeout, wait indefinitely.
         * 
         * @param enqueueTimeout    time in milliseconds the message will wait in the outgoing queue.
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Mocket#getSender(boolean, boolean) 
         */
        public native void setDefaultEnqueueTimeout (long enqueueTimeout) throws IOException,IllegalArgumentException;

        /**
         * Sets the default timeout for retransmission of reliable packets that have not been acknowledged.
         * Specifying a timeout value of 0 disables the timeout.
         * NOTE: Setting a timeout would imply that packets may not be reliable!
         * 
         * @param retryTimeout  time in milliseconds the message will wait in the unacknowledged packet queue.
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Mocket#getSender(boolean, boolean) 
         */
        public native void setDefaultRetryTimeout (long retryTimeout) throws IOException,IllegalArgumentException;

        /**
         * Removes <code>mocket</code> object.
         * 
         */
        protected void finalize()
        {
            //System.out.println("Debug java side: finalize");
            dispose();
        }

        private native void dispose();

        private native int sendNative (byte[] buffer, int offset, int length)  throws IOException,IllegalArgumentException;

        private native int gsend2Args (byte[] buf1, int offset1, int length1, byte[] buf2, int offset2, int length2) throws IOException,IllegalArgumentException;

        private native int gsend3Args (byte[] buf1, int offset1, int length1, byte[] buf2, int offset2, int length2, byte[] buf3,
                int offset3, int length3) throws IOException,IllegalArgumentException;

        private native int gsend4Args (byte[] buf1, int offset1, int length1, byte[] buf2, int offset2, int length2, byte[] buf3,
                int offset3, int length3, byte[] buf4, int offset4, int length4) throws IOException,IllegalArgumentException;

        private native int sendTagPrio(byte[] buffer, int offset, int length, int tag, short priority) throws IOException,IllegalArgumentException;

        private native int sendParams(byte[] buffer, int offset, int length, Params params) throws IOException,IllegalArgumentException;

        private native int sendTagPrioEnqueRetr (byte[] buffer, int offset, int length, int tag, short priority, long enqueueTimeout, long retryTimeout) throws IOException,IllegalArgumentException;

        private native int replaceNative (byte[] buffer, int offset, int length, int oldTag, int newTag) throws IOException,IllegalArgumentException;

        private native int replaceTagPar (byte[] Buffer, int offset, int length, int oldTag, Params params) throws IOException,IllegalArgumentException;

        private native int replaceTagPrioEnqueRetr (byte[] buffer, int offset, int length, int oldTag, int newTag, short priority,
                long enqueueTimeout, long retryTimeout) throws IOException,IllegalArgumentException;

        private long _messageSender;
    } // class Sender

    /**
     * The class that contains statistics about an active mocket connection.
     * Obtained by calling <code>getStatistics</code> on an instance of Mocket.
     */
    public class Statistics
    {
        /**
         * Returns the number of retransmitted packets during this mocket connection
         * 
         * @return  number of retransmitted packets;
         *          <code>-1</code> if it is unable to get a <code>MocketStats</code> object.
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Mocket#getStatistics() 
         */
        public native long getRetransmittedPacketCount()
            throws IOException, IllegalArgumentException;

        /**
         * Returns the number of sent packets during this mocket connection.
         * Includes the first transmission of packets from pending packet queue and also
         * retransmissions of packets from unacknowledged packet queue.
         * 
         * @return  number of sent packets;
         *          <code>-1</code> if it is unable to get a <code>MocketStats</code> object.
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Mocket#getStatistics() 
         */
        public native long getSentPacketCount()
            throws IOException, IllegalArgumentException;

        /**
         * Returns the number of bytes transmitted during this mocket connection.
         * Does not include retransmissions. The number of bytes in each packet are
         * evaluated only once when the packet is initially inserted into pending packet queue.
         * 
         * @return  number of byte transmitted;
         *          <code>-1</code> if it is unable to get a <code>MocketStats</code> object.
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Mocket#getStatistics() 
         */
        public native long getSentByteCount()
            throws IOException, IllegalArgumentException;

        /**
         * Returns the number of packets received during this mocket connection.
         * Includes duplicate packets.
         * 
         * @return  number of received packets;
         *          <code>-1</code> if it is unable to get a <code>MocketStats</code> object.
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Mocket#getStatistics() 
         */
        public native long getReceivedPacketCount()
            throws IOException, IllegalArgumentException;

        /**
         * Returns the number of bytes received during this mocket connection.
         * Does not include bytes from duplicate packets.
         * 
         * @return  number of bytes received;
         *          <code>-1</code> if it is unable to get a <code>MocketStats</code> object.
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Mocket#getStatistics() 
         */
        public native long getReceivedByteCount()
            throws IOException, IllegalArgumentException;

        /**
         * Returns the number of incoming packets that were discarded because they were duplicates.
         * 
         * @return  number of duplicated packets;
         *          <code>-1</code> if it is unable to get a <code>MocketStats</code> object.
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Mocket#getStatistics() 
         */
        public native long getDuplicatedDiscardedPacketCount()
            throws IOException,  IllegalArgumentException;

        /**
         * Returns the number of incoming packets that were discarded because there was no room to buffer them.
         * 
         * @return  number of packets discarded;
         *          <code>-1</code> if it is unable to get a <code>MocketStats</code> object.
         * @throws java.io.IOException
         * @throws java.lang.IllegalArgumentException
         * @see Mocket#getStatistics() 
         */
        public native long getNoRoomDiscardedPacketCount()
            throws IOException,  IllegalArgumentException;

        private long _statistics;
    } //class Statistics

    /**
     * Register a callback function to be invoked when no data (or keepalive) has
     * been received from the peer mocket.
     * The callback will indicate the time (in milliseconds) since last contact.
     * If the callback returns true, the mocket connection will be closed.
     * The peer is declared unreachable and the callback is invoked if no messages
     * are received from the peer for more than 2 seconds.
     * This waiting time is twice the default interval between keep-alive messages.
     * 
     * @param msl   callback function to register.
     */
    public void setStatusListener (MocketStatusListener msl)
    {
        if (msl == null) {
            return;
        }

        registerStatusListener (msl);
    }

    /**
     * Attempts to connect to a <code>ServerMocket</code> at the specified remote host on the specified remote port.
     * The host may be a hostname that can be resolved to an IP address or an IP address in string format (e.g. "127.0.0.1").
     * The default connection timeout is 30 seconds.
     * 
     * @param remoteHost    hostname of the remote host. It can be an IP address a string hostname that will be resolved.
     * @param remotePort    port where the <code>ServerMocket</code> is listening.
     * @return              <ul class="return">
     *                      <li><code>0</code> if the connection is successfully established;</li>
     *                      <li><code>-1</code> if the state of the mocket is not closed. The connection has already been established;</li>
     *                      <li><code>-2</code> if the port to connect to is null;</li>
     *                      <li><code>-3</code> if the lookup of the IP address failed;</li>
     *                      <li><code>-4</code> if the initialization of datagram socket failed;</li>
     *                      <li><code>-5</code> if setting UDP receiver buffer failed;</li>
     *                      <li><code>-6</code> if setting UDP timeout failed;</li>
     *                      <li><code>-7</code> if mocket state machine is in illegal state when receiving InitAck;</li>
     *                      <li><code>-8</code> if the connection timed out while waiting for InitAck;</li>
     *                      <li><code>-9</code> if mocket state machine is in illegal state when receiving CookieAck;</li>
     *                      <li><code>-10</code> if the connection timed out while waiting for CookieAck;</li>
     *                      <li><code>-20</code> if <code>remotePort</code> has an illegal value (less then zero);</li>
     *                      <li><code>-21</code> if <code>remoteHost</code> has an illegal value, host is null;</li>
     *                      <li><code>-22</code> if the object mocket is not initialized, mocket is null.</li>
     *                      </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int connect (String remoteHost,int remotePort)
        throws IOException,IllegalArgumentException
    {
        return connectNative (remoteHost, remotePort);
    }

    /**
     * Attempts to connect to a <code>ServerMocket</code> at the specified remote host on the specified remote port, and
     * choosing the connection attempt timeout.
     * The host may be a hostname that can be resolved to an IP address or an IP address in string format (e.g. "127.0.0.1").
     * 
     * @param remoteHost    hostname of the remote host. It can be an IP address a string hostname that will be resolved.
     * @param remotePort    port where the <code>ServerMocket</code> is listening.
     * @param timeout       time in milliseconds while mocket will attempt to connect to <code>ServerMocket</code>.
     * @return              <ul class="return">
     *                      <li><code>0</code> if the connection is successfully established;</li>
     *                      <li><code>-1</code> if the state of the mocket is not closed. The connection has already been established;</li>
     *                      <li><code>-2</code> if the port to connect to is null;</li>
     *                      <li><code>-3</code> if the lookup of the IP address failed;</li>
     *                      <li><code>-4</code> if the initialization of datagram socket failed;</li>
     *                      <li><code>-5</code> if setting UDP receiver buffer failed;</li>
     *                      <li><code>-6</code> if setting UDP timeout failed;</li>
     *                      <li><code>-7</code> if mocket state machine is in illegal state when receiving InitAck;</li>
     *                      <li><code>-8</code> if the connection timed out while waiting for InitAck;</li>
     *                      <li><code>-9</code> if mocket state machine is in illegal state when receiving CookieAck;</li>
     *                      <li><code>-10</code> if the connection timed out while waiting for CookieAck;</li>
     *                      <li><code>-20</code> if <code>remotePort</code> has an illegal value (less then zero);</li>
     *                      <li><code>-21</code> if <code>remoteHost</code> has an illegal value, host is null;</li>
     *                      <li><code>-22</code> if the object mocket is not initialized, mocket is null.</li>
     *                      </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int connect (String remoteHost, int remotePort, long timeout)
            throws IOException,IllegalArgumentException
    {
        return connectTimeout (remoteHost, remotePort, timeout);
    }
    
    /**
     * Attempts to connect to a <code>ServerMocket</code> at the specified remote host on the specified remote port, and
     * choosing the connection attempt timeout.
     * The host may be a hostname that can be resolved to an IP address or an IP address in string format (e.g. "127.0.0.1").
     * 
     * @param remoteHost    hostname of the remote host. It can be an IP address a string hostname that will be resolved.
     * @param remotePort    port where the <code>ServerMocket</code> is listening.
     * @param preExchangeKeys       specify if security keys should be exchanged at connection time so ReEstablishConnection is supported.
     * @return              <ul class="return">
     *                      <li><code>0</code> if the connection is successfully established;</li>
     *                      <li><code>-1</code> if the state of the mocket is not closed. The connection has already been established;</li>
     *                      <li><code>-2</code> if the port to connect to is null;</li>
     *                      <li><code>-3</code> if the lookup of the IP address failed;</li>
     *                      <li><code>-4</code> if the initialization of datagram socket failed;</li>
     *                      <li><code>-5</code> if setting UDP receiver buffer failed;</li>
     *                      <li><code>-6</code> if setting UDP timeout failed;</li>
     *                      <li><code>-7</code> if mocket state machine is in illegal state when receiving InitAck;</li>
     *                      <li><code>-8</code> if the connection timed out while waiting for InitAck;</li>
     *                      <li><code>-9</code> if mocket state machine is in illegal state when receiving CookieAck;</li>
     *                      <li><code>-10</code> if the connection timed out while waiting for CookieAck;</li>
     *                      <li><code>-20</code> if <code>remotePort</code> has an illegal value (less then zero);</li>
     *                      <li><code>-21</code> if <code>remoteHost</code> has an illegal value, host is null;</li>
     *                      <li><code>-22</code> if the object mocket is not initialized, mocket is null.</li>
     *                      </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int connect (String remoteHost, int remotePort, boolean preExchangeKeys)
            throws IOException,IllegalArgumentException
    {
        return connectExchangeKeysTimeout (remoteHost, remotePort, preExchangeKeys, 0);
    }

    /**
     * Attempts to connect to a <code>ServerMocket</code> at the specified remote host on the specified remote port, and
     * choosing the connection attempt timeout.
     * The host may be a hostname that can be resolved to an IP address or an IP address in string format (e.g. "127.0.0.1").
     *
     * @param remoteHost    hostname of the remote host. It can be an IP address a string hostname that will be resolved.
     * @param remotePort    port where the <code>ServerMocket</code> is listening.
     * @param preExchangeKeys   specify if security keys should be exchanged at connection time so ReEstablishConnection is supported.
     * @param timeout       time in milliseconds while mocket will attempt to connect to <code>ServerMocket</code>.
     * @return              <ul class="return">
     *                      <li><code>0</code> if the connection is successfully established;</li>
     *                      <li><code>-1</code> if the state of the mocket is not closed. The connection has already been established;</li>
     *                      <li><code>-2</code> if the port to connect to is null;</li>
     *                      <li><code>-3</code> if the lookup of the IP address failed;</li>
     *                      <li><code>-4</code> if the initialization of datagram socket failed;</li>
     *                      <li><code>-5</code> if setting UDP receiver buffer failed;</li>
     *                      <li><code>-6</code> if setting UDP timeout failed;</li>
     *                      <li><code>-7</code> if mocket state machine is in illegal state when receiving InitAck;</li>
     *                      <li><code>-8</code> if the connection timed out while waiting for InitAck;</li>
     *                      <li><code>-9</code> if mocket state machine is in illegal state when receiving CookieAck;</li>
     *                      <li><code>-10</code> if the connection timed out while waiting for CookieAck;</li>
     *                      <li><code>-20</code> if <code>remotePort</code> has an illegal value (less then zero);</li>
     *                      <li><code>-21</code> if <code>remoteHost</code> has an illegal value, host is null;</li>
     *                      <li><code>-22</code> if the object mocket is not initialized, mocket is null.</li>
     *                      </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int connect (String remoteHost, int remotePort, boolean preExchangeKeys, long timeout)
            throws IOException,IllegalArgumentException
    {
        return connectExchangeKeysTimeout (remoteHost, remotePort, preExchangeKeys, timeout);
    }

    /**
     * Attempts to connect to a <code>ServerMocket</code> at the specified remote host on the specified remote port.
     * The host may be a hostname that can be resolved to an IP address or an IP address in string format (e.g. "127.0.0.1").
     * The connection attempt is asynchronous, this call will return 0 on success and a
     * callback will notify the application when the connection attempt succeded or failed.
     *
     * @param remoteHost    hostname of the remote host. It can be an IP address a string hostname that will be resolved.
     * @param remotePort    port where the <code>ServerMocket</code> is listening.
     * @return              <ul class="return">
     *                      <li><code>0</code> if no error occurred and the connection is going to be establised;</li>
     *                      <li><code>-1</code> if an error occurred and the connection will not be established;</li>
     *                      </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int connectAsync (String remoteHost,int remotePort)
        throws IOException,IllegalArgumentException
    {
        return connectAsyncNative (remoteHost, remotePort);
    }

    /**
     * Attempts to connect to a <code>ServerMocket</code>, address and port are specified using an IP Socket Address.
     * The default connection timeout is 30 seconds.
     * 
     * @param addr          remote host address specified as an IP Socket Address.
     * @return              <ul class="return">
     *                      <li><code>0</code> if the connection is successfully established;</li>
     *                      <li><code>-1</code> if the state of the mocket is not closed. The connection has already been established;</li>
     *                      <li><code>-2</code> if the port to connect to is null;</li>
     *                      <li><code>-3</code> if the lookup of the IP address failed;</li>
     *                      <li><code>-4</code> if the initialization of datagram socket failed;</li>
     *                      <li><code>-5</code> if setting UDP receiver buffer failed;</li>
     *                      <li><code>-6</code> if setting UDP timeout failed;</li>
     *                      <li><code>-7</code> if mocket state machine is in illegal state when receiving InitAck;</li>
     *                      <li><code>-8</code> if the connection timed out while waiting for InitAck;</li>
     *                      <li><code>-9</code> if mocket state machine is in illegal state when receiving CookieAck;</li>
     *                      <li><code>-10</code> if the connection timed out while waiting for CookieAck;</li>
     *                      <li><code>-20</code> if <code>remotePort</code> has an illegal value (less then zero);</li>
     *                      <li><code>-21</code> if <code>remoteHost</code> has an illegal value, host is null;</li>
     *                      <li><code>-22</code> if the object mocket is not initialized, mocket is null.</li>
     *                      </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @see java.net.InetSocketAddress
     */
    public int connect (SocketAddress addr)
        throws IOException,IllegalArgumentException
    {
        InetSocketAddress inetSockAddr = (InetSocketAddress) addr;
        int remotePort = inetSockAddr.getPort();
        InetAddress inetAddr = inetSockAddr.getAddress();
        String remoteHost = inetAddr.getHostAddress();
        //System.out.println("Debug wrapper: host " + remoteHost + " port " + remotePort);
        return connectNative (remoteHost, remotePort);
    }

    /**
     * Attempts to connect to a <code>ServerMocket</code>, address and port are specified as a Socket Address.
     * Allows to choose the connection attempt timeout.
     * 
     * @param addr          remote host address specified as a SocketAddress.
     * @param timeout       time in milliseconds while mocket will attempt to connect to <code>ServerMocket</code>.
     * @return              <ul class="return">
     *                      <li><code>0</code> if the connection is successfully established;</li>
     *                      <li><code>-1</code> if the state of the mocket is not closed. The connection has already been established;</li>
     *                      <li><code>-2</code> if the port to connect to is null;</li>
     *                      <li><code>-3</code> if the lookup of the IP address failed;</li>
     *                      <li><code>-4</code> if the initialization of datagram socket failed;</li>
     *                      <li><code>-5</code> if setting UDP receiver buffer failed;</li>
     *                      <li><code>-6</code> if setting UDP timeout failed;</li>
     *                      <li><code>-7</code> if mocket state machine is in illegal state when receiving InitAck;</li>
     *                      <li><code>-8</code> if the connection timed out while waiting for InitAck;</li>
     *                      <li><code>-9</code> if mocket state machine is in illegal state when receiving CookieAck;</li>
     *                      <li><code>-10</code> if the connection timed out while waiting for CookieAck;</li>
     *                      <li><code>-20</code> if <code>remotePort</code> has an illegal value (less then zero);</li>
     *                      <li><code>-21</code> if <code>remoteHost</code> has an illegal value, host is null;</li>
     *                      <li><code>-22</code> if the object mocket is not initialized, mocket is null.</li>
     *                      </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int connect (SocketAddress addr, long timeout)
        throws IOException,IllegalArgumentException
    {
        InetSocketAddress inetSockAddr = (InetSocketAddress) addr;
        int remotePort = inetSockAddr.getPort();
        InetAddress inetAddr = inetSockAddr.getAddress();
        String remoteHost = inetAddr.getHostAddress();
        return connectTimeout (remoteHost, remotePort, timeout);
    }

    /**
     * Attempts to connect to a <code>ServerMocket</code>, address and port are specified using an IP Socket Address.
     * The connection attempt is asynchronous, this call will return 0 on success and a
     * callback will notify the application when the connection attempt succeded or failed.
     *
     * @param addr          remote host address specified as an IP Socket Address.
     * @return              <ul class="return">
     *                      <li><code>0</code> if no error occurred and the connection is going to be establised;</li>
     *                      <li><code>-1</code> if an error occurred and the connection will not be established;</li>
     *                      </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @see java.net.InetSocketAddress
     */
    public int connectAsync (SocketAddress addr)
        throws IOException,IllegalArgumentException
    {
        InetSocketAddress inetSockAddr = (InetSocketAddress) addr;
        int remotePort = inetSockAddr.getPort();
        InetAddress inetAddr = inetSockAddr.getAddress();
        String remoteHost = inetAddr.getHostAddress();
        System.out.println("Debug wrapper: host " + remoteHost + " port " + remotePort);
        return connectAsyncNative (remoteHost, remotePort);
    }

    /**
     *
     *
     *
     */
    public int finishConnect ()
    {
        return finishConnectNative();
    }

    /**
     * Binds the local end point to a particular address (interface) and port.
     * Calls to this method will work if invoked before calling <code>connect</code>
     * otherwise, it will return an error code. 
     * 
     * @param addr  remote host address specified as a SocketAddress.
     * @return      <code>0</code> if success;
     *              <code>&#60;0</code> if error.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int bind (SocketAddress addr)
        throws IOException, IllegalArgumentException
    {
        InetSocketAddress inetSockAddr = (InetSocketAddress) addr;
        int remotePort = inetSockAddr.getPort();
        InetAddress inetAddr = inetSockAddr.getAddress();
        String remoteHost = inetAddr.getHostAddress();
        return bindNative (remoteHost, remotePort);
    }

    /**
     * Connect to the remote host after a change of the machine's IP address and/or port due to a change in the network attachment.
     *
     * @return  0 if successful or a negative value in case of failure
     * @throws IOException
     * @throws IllegalArgumentException
     */
    public native int reEstablishConn() throws IOException,IllegalArgumentException;

    /**
     * Native method that returns the IP addess of the remote host.
     * The address is represented as a long value.
     * 
     * @return  remote host address.
     *          <code>-1</code> if the object mocket is not initialized, mocket is null.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public native long getRemoteAddress() throws IOException,IllegalArgumentException;

    /**
     * Native method that returns the remote port to which the connection has been established.
     * 
     * @return  remote port as an int value.
     *          <code>-1</code> if the object mocket is not initialized, mocket is null.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public native int getRemotePort() throws IOException,IllegalArgumentException;

    /**
     * Closes the current open connection to a remote endpoint.
     * 
     * @return  <code>0</code> if it ends successfully or if the connection was already closed;
     *          <code>-1</code> if the connection we are attempting to close is not in the ESTABLISHED state.
     *          <code>-10</code> if the object mocket is not initialized, mocket is null.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public native int close() throws IOException,IllegalArgumentException;

    /**
     * Invoked by the application to suspend the mocket.
     *
     * @return  Returns 0 in case of success and negative values in case of error
     * @throws IOException
     * @throws IllegalArgumentException
     */
    public native int suspend() throws IOException,IllegalArgumentException;

    /**
     * Invoked by the application if the suspend ends with success.
     * <p>
     * Creates an ObjectFreezer that contains the state of the mocket connection.
     * 
     * @param os    OutputStream object to write the state of the suspended Mocket over a channel to the new node.
     * @return      Returns 0 in case of success and a negative value in case of error
     * @throws IOException
     * @throws IllegalArgumentException
     */
    public int getState (OutputStream os)
        throws IOException, IllegalArgumentException
    {
        return getStateNative (os);
    }

    /**
     * Enables or disables cross sequencing across the reliable sequenced and unreliable sequenced packets.
     * 
     * @param enable    boolean value to indicate if we want to enable (true) or disable (false) the cross sequencing.
     * @return          <code>0</code> if ends with success.
     *                  <code>-1</code> if the object mocket is not initialized, mocket is null.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public native int enableCrossSequecing (boolean enable) throws IOException,IllegalArgumentException;

    /**
     * Returns the current setting for cross sequencing.
     * 
     * @return  boolean value that indicates if cross sequencing is enabled.
     *          <code>-1</code> if the object mocket is not initialized, mocket is null.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @see Mocket#enableCrossSequecing(boolean) 
     */
    public native boolean isCrossSequencingEnabled() throws IOException,IllegalArgumentException;

    /**
     * Obtains a new sender with the specified combination of reliability and sequencing.
     * 
     * @param reliable  select the type of flow: reliable (true) or unreliable (false).
     * @param sequenced select the type of flow: sequenced (true) or unsequenced (false).
     * @return          a <code>Sender</code> object to be used to send messages;
     *                  <code>NULL</code> if the objects mocket or sender are not initialized, mocket or sender is null.
     * @see Sender
     */
    public Sender getSender (boolean reliable, boolean sequenced)
    {
        Mocket.Sender sender = new Mocket.Sender();
        return getSenderNative (reliable, sequenced, sender);
    }

    /**
     * Enqueues the specified data for transmission.
     * 
     * @param reliable          select the type of flow: reliable (true) or unreliable (false).
     * @param sequenced         select the type of flow: sequenced (true) or unsequenced (false).
     * @param buffer            the buffer to be sent. The buffer is going to be sent entirely.
     * @param tag               used to identify the type of the packet.
     * @param priority          indicates the priority of the packet. The range of priority values is 0-255.
     * @param enqueueTimeout    indicates the length of time in milliseconds for which the method will wait if
     *                          there is no room in the outgoing buffer (a zero value indicates wait forever).
     * @param retryTimeout      indicates the length of time for which the transmitter will retransmit the packet
     *                          to ensure successful delivery (a zero value indicates retry with no time limit).
     * @return                  <code>0</code> if ends with success.
     *                          <code>&#60;0</code> if an error occurs.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int send (boolean reliable, boolean sequenced, byte []buffer, int tag, short priority,
            long enqueueTimeout,long retryTimeout)
            throws IOException,IllegalArgumentException
    {
        return sendNative (reliable, sequenced, buffer, 0, buffer.length, tag, priority, enqueueTimeout, retryTimeout);
    }

    /**
     * Enqueues the specified data for transmission.
     * 
     * @param reliable          select the type of flow: reliable (true) or unreliable (false).
     * @param sequenced         select the type of flow: sequenced (true) or unsequenced (false).
     * @param buffer            the buffer to be sent.
     * @param offset            the position in the buffer we start to send from.
     *                          E.g. offset=10, we send buffer starting at the 10th byte in the array.
     * @param length            the length for which we send.
     *                          E.g. length=50 we send 50 bytes of the buffer, starting at offset.
     *                          If offset=10, length=50 we send bytes from position 10 in the
     *                          buffer through position 60.
     * @param tag               used to identify the type of the packet.
     * @param priority          indicates the priority of the packet. The range of priority values is 0-255.
     * @param enqueueTimeout    indicates the length of time in milliseconds for which the method will wait if
     *                          there is no room in the outgoing buffer (a zero value indicates wait forever).
     * @param retryTimeout      indicates the length of time for which the transmitter will retransmit the packet
     *                          to ensure successful delivery (a zero value indicates retry with no time limit).
     * @return                  <code>0</code> if ends with success.
     *                          <code>&#60;0</code> if an error occurs.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @throws java.lang.IndexOutOfBoundsException  if the parameters <code>offset</code> or 
     *                                              <code>length</code> are illegal for <code>buffer</code>.
     */
     public int send (boolean reliable, boolean sequenced, byte []buffer, int offset, int length, int tag, short priority,
            long enqueueTimeout,long retryTimeout)
            throws IOException,IllegalArgumentException,IndexOutOfBoundsException
     {
        if (offset < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset + length) > buffer.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        return sendNative (reliable, sequenced, buffer, offset, length, tag, priority, enqueueTimeout, retryTimeout);
     }

     /**
      * 
      * @param reliable         select the type of flow: reliable (true) or unreliable (false).
      * @param sequenced        select the type of flow: sequenced (true) or unsequenced (false).
      * @param buffer           the buffer to be sent. The buffer is going to be sent entirely.
      * @param tag              used to identify the type of the packet.
      * @param priority         indicates the priority of the packet. The range of priority values is 0-255.
      * @param enqueueTimeout   indicates the length of time in milliseconds for which the method will wait if
      *                         there is no room in the outgoing buffer (a zero value indicates wait forever).
      * @param retryTimeout     indicates the length of time for which the transmitter will retransmit the packet
      *                         to ensure successful delivery (a zero value indicates retry with no time limit).
      * @param valist1
      * @param valist2
      * @return
      * @throws java.io.IOException
      * @throws java.lang.IllegalArgumentException
      */
    public int gsend (boolean reliable, boolean sequenced, byte []buffer, int tag, short priority,
            long enqueueTimeout, long retryTimeout, String valist1, String valist2)
            throws IOException,IllegalArgumentException
    {
        return gsendNative (reliable, sequenced, buffer, 0, buffer.length, tag, priority, enqueueTimeout, retryTimeout, valist1, valist2);
    }

    /**
     *
     * @param reliable          select the type of flow: reliable (true) or unreliable (false).
     * @param sequenced         select the type of flow: sequenced (true) or unsequenced (false).
     * @param buffer            the buffer to be sent. The buffer is going to be sent entirely.
     * @param offset            the position in the buffer we start to send from.
     *                          E.g. offset=10, we send buffer starting at the 10th byte in the array.
     * @param length            the length for which we send.
     *                          E.g. length=50 we send 50 bytes of the buffer, starting at offset.
     *                          If offset=10, length=50 we send bytes from position 10 in the
     *                          buffer through position 60.
     * @param tag               used to identify the type of the packet.
     * @param priority          indicates the priority of the packet. The range of priority values is 0-255.
     * @param enqueueTimeout    indicates the length of time in milliseconds for which the method will wait if
     *                          there is no room in the outgoing buffer (a zero value indicates wait forever).
     * @param retryTimeout      indicates the length of time for which the transmitter will retransmit the packet
     *                          to ensure successful delivery (a zero value indicates retry with no time limit).
     * @param valist1
     * @param valist2
     * @return
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @throws java.lang.IndexOutOfBoundsException  if the parameters <code>offset</code> or
     *                                              <code>length</code> are illegal for <code>buffer</code>.
     */
    public int gsend (boolean reliable, boolean sequenced, byte []buffer, int offset, int length, int tag, short priority,
            long enqueueTimeout, long retryTimeout, String valist1, String valist2)
            throws IOException,IllegalArgumentException,IndexOutOfBoundsException
     {
        if (offset < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset + length) > buffer.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        return gsendNative (reliable, sequenced, buffer, offset, length, tag, priority, enqueueTimeout, retryTimeout, valist1, valist2);
    }

    /**
     * Returns the size of the next message that is ready to be delivered to the application.
     * If no message is available, the call will block based on the timeout parameter.
     * A timeout of 0 implies that the default timeout should be used
     * whereas a timeout of -1 implies wait indefinitely.
     * 
     * @param Timeout   milliseconds to wait for a message available. A 0 value means use the default timeout (-1).
     *                  A -1 value means wait indefinitely.
     * @return          size of the message waiting to be read, in bytes;
     *                  <code>0</code> if no data is available in the specified timeout;
     *                  <code>-1</code> if the connection has been closed.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public native int getNextMessageSize (long Timeout) throws IOException, IllegalArgumentException;

    /**
     * Retrieves the data from next message that is ready to be delivered to the application.
     * At most <code>buffer.length</code> bytes are copied into the specified buffer. 
     * NOTE: Any additional data in the packet that will not fit in the buffer is discarded.
     * The receive is a blocking function, waits until there are data to be received or the timeout expires.
     * The timeout is not specified, the default value is going to be used. The default value is -1.
     * A timeout of -1 implies wait indefinitely.
     * 
     * @param buffer    byte array where the data is copied for delivery.
     * @return          <ul class="return">
     *                  <li>returns the number of bytes that were copied into the buffer;</li>
     *                  <li><code>-1</code> if an error occurred;</li>
     *                  <li><code>-10</code> if the object mocket is not initialized, mocket is null;</li>
     *                  <li><code>-11</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
     *                  </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int receive (byte []buffer)
        throws IOException,IllegalArgumentException
    {
        return receiveNative (buffer, 0, buffer.length, 0);
    }

    /**
     * Retrieves the data from next message that is ready to be delivered to the application.
     * The bytes are copied into the specified buffer starting at the specified offset.
     * At most <code>length</code> bytes are copied. 
     * NOTE: Any additional data in the packet that will not fit in the buffer is discarded.
     * The receive is a blocking function, waits until there are data to be received or the timeout expires.
     * The timeout is not specified, the default value is going to be used. The default value is -1.
     * A timeout of -1 implies wait indefinitely.
     * 
     * @param buffer    byte array where the data is copied for delivery.
     * @param offset    the point in the buffer where we start to copy the data.
     * @param length    the number of bytes that are copied into the buffer.
     * @return          <ul class="return">
     *                  <li>returns the number of bytes that were copied into the buffer;</li>
     *                  <li><code>-1</code> if an error occurred;</li>
     *                  <li><code>-10</code> if the object mocket is not initialized, mocket is null;</li>
     *                  <li><code>-11</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
     *                  </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @throws java.lang.IndexOutOfBoundsException  if the parameters <code>offset</code> or 
     *                                              <code>length</code> are illegal for <code>buffer</code>.
     */
    public int receive (byte []buffer, int offset, int length)
        throws IOException,IllegalArgumentException,IndexOutOfBoundsException
    {
        if (length == 0) {
            return 0;
        }
        if (offset < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset + length) > buffer.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        return receiveNative (buffer, offset, length, 0);
    }

    /**
     * Retrieves the data from next message that is ready to be delivered to the application.
     * At most <code>buffer.length</code> bytes are copied into the specified buffer. 
     * NOTE: Any additional data in the packet that will not fit in the buffer is discarded.
     * The receive is a blocking function, waits until there are data to be received or the timeout expires.
     * This function allows to specify a value for the timeout.
     * Specifying a timeout of 0 implies that the default timeout should be used whereas a timeout
     * of -1 implies wait indefinitely. The default timeout is -1.
     * 
     * @param buffer    byte array where the data is copied for delivery.
     * @param timeout   milliseconds to wait for data.
     * @return          <ul class="return">
     *                  <li>returns the number of bytes that were copied into the buffer;</li>
     *                  <li><code>-1</code> if an error occurred;</li>
     *                  <li><code>-10</code> if the object mocket is not initialized, mocket is null;</li>
     *                  <li><code>-11</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
     *                  </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int receive (byte []buffer, long timeout)
        throws IOException,IllegalArgumentException
    {
        return receiveNative (buffer, 0, buffer.length, timeout);
    }

    /**
     * Retrieves the data from next message that is ready to be delivered to the application.
     * The bytes are copied into the specified buffer starting at the specified offset.
     * At most <code>length</code> bytes are copied. 
     * NOTE: Any additional data in the packet that will not fit in the buffer is discarded.
     * The receive is a blocking function, waits until there are data to be received or the timeout expires.
     * This function allows to specify a value for the timeout.
     * Specifying a timeout of 0 implies that the default timeout should be used whereas a timeout
     * of -1 implies wait indefinitely. The default timeout is -1.
     * 
     * @param buffer    byte array where the data is copied for delivery.
     * @param offset    the point in the buffer where we start to copy the data.
     * @param length    the number of bytes that are copied into the buffer.
     * @param timeout   milliseconds to wait for data.
     * @return          <ul class="return">
     *                  <li>returns the number of bytes that were copied into the buffer;</li>
     *                  <li><code>-1</code> if an error occurred;</li>
     *                  <li><code>-10</code> if the object mocket is not initialized, mocket is null;</li>
     *                  <li><code>-11</code> if the parameter <code>buffer</code> is invalid, <code>buffer</code> is null.</li>
     *                  </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @throws java.lang.IndexOutOfBoundsException  if the parameters <code>offset</code> or 
     *                                              <code>length</code> are illegal for <code>buffer</code>.
     */
    public int receive (byte []buffer, int offset, int length, long timeout)
        throws IOException, IllegalArgumentException, IndexOutOfBoundsException
    {
        if (length == 0) {
            return 0;
        }
        if (offset < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset + length) > buffer.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        return receiveNative (buffer, offset, length, timeout);
    }

    /**
     * Retrieves the data from next message that is ready to be delivered to the application.
     * The bytes are returned as return value.
     * The receive is a blocking function, waits until there are data to be received or the timeout expires.
     * This function allows to specify a value for the timeout.
     * Specifying a timeout of 0 implies that the default timeout should be used whereas a timeout
     * of -1 implies wait indefinitely. The default timeout is -1.
     * 
     * @param timeout   milliseconds to wait for data.
     * @return          a buffer containing the data read;
     *                  <code>NULL</code> if an error occurred.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public byte[] receive (long timeout)
        throws IOException, IllegalArgumentException
    {
        int byteArraySize = getNextMessageSize (timeout);
        if (byteArraySize <= 0) {
            return null;
        }
        else {
            return receiveBuffer (timeout);
        }
    }

    /**
     * Retrieves the data from the next message that is ready to be delivered to the application
     * splitting the data in different buffers.
     * Not specifying a timeout or a timeout of 0 implies that the default timeout should be used
     * whereas a timeout of -1 implies wait indefinitely.
     * The data is scattered into the buffers that are passed into the method.
     * Note: Any additional data in the packet that will not fit in the buffers is discarded.
     * <p>
     * E.g.: if the caller passes in two buffers, sreceive (pBufOne, pBufTwo, iTimeout),
     * and the method returns 4000, the implication is that pBufOne.length bytes were read into pBufOne,
     * pBufTwo.length bytes into pBufTwo. If pBufOne.length+pBufTwo.length&#60;4000 all the data are read,
     * otherwise some data is lost.
     *
     * @param buf1      first byte array where data is copied for delivery.
     * @param buf2      second byte array where additional data is copied for delivery.
     * @param timeout   milliseconds to wait for data.
     * @return          returns the total number of bytes that were copied into all the buffers;
     *                  <code>-1</code> in case of the connection being closed;
     *                  <code>0</code> in case no data is available within the specified timeout.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int sreceive (byte[] buf1, byte[] buf2, long timeout)
        throws IOException, IllegalArgumentException
    {
        return sreceive2Args(buf1, 0, buf1.length, buf2, 0, buf2.length, timeout);
    }

    /**
     * Retrieves the data from the next message that is ready to be delivered to the application
     * splitting the data in different buffers.
     * Not specifying a timeout or a timeout of 0 implies that the default timeout should be used
     * whereas a timeout of -1 implies wait indefinitely.
     * The data is scattered into the buffers that are passed into the method.
     * The pointer to the buffer and the buffer size arguments must be passed in pairs.
     * Note: Any additional data in the packet that will not fit in the buffers is discarded.
     * <p>
     * E.g.: if the caller passes in two buffers, sreceive (pBufOne, 0, 8, pBufTwo, 0, 1024, iTimeout),
     * and the method returns 500, the implication is that 8 bytes were read into pBufOne and 492 bytes into
     * pBufTwo.
     * 
     * @param buf1      first byte array where data is copied for delivery.
     * @param offset1   the point in the first buffer where we start to copy the data.
     * @param length1   the number of bytes that are copied into the first buffer.
     * @param buf2      second byte array where additional data is copied for delivery.
     * @param offset2   the point in the second buffer where we start to copy the data.
     * @param length2   the number of bytes that are copied into the second buffer.
     * @param timeout   milliseconds to wait for data.
     * @return          returns the total number of bytes that were copied into all the buffers;
     *                  <code>-1</code> in case of the connection being closed;
     *                  <code>0</code> in case no data is available within the specified timeout.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @throws java.lang.IndexOutOfBoundsException  if the parameters offset or length are illegal for buffer.
     */
    public int sreceive (byte[] buf1, int offset1, int length1, byte[] buf2, int offset2, int length2, long timeout)
        throws IOException, IllegalArgumentException, IndexOutOfBoundsException
    {
        if (offset1 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length1 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset1 + length1) > buf1.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (offset2 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length2 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset2 + length2) > buf2.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        return sreceive2Args(buf1, offset1, length1, buf2, offset2, length2, timeout);
    }

    /**
     * Retrieves the data from the next message that is ready to be delivered to the application
     * splitting the data in different buffers.
     * Not specifying a timeout or a timeout of 0 implies that the default timeout should be used
     * whereas a timeout of -1 implies wait indefinitely.
     * The data is scattered into the buffers that are passed into the method.
     * The pointer to the buffer and the buffer size arguments must be passed in pairs.
     * Note: Any additional data in the packet that will not fit in the buffers is discarded.
     * <p>
     * E.g.: if the caller passes in three buffers, sreceive (pBufOne, pBufTwo, pBufThree, iTimeout),
     * and the method returns 4000, the implication is that pBufOne.length bytes were read into pBufOne,
     * pBufTwo.length bytes into pBufTwo, and the remaining pBufThree.length bytes into pBufThree.
     * If pBufOne.length+pBufTwo.length+pBufThree.length&#60;4000 all the data are read, otherwise some data is lost.
     * 
     * @param buf1      first byte array where data is copied for delivery.
     * @param buf2      second byte array where additional data is copied for delivery.
     * @param buf3      third byte array where additional data is copied for delivery.
     * @param timeout   milliseconds to wait for data.
     * @return          returns the total number of bytes that were copied into all the buffers;
     *                  <code>-1</code> in case of the connection being closed;
     *                  <code>0</code> in case no data is available within the specified timeout.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int sreceive (byte[] buf1, byte[] buf2, byte[] buf3, long timeout)
        throws IOException,IllegalArgumentException
    {
        return sreceive3Args(buf1, 0, buf1.length, buf2, 0, buf2.length, buf3, 0, buf3.length, timeout);
    }

    /**
     * Retrieves the data from the next message that is ready to be delivered to the application
     * splitting the data in different buffers.
     * Not specifying a timeout or a timeout of 0 implies that the default timeout should be used
     * whereas a timeout of -1 implies wait indefinitely.
     * The data is scattered into the buffers that are passed into the method.
     * The pointer to the buffer and the buffer size arguments must be passed in pairs.
     * Note: Any additional data in the packet that will not fit in the buffers is discarded.
     * <p>
     * E.g.: If the caller passes in three buffers, sreceive (pBufOne, 0, 8, pBufTwo, 0, 1024, pBufThree, 0, 4096, iTimeout),
     * and the method returns 4000, the implication is that 8 bytes were read into pBufOne, 1024 bytes into pBufTwo,
     * and the remaining 2968 bytes into pBufThree.
     * 
     * @param buf1      first byte array where data is copied for delivery.
     * @param offset1   the point in the first buffer where we start to copy the data.
     * @param length1   the number of bytes that are copied into the first buffer.
     * @param buf2      second byte array where additional data is copied for delivery.
     * @param offset2   the point in the second buffer where we start to copy the data.
     * @param length2   the number of bytes that are copied into the second buffer.
     * @param buf3      third byte array where additional data is copied for delivery.
     * @param offset3   the point in the third buffer where we start to copy the data.
     * @param length3   the number of bytes that are copied into the third buffer.
     * @param timeout   milliseconds to wait for data.
     * @return          returns the total number of bytes that were copied into all the buffers;
     *                  <code>-1</code> in case of the connection being closed;
     *                  <code>0</code> in case no data is available within the specified timeout.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @throws java.lang.IndexOutOfBoundsException  if the parameters offset or length are illegal for buffer.
     */
    public int sreceive (byte[] buf1, int offset1, int length1, byte[] buf2, int offset2, int length2, byte[] buf3, int offset3, int length3, long timeout)
        throws IOException,IllegalArgumentException,IndexOutOfBoundsException
    {
        if (offset1 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length1 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset1 + length1) > buf1.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (offset2 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length2 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset2 + length2) > buf2.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
         if (offset3 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length3 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset3 + length3) > buf3.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        return sreceive3Args(buf1, offset1, length1, buf2, offset2, length2, buf3, offset3, length3, timeout);
    }

    /**
     * Retrieves the data from the next message that is ready to be delivered to the application
     * splitting the data in different buffers.
     * Not specifying a timeout or a timeout of 0 implies that the default timeout should be used
     * whereas a timeout of -1 implies wait indefinitely.
     * The data is scattered into the buffers that are passed into the method.
     * The pointer to the buffer and the buffer size arguments must be passed in pairs.
     * Note: Any additional data in the packet that will not fit in the buffers is discarded.
     * <p>
     * E.g.: if the caller passes in four buffers, sreceive (pBufOne, pBufTwo, pBufThree, pBufFour, iTimeout),
     * and the method returns 4000, the implication is that pBufOne.length bytes were read into pBufOne,
     * pBufTwo.length bytes into pBufTwo, pBufThree.length bytes into pBufThree, and the remaining pBufFour.length
     * bytes into pBufFour. If pBufOne.length+pBufTwo.length+pBufThree.length+pBufFour.length&#60;4000 all the
     * data are read, otherwise some data is lost.
     * 
     * @param buf1      first byte array where data is copied for delivery.
     * @param buf2      second byte array where additional data is copied for delivery.
     * @param buf3      third byte array where additional data is copied for delivery.
     * @param buf4      fourth byte array where additional data is copied for delivery.
     * @param timeout   milliseconds to wait for data.
     * @return          returns the total number of bytes that were copied into all the buffers;
     *                  <code>-1</code> in case of the connection being closed;
     *                  <code>0</code> in case no data is available within the specified timeout.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int sreceive (byte[] buf1, byte[] buf2, byte[] buf3, byte[] buf4, long timeout)
        throws IOException, IllegalArgumentException
    {
        return sreceive4Args(buf1, 0, buf1.length, buf2, 0, buf2.length, buf3, 0, buf3.length, buf4, 0, buf4.length, timeout);
    }

    /**
     * Retrieves the data from the next message that is ready to be delivered to the application
     * splitting the data in different buffers.
     * Not specifying a timeout or a timeout of 0 implies that the default timeout should be used
     * whereas a timeout of -1 implies wait indefinitely.
     * The data is scattered into the buffers that are passed into the method.
     * The pointer to the buffer and the buffer size arguments must be passed in pairs.
     * Note: Any additional data in the packet that will not fit in the buffers is discarded.
     * <p>
     * E.g.: If the caller passes in four buffers, sreceive (pBufOne, 0, 8, pBufTwo, 0, 1024, pBufThree,
     * 0, 1024, pBufFour, 0 4096, iTimeout), and the method returns 4000, the implication is that 8 bytes
     * were read into pBufOne, 1024 bytes into pBufTwo, 1024 bytes into pBufThree, and the remaining 1944
     * bytes into pBufFour.
     * 
     * @param buf1      first byte array where data is copied for delivery.
     * @param offset1   the point in the first buffer where we start to copy the data.
     * @param length1   the number of bytes that are copied into the first buffer.
     * @param buf2      second byte array where additional data is copied for delivery.
     * @param offset2   the point in the second buffer where we start to copy the data.
     * @param length2   the number of bytes that are copied into the second buffer.
     * @param buf3      third byte array where additional data is copied for delivery.
     * @param offset3   the point in the third buffer where we start to copy the data.
     * @param length3   the number of bytes that are copied into the third buffer.
     * @param buf4      fourth byte array where additional data is copied for delivery.
     * @param offset4   the point in the fourth buffer where we start to copy the data.
     * @param length4   the number of bytes that are copied into the fourth buffer.
     * @param timeout   milliseconds to wait for data.
     * @return          returns the total number of bytes that were copied into all the buffers;
     *                  <code>-1</code> in case of the connection being closed;
     *                  <code>0</code> in case no data is available within the specified timeout.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @throws java.lang.IndexOutOfBoundsException  if the parameters offset or length are illegal for buffer.
     */
    public int sreceive (byte[] buf1, int offset1, int length1, byte[] buf2, int offset2, int length2, byte[] buf3, int offset3,
                         int length3, byte[] buf4, int offset4, int length4, long timeout)
        throws IOException, IllegalArgumentException, IndexOutOfBoundsException
    {
        if (offset1 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length1 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset1 + length1) > buf1.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (offset2 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length2 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset2 + length2) > buf2.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
         if (offset3 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length3 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset3 + length3) > buf3.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
         if (offset4 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length4 < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset4 + length4) > buf4.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        return sreceive4Args(buf1, offset1, length1, buf2, offset2, length2, buf3, offset3, length3, buf4, offset4, length4, timeout);
    }

    /**
     * First cancels any previously enqueued messages that have been tagged with the specified <code>OldTag</code>
     * value and then transmits the new message using the specified parameters. Note that there may be no old
     * messages to cancel, in which case this call behaves just like a <code>send</code>.
     * 
     * @param reliable          select the type of flow: reliable (true) or unreliable (false).
     * @param sequenced         select the type of flow: sequenced (true) or unsequenced (false).
     * @param buffer            the buffer to be sent. The buffer is going to be sent entirely.
     * @param oldTag            tag of the messages to replace. Messages tagged with <code>oldTag</code> will be canceled.
     * @param newTag            tag of the new message to be enqueued. The value has to be >0.
     * @param priority          indicates the priority of the packet. The range of priority values is 0-255.
     * @param enqueueTimeout    indicates the length of time in milliseconds for which the method will wait if
     *                          there is no room in the outgoing buffer (a zero value indicates wait forever).
     * @param retryTimeout      indicates the length of time for which the transmitter will retransmit the packet
     *                          to ensure successful delivery (a zero value indicates retry with no time limit).
     * @return                  <ul class="return">
     *                          <li><code>0</code> if ends with success;</li>
     *                          <li><code>-1</code> if an error occurred;</li>
     *                          <li><code>-2</code> if the <code>cancel</code> function ended with error;</li>
     *                          <li><code>-3</code> if the <code>send</code> function ended with error.</li>
     *                          </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public int replace (boolean reliable, boolean sequenced,
                        byte[] buffer,
                        int oldTag, int newTag,
                        short priority,
                        long enqueueTimeout, long retryTimeout)
            throws IOException, IllegalArgumentException
    {
        return replaceNative (reliable, sequenced, buffer, 0, buffer.length, oldTag, newTag, priority, enqueueTimeout, retryTimeout);
    }

    /**
     * First cancels any previously enqueued messages that have been tagged with the specified <code>OldTag</code>
     * value and then transmits the new message using the specified parameters. Note that there may be no old
     * messages to cancel, in which case this call behaves just like a <code>send</code>.
     * 
     * @param reliable          select the type of flow: reliable (true) or unreliable (false).
     * @param sequenced         select the type of flow: sequenced (true) or unsequenced (false).
     * @param buffer            the buffer to be sent.
     * @param offset            the position in the buffer we start to send from.
     *                          E.g. offset=10, we send buffer starting at the 10th byte in the array
     * @param length            the length for which we send.
     *                          E.g. length=50 we send 50 bytes of the buffer, starting at offset.
     *                          If offset=10, length=50 we send bytes from position 10 in the
     *                          buffer through position 60.
     * @param oldTag            tag of the messages to replace. Messages tagged with <code>oldTag</code> will be canceled.
     * @param newTag            tag of the new message to be enqueued. The value has to be >0.
     * @param priority          indicates the priority of the packet. The range of priority values is 0-255.
     * @param enqueueTimeout    indicates the length of time in milliseconds for which the method will wait if
     *                          there is no room in the outgoing buffer (a zero value indicates wait forever).
     * @param retryTimeout      indicates the length of time for which the transmitter will retransmit the packet
     *                          to ensure successful delivery (a zero value indicates retry with no time limit).
     * @return                  <ul class="return">
     *                          <li><code>0</code> if ends with success;</li>
     *                          <li><code>-1</code> if an error occurred;</li>
     *                          <li><code>-2</code> if the <code>cancel</code> function ended with error;</li>
     *                          <li><code>-3</code> if the <code>send</code> function ended with error.</li>
     *                          </ul>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @throws java.lang.IndexOutOfBoundsException  if the parameters <code>offset</code> or 
     *                                              <code>length</code> are illegal for <code>buffer</code>.
     */
    public int replace (boolean reliable, boolean sequenced,
                        byte[] buffer,
                        int offset, int length,
                        int oldTag, int newTag,
                        short priority,
                        long enqueueTimeout, long retryTimeout)
            throws IOException,IllegalArgumentException,IndexOutOfBoundsException
     {
        if (offset < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if (length < 0) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        if ((offset + length) > buffer.length) {
            throw new java.lang.IndexOutOfBoundsException();
        }
        return replaceNative (reliable, sequenced, buffer, offset, length,oldTag, newTag, priority, enqueueTimeout, retryTimeout);
    }

    /**
     * Cancels (deletes) previously enqueued messages that have been tagged with the specified tag.
     * Note that the messages may be pending transmission (which applies to all flows) or may have
     * already been transmitted but not yet acknowledged (which only applies to reliable flows).
     * 
     * @param reliable      select the type of flow: reliable (true) or unreliable (false).
     * @param sequenced     select the type of flow: sequenced (true) or unsequenced (false).
     * @param tagId         used to identify the packets to delete.
     * @return              number of messages removed form the queues;
     *                      <code>-1</code> if an error occurred.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @see Sender#cancel(int) to obtain the same result specifying less parameters.
     */
    public native int cancel (boolean reliable,boolean sequenced,int tagId) throws IOException,IllegalArgumentException;

    /**
     * Sets the length of time (in milliseconds) for which a connection should linger before
     * closing in case there is unsent data.
     * A timeout value of 0 implies that the connection should wait indefinitely until all data has been sent.
     * 
     * @param lingerTime    milliseconds for which the connection should linger.
     * @return              <code>0</code> if success.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public native int setConnectionLingerTime (long lingerTime) throws IOException,IllegalArgumentException;

    /**
     * Returns the current setting for the connection linger time.
     * The linger time represents the amount of time (in milliseconds) for which a connection
     * should linger before closing in case there is unsent data.
     * A value of zero means wait indefinitely.
     * 
     * @return  long representing the connection linger time in milliseconds.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     * @see Mocket#setConnectionLingerTime(long) 
     */
    public native long getConnectionLingerTime() throws IOException,IllegalArgumentException;

    /**
     * Returns a pointer to the Statistics class that maintains statistics about this mocket connection.
     * 
     * @return  an object of class <code>Statistic</code>.
     *          <code>NULL</code> if the objects mocket or statistic are not initialized, mocket or statistic is null.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public Statistics getStatistics()
        throws IOException,IllegalArgumentException
    {
        Mocket.Statistics statistics = new Mocket.Statistics();
        return getStatisticsNative (statistics);
    }

    /**
     * Returns the maximum MTU that may be used.
     * 
     * @return  integer representing the maximum MTU that may be used.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public native static int getMaximumMTU() throws IOException,IllegalArgumentException;

    /**
     * Extract the address of the peer in the form Socket Address.
     * 
     * @return  the address, in the form Socket Address, of the peer;
     *          <code>NULL</code> if an error occurred.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public SocketAddress getPeerName()
        throws IOException,IllegalArgumentException
    {
        getPeerNameNative();
        if (_remoteAddress == null) {
            return null;
        }
        InetAddress remoteIP = InetAddress.getByName (_remoteAddress);
        InetSocketAddress addr = new InetSocketAddress (remoteIP, _remotePort);
        return addr;
    }

    public native void useTwoWayHandshake();

    //public native void useTransmissionRateModulation();

    /**
     * Even when keepAlive are disabled this timeout is used to trigger peerUnreachable callbacks
     * @param timeout
     */
    public native void setKeepAliveTimeout (int timeout);

    public native void disableKeepAlive();

    /**
     * InitialAssumedRTT is used to calculate the retransmission timeout (RTO) of packets
     * @param RTT
     */
    public native void setInitialAssumedRTT (long RTT);

    /**
     * The value of the maximum RTO is zero by default. A value of zero means no maximum RTO
     * @param RTO
     */
    public native void setMaximumRTO (long RTO);

    /**
     * Set a minimum value for the Retransmission timeout.
     * @param RTO
     */
    public native void setMinimumRTO (long RTO);
    
    /**
     * RTO factor and RTO constant are used in the calculation of the RTO for the packet according to this formula:
     * (_fSRTT + RTOConstant) * RTOFactor * (1 + pWrapper->getRetransmitCount())
     * @param RTOfactor
     */
    public native void setRTOFactor (double RTOfactor);

    /**
     * RTO factor and RTO constant are used in the calculation of the RTO for the packet according to this formula:
     * (_fSRTT + RTOConstant) * RTOFactor * (1 + pWrapper->getRetransmitCount())
     * @param RTOconstant
     */
    public native void setRTOConstant (int RTOconstant);

    /**
     * Disables the the factor (1 + pWrapper->getRetransmitCount()) from RTO calculation
     */
    public native void disableRetransmitCountFactorInRTO();

    public native void setMaximumWindowSize (long windowSize);

    public native void setSAckTransmitTimeout (int sackTransTO);

    public native void setConnectTimeout (long connectTO);

    /**
     * Low level socket timeout at connection time
     * @param UDPConnTO
     */
    public native void setUDPReceiveConnectionTimeout (int UDPConnTO);

    /**
     * Low level socket timeout after connection is open
     * @param UDPRecTO
     */
    public native void setUDPReceiveTimeout (int UDPRecTO);

    /**
     * Activates congestion control in Mockets.
     * <p>
     * Must be called after connect.
     *
     * @return  0 if success, -1 if error
     */
    public native int activateCongestionControl();

    /**
     * Use to activate debugging of the state capture during mockets migration.
     * <p>
     * This will disable sending of messages with odd sequence number in order to
     * perform a migration with messages in the queues.
     * 
     */
    public native void debugStateCapture();

    /**
     * Native method that initializes an object <code>mocket</code>
     * with the specified config file.
     */
    private native void init (String configFile) throws IOException;

    /**
     * Native method that initializes an object <code>mocket</code>DTLS
     * with the specified config file.
     */
    private native void initDtls (String configFile, String pathToCertificate, String pathToPrivateKey) throws IOException;

//    /**
//     * Native method that removes <code>mocket</code> object.
//     */
//    protected void finalize()
//    {
//        dispose();
//    }

    private native void dispose();

    private native int sendNative (boolean reliable, boolean sequenced, byte []buffer, int offset, int length, int tag,
                                   short priority, long enqueueTimeout, long retryTimeout)
        throws IOException,IllegalArgumentException;

    private native int gsendNative (boolean reliable, boolean sequencedd, byte []buffer, int offset, int length, int tag,
                                    short priority, long enqueueTimeout, long retryTimeout, String valist1, String valist2)
        throws IOException, IllegalArgumentException;

    private native int receiveNative (byte []buffer, int offset, int length, long timeout)
        throws IOException, IllegalArgumentException;

    private native int sreceive2Args (byte[] buf1, int offset1, int length1, byte[] buf2, int offset2, int length2, long timeout)
        throws IOException, IllegalArgumentException;

    private native int sreceive3Args (byte[] buf1, int offset1, int length1, byte[] buf2, int offset2, int length2, byte[] buf3,
                                      int offset3, int length3, long timeout)
        throws IOException, IllegalArgumentException;

    private native int sreceive4Args (byte[] buf1, int offset1, int length1, byte[] buf2, int offset2, int length2, byte[] buf3,
                                      int offset3, int length3, byte[] buf4, int offset4, int length4, long timeout)
        throws IOException, IllegalArgumentException;

    private native int replaceNative (boolean reliable, boolean sequenced, byte []buffer, int offset, int length, int oldTag,
                                      int newTag, short priority, long enqueueTimeout, long retryTimeout)
        throws IOException, IllegalArgumentException;

    private native byte[] receiveBuffer (long timeout)
        throws IOException, IllegalArgumentException;

    private native int connectNative (String remoteHost, int remotePort)
        throws IOException, IllegalArgumentException;

    private native int connectExchangeKeysTimeout (String remoteHost, int remotePort, boolean preExchangeKeys, long timeout)
            throws IOException, IllegalArgumentException;

    private native int connectAsyncNative (String remoteHost,int remotePort)
        throws IOException, IllegalArgumentException;

    private native int connectTimeout (String remoteHost, int remotePort, long timeout)
        throws IOException, IllegalArgumentException;

    private native int finishConnectNative();

    private native int bindNative (String remoteHost, int RemotePort)
        throws IOException, IllegalArgumentException;

    private native int getStateNative (OutputStream os)
            throws IOException, IllegalArgumentException;

    private native void registerStatusListener (MocketStatusListener msl);

    private native void getPeerNameNative();

    private native Sender getSenderNative (boolean reliable, boolean sequenced, Sender sender);

    private native Statistics getStatisticsNative (Statistics statistics);

    /**
     * Sets a string to use as the application or user friendly identifier for this mocket instance.
     * The identifier is used when sending out statistics and when logging information.
     * Some suggestions include the name of the application, the purpose for this mocket, etc.
     * May be set to NULL to clear a previously set identifier.
     * NOTE: The string is copied internally, so the caller does not need to preserve the string
     * 
     * @param identifier    string to be used to identify this mocket instance.
     */
    public native void setIdentifier (String identifier);
    
    /**
     * Returns the identifier for this mocket instance.
     * 
     * @return  identifier of this mocket instance;
     *          <code>NULL</code> if no identifier is set.
     * @see Mocket#setIdentifier(java.lang.String) 
     */
    public native String getIdentifier();

    // /////////////////////////////////////////////////////////////////////////
    private long _mocket;
    private int _remotePort;
    private String _remoteAddress;
    
    // /////////////////////////////////////////////////////////////////////////
    static {
        System.loadLibrary ("mocketsjavawrapper");
    }
}


