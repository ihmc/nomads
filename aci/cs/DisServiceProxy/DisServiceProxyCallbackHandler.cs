/**
 * DisServiceProxyCallbackHandler
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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
 * @author     Marco Arguedas  <marguedas@ihmc.us>
 * 
 * @version    $Revision: 1.24 $            
 *             $Date: 2014/11/07 03:07:18 $
 */

using System;
using System.IO;
using System.Net.Sockets;
using System.Threading;
using us.ihmc.util;

namespace us.ihmc.aci.DisService
{
    public class DisServiceProxyCallbackHandler
    {
        public DisServiceProxyCallbackHandler(DisServiceProxy proxy, CommHelper callbackCommHelper)
        {
            if (proxy == null) {
                throw new ArgumentNullException ("proxy");
            }

            if (callbackCommHelper == null) {
                throw new ArgumentNullException("callbackCommHelper");
            }

            _proxy = proxy;
            _commHelper = callbackCommHelper;
			_asciiEnc = new System.Text.ASCIIEncoding();
        }

        public void start(string debuggerName)
        {
            _thread = new Thread (new ThreadStart(this.run));
            _thread.Name = String.Format("dsCallback {0}", debuggerName);
            _thread.IsBackground = true;
            _thread.Start();
        }
        
        public void run()
        {
            _proxy.setCallbackThreadId (System.Threading.Thread.CurrentThread.ManagedThreadId);
            while (true) {
                try
                {
                    string[] tokens = _commHelper.receiveParsed();

                    if (tokens[0] == "dataArrivedCallback")
                    {
                        doDataArrivedCallback();
                    }
                    else if (tokens[0] == "chunkArrivedCallback")
                    {
                        doChunkArrivedCallback();
                    }
                    else if (tokens[0] == "metadataArrivedCallback")
                    {
                        doMetadataArrivedCallback();
                    }
                    else if (tokens[0] == "dataAvailableCallback")
                    {
                        doDataAvailableCallback();
                    }
                    else if (tokens[0] == "searchArrivedCallback")
                    {
                        // Search arrived is asynchronous!
                        doSearchArrivedCallback();
                    }
                    else
                    {
                        Console.WriteLine("ERROR: operation [" + tokens[0] + "] unknown.");
                    }
                }
                catch (CommException)
                {
                    _proxy.reconnect();
                    break;
                }
            }
        }

        private void doDataArrivedCallback()
        {
            string sender = _commHelper.receiveLine();
            string groupname = _commHelper.receiveLine();
            uint ui32SeqNum = _commHelper.read32();

            uint ui32Len = _commHelper.read32();
			string objectId = null;
			if (ui32Len > 0) {
				byte[] buf = _commHelper.receiveBlob((int)ui32Len);
				objectId = _asciiEnc.GetString(buf);
			}

			ui32Len = _commHelper.read32();
			string instanceId = null;
			if (ui32Len > 0) {
				byte[] buf = _commHelper.receiveBlob((int)ui32Len);
				instanceId = _asciiEnc.GetString(buf);
			}

			ui32Len = _commHelper.read32();
			string mimeType = null;
			if (ui32Len > 0) {
				byte[] buf = _commHelper.receiveBlob((int)ui32Len);
				mimeType = _asciiEnc.GetString(buf);
			}

            uint ui32DataLength = _commHelper.read32();
            uint ui32MetadataLength = _commHelper.read32();
            byte[] data = _commHelper.receiveBlob((int)ui32DataLength);
            ushort ui16Tag = _commHelper.read16();
            byte priority = _commHelper.read8();
            ui32Len = _commHelper.read32();
            string queryId = null;

            if (ui32Len > 0) {
                byte[] buf = _commHelper.receiveBlob((int)ui32Len);
                queryId = _asciiEnc.GetString(buf);
            }

            _proxy.dataArrived(Utils.getMessageID(sender, groupname, ui32SeqNum), sender, groupname, ui32SeqNum, objectId, instanceId, mimeType,
			                   data, ui32MetadataLength, ui16Tag, priority, queryId);

            _commHelper.sendLine("OK");
        }

		private void doChunkArrivedCallback()
        {
            string sender = _commHelper.receiveLine();
            string groupname = _commHelper.receiveLine();
            uint ui32SeqNum = _commHelper.read32();

			uint ui32Len = _commHelper.read32();
			string objectId = null;
			if (ui32Len > 0) {
				byte[] buf = _commHelper.receiveBlob((int)ui32Len);
				objectId = _asciiEnc.GetString(buf);
			}

			ui32Len = _commHelper.read32();
			string instanceId = null;
			if (ui32Len > 0) {
				byte[] buf = _commHelper.receiveBlob((int)ui32Len);
				instanceId = _asciiEnc.GetString(buf);
			}

			ui32Len = _commHelper.read32();
			string mimeType = null;
			if (ui32Len > 0) {
				byte[] buf = _commHelper.receiveBlob((int)ui32Len);
				mimeType = _asciiEnc.GetString(buf);
			}

            uint ui32DataLength = _commHelper.read32();
            byte[] data = _commHelper.receiveBlob((int)ui32DataLength);
            byte ui8NChunks = _commHelper.read8();
            byte ui8TotNChunks = _commHelper.read8();
            string chunkedMsgId = _commHelper.receiveLine();
            ushort ui16Tag = _commHelper.read16();
            byte priority = _commHelper.read8();

            ui32Len = _commHelper.read32();
            string queryId = null;
            if (ui32Len > 0) {
                byte[] buf = _commHelper.receiveBlob((int)ui32Len);
                queryId = _asciiEnc.GetString(buf);
            }

            _proxy.chunkArrived(Utils.getChunkMessageID(sender, groupname, ui32SeqNum),
                                sender, groupname, ui32SeqNum, objectId, instanceId, mimeType,
			                    data, ui8NChunks, ui8TotNChunks, chunkedMsgId,
                                ui16Tag, priority, queryId);

            _commHelper.sendLine("OK");
        }

        private void doMetadataArrivedCallback()
        {
            string sender = _commHelper.receiveLine();
            string groupname = _commHelper.receiveLine();
            uint ui32SeqNum = _commHelper.read32();

			uint ui32Len = _commHelper.read32();
			string objectId = null;
			if (ui32Len > 0) {
				byte[] buf = _commHelper.receiveBlob((int)ui32Len);
				objectId = _asciiEnc.GetString(buf);
			}

			ui32Len = _commHelper.read32();
			string instanceId = null;
			if (ui32Len > 0) {
				byte[] buf = _commHelper.receiveBlob((int)ui32Len);
				instanceId = _asciiEnc.GetString(buf);
			}

			ui32Len = _commHelper.read32();
			string mimeType = null;
			if (ui32Len > 0) {
				byte[] buf = _commHelper.receiveBlob((int)ui32Len);
				mimeType = _asciiEnc.GetString(buf);
			}

            uint ui32MetaDataLength = _commHelper.read32();
            byte[] metadata = _commHelper.receiveBlob((int)ui32MetaDataLength);
            byte tmp = _commHelper.read8();
            bool bDataChunked = (tmp == 1);
            ushort ui16Tag = _commHelper.read16();
            byte priority = _commHelper.read8();

            ui32Len = _commHelper.read32();
            string queryId = null;
            if (ui32Len > 0) {
                byte[] buf = _commHelper.receiveBlob((int)ui32Len);
                queryId = _asciiEnc.GetString(buf);
            }

            _proxy.metadataArrived (Utils.getMessageID(sender, groupname, ui32SeqNum),
                                    sender, groupname, ui32SeqNum, objectId, instanceId, mimeType,
			                        metadata, bDataChunked, ui16Tag, priority, queryId);

            _commHelper.sendLine("OK");
        }

        private void doDataAvailableCallback()
        {
            string sender = _commHelper.receiveLine();
            string groupname = _commHelper.receiveLine();
            uint ui32SeqNum = _commHelper.read32();

			uint ui32Len = _commHelper.read32();
			string objectId = null;
			if (ui32Len > 0) {
				byte[] buf = _commHelper.receiveBlob((int)ui32Len);
				objectId = _asciiEnc.GetString(buf);
			}

			ui32Len = _commHelper.read32();
			string instanceId = null;
			if (ui32Len > 0) {
				byte[] buf = _commHelper.receiveBlob((int)ui32Len);
				instanceId = _asciiEnc.GetString(buf);
			}

			ui32Len = _commHelper.read32();
			string mimeType = null;
			if (ui32Len > 0) {
				byte[] buf = _commHelper.receiveBlob((int)ui32Len);
				mimeType = _asciiEnc.GetString(buf);
			}

            uint ui32IdLen = _commHelper.read32();
            string id = null;
            if (ui32IdLen > 0)
            {
                byte[] idBytes = _commHelper.receiveBlob((int)ui32IdLen);
                id = System.Text.Encoding.ASCII.GetString(idBytes);
            }
            uint ui32MetaDataLength = _commHelper.read32();
            byte[] metadata = _commHelper.receiveBlob((int)ui32MetaDataLength);
            ushort ui16Tag = _commHelper.read16();
            byte priority = _commHelper.read8();

            ui32Len = _commHelper.read32();
            string queryId = null;
            if (ui32Len > 0) {
                byte[] buf = _commHelper.receiveBlob((int)ui32Len);
                queryId = _asciiEnc.GetString(buf);
            }

            _proxy.dataAvailable(Utils.getMessageID(sender, groupname, ui32SeqNum),
                                 sender, groupname, ui32SeqNum, objectId, instanceId, mimeType,
                                 id, metadata, ui16Tag, priority, queryId);

            _commHelper.sendLine("OK");
        }

        private void doSearchArrivedCallback ()
        {
            uint ui32Len = _commHelper.read32();
            string searchId = null;
            if (ui32Len > 0)
            {
                byte[] buf = _commHelper.receiveBlob((int)ui32Len);
                searchId = _asciiEnc.GetString(buf);
            }

            ui32Len = _commHelper.read32();
            string groupName = null;
            if (ui32Len > 0)
            {
                byte[] buf = _commHelper.receiveBlob((int)ui32Len);
                groupName = _asciiEnc.GetString(buf);
            }

            ui32Len = _commHelper.read32();
            string querier = null;
            if (ui32Len > 0)
            {
                byte[] buf = _commHelper.receiveBlob((int)ui32Len);
                querier = _asciiEnc.GetString(buf);
            }

            ui32Len = _commHelper.read32();
            string queryType = null;
            if (ui32Len > 0)
            {
                byte[] buf = _commHelper.receiveBlob((int)ui32Len);
                queryType = _asciiEnc.GetString(buf);
            }

            ui32Len = _commHelper.read32();
            string queryQualifiers = null;
            if (ui32Len > 0)
            {
                byte[] buf = _commHelper.receiveBlob((int)ui32Len);
                queryQualifiers = _asciiEnc.GetString(buf);
            }

            byte[] query = null;
            ui32Len = _commHelper.read32();
            if (ui32Len > 0) {
                query = _commHelper.receiveBlob((int)ui32Len);
            }

            _proxy.searchArrived (searchId, groupName, queryType, queryQualifiers, query);
            _commHelper.sendLine ("OK");
        }

        // ////////////////////////////////////////////////////////////////////////////
        private CommHelper _commHelper;
        private Thread _thread;
        private DisServiceProxy _proxy;
		private System.Text.ASCIIEncoding _asciiEnc;
    }
}
