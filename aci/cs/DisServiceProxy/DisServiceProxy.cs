/*
 * DisServiceProxy
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
 * @version    $Revision: 1.36 $            
 *             $Date: 2014/11/07 03:07:18 $
 */

using System;
using System.Net;
using System.Net.Sockets;
using System.IO;

using us.ihmc.util;
using System.Threading;

namespace us.ihmc.aci.DisService
{
    public class BaseDataArrivedCallBackArgs
    {
        public BaseDataArrivedCallBackArgs (string msgId, string sender, string groupName, uint ui32SeqNum, string objectId,
		                                    string instanceId, string mimeType, byte[] data, ushort ui16Tag,
                                            byte ui8Priority, string queryId)
        {
            this.msgId = msgId;
            this.sender = sender;
            this.groupName = groupName;
			this.objectId = objectId;
			this.instanceId = instanceId;
			this.mimeType = mimeType;
            this.ui32SeqNum = ui32SeqNum;
            this.data = data;
            this.ui16Tag = ui16Tag;
            this.ui8Priority = ui8Priority;
            this.queryId = queryId;
        }

        public readonly string msgId;
        public readonly string sender;
        public readonly string groupName;
        public readonly uint ui32SeqNum;
        public readonly string objectId;
        public readonly string instanceId;
        public readonly string mimeType;
        public readonly byte[] data;
        public readonly ushort ui16Tag;
        public readonly byte ui8Priority;
        public readonly String queryId;
    }

    public class MetadataArrivedCallbackArgs : BaseDataArrivedCallBackArgs
    {
        public MetadataArrivedCallbackArgs(string msgId, string sender, string groupName, uint ui32SeqNum, string objectId,
		                                   string instanceId, string mimeType, byte[] metadata, bool bDataChunked,
                                           ushort ui16Tag, byte ui8Priority, string queryId)
            : base(msgId, sender, groupName, ui32SeqNum, objectId, instanceId, mimeType, metadata,
			        ui16Tag, ui8Priority, queryId)
        {
            this.bDataChunked = bDataChunked;
        }

        public readonly bool bDataChunked;
    }

    public class DataArrivedCallbackArgs : BaseDataArrivedCallBackArgs
    {
        public DataArrivedCallbackArgs(string msgId, string sender, string groupName, uint ui32SeqNum, string objectId,
		                               string instanceId, string mimeType, byte[] data, uint metadataLength,
		                               ushort ui16Tag, byte ui8Priority, string queryId)
            : base(msgId, sender, groupName, ui32SeqNum, objectId, instanceId, mimeType,
			        data, ui16Tag, ui8Priority, queryId)
        {
            this.metadataLength = (uint) metadataLength;
        }

        public readonly uint metadataLength;
    }

    public class DataAvailableCallbackArgs : BaseDataArrivedCallBackArgs
    {
        public DataAvailableCallbackArgs(string msgId, string sender, string groupName, uint ui32SeqNum, string objectId,
		                                 string instanceId, string mimeType, string refObjId, byte[] data,
		                                 ushort ui16Tag, byte ui8Priority, string queryId)
            : base(msgId, sender, groupName, ui32SeqNum, objectId, instanceId, mimeType, data,
			        ui16Tag, ui8Priority, queryId)
        {
            this.refObjId = refObjId;
        }

        public readonly string refObjId;
    }

    public class ChunkArrivedCallbackArgs : BaseDataArrivedCallBackArgs
    {
        public ChunkArrivedCallbackArgs(string msgId, string sender, string groupName, uint ui32SeqNum, string objectId,
		                                string instanceId, string mimeType, byte[] data, byte ui8NChunks,
		                                byte ui8TotNChunks, string chunkedMsgId,ushort ui16Tag,
                                        byte ui8Priority, string queryId)
            : base(msgId, sender, groupName, ui32SeqNum, objectId, instanceId, mimeType, data,
			       ui16Tag, ui8Priority, queryId)
        {
            this.ui8NChunks = ui8NChunks;
            this.ui8TotNChunks = ui8TotNChunks;
            this.chunkedMsgId = chunkedMsgId;
        }

        public readonly byte ui8NChunks;
        public readonly byte ui8TotNChunks;
        public readonly string chunkedMsgId;
    }

    public class SearchArrivedCallbackArgs
    {
        public SearchArrivedCallbackArgs(string searchId, string groupName, string queryType, string queryQualifiers, byte[] query)
        {
            this.searchId = searchId;
            this.groupName = groupName;
            this.queryType = queryType;
            this.queryQualifiers = queryQualifiers;
            this.query = query;
        }

        public string searchId;
        public string groupName;
        public string queryType;
        public string queryQualifiers;
        public byte[] query;
    }

    public class ConnectCallbackArgs
    {
        public ConnectCallbackArgs(bool connected)
        {
            this.connected = connected;
        }

        public bool connected;
    }

    /// <summary>
    /// Exception thrown on DisService errors.
    /// </summary>
    public class DisServiceException : Exception
    {
        public DisServiceException(string message)
            : base(message)
        {
        }
    }

    public delegate void MetadataArrivedCallBack(object sender, MetadataArrivedCallbackArgs e);
    public delegate void ChunkArrivedCallBack(object sender, ChunkArrivedCallbackArgs e);
    public delegate void DataArrivedCallback(object sender, DataArrivedCallbackArgs e);
	public delegate void DataAvailableCallback(object sender, DataAvailableCallbackArgs e);
    public delegate void SearchArrivedCallback(object sender, SearchArrivedCallbackArgs e);
    public delegate void ConnectCallback(object sender, ConnectCallbackArgs e);

    public class DisServiceProxy: IDisposable
    {
        public DisServiceProxy()
            : this(0)
        { }

        public DisServiceProxy (ushort applicationId)
            : this(applicationId, DEFAULT_HOST, DEFAULT_PORT, "dsProxy")
        { }

        public DisServiceProxy(ushort applicationId, string host, int port)
            : this(applicationId, host, port, "dsProxy")
        { }

        public DisServiceProxy(ushort applicationId, string host, int port, string nameForDebugger)
        {
            connected = false;

            if (host == null)
            {
                throw new ArgumentNullException("host");
            }
            if (port <= 0 || port > 65535) 
            {
                throw new System.ArgumentOutOfRangeException("port");
            }

            _applicationID = applicationId;

            _host = host;
            _port = port;
            _nameForDebugger = nameForDebugger;

            startConnect();
        }

        ~DisServiceProxy()
        {
            Dispose(false);
        }

        /// <summary>
        /// Cleans up the connection to the DisServiceProxyServer.
        /// </summary>
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            connectThread.Abort();
            disconnect();

            if (disposing)
            {
                _host = null;
                _port = 0;

                _callbackCommHelper.Dispose();
                _commHelper.Dispose();

                connectFailFlag.Close();
                connectFlag.Close();
            }
        }

        public void setCallbackThreadId (int cbackThreadId)
        {
            _cbackThreadId = cbackThreadId;
        }

        /// <summary>
        /// Disseminates data to nodes belonging to the specified group that are reachable.
        /// </summary>
        /// <param name="groupName">specifies the group to which this data should be disseminated.</param>
        /// <param name="metaData">any associated metadata for this data. May be null.</param>
        /// <param name="data">the data itself.</param>
        /// <param name="expiration">the expiration time for the data specified in milliseconds.
        ///       Beyond this time, the data will no longer be disseminated to any new nodes.
        ///       A value of 0 indicates there is no expiration time.</param>
        /// <param name="ui16HistoryWindow">Reserved.</param>
        /// <param name="ui16Tag">the tag value identifying the type (application defined) for the data.
        /// A value of 0 indicates there is no tag.</param>
        /// <param name="ui8Priority">the priority value for the data, with higher values indicated higher priority</param>
        /// <returns>message ID</returns>
        public string push(string groupName, string objectId, string instanceId, string mimeType, byte[] metaData, byte[] data,
                           TimeSpan expiration, ushort ui16HistoryWindow, ushort ui16Tag, byte ui8Priority)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    return doPushOrStore ("push", groupName, objectId, instanceId, mimeType, metaData, data,
                                   expiration, ui16HistoryWindow, ui16Tag, ui8Priority);
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Store data to the DisService data cache.
        /// </summary>
        /// <param name="groupName">specifies the group to which this data should be disseminated.</param>
        /// <param name="metaData">any associated metadata for this data. May be null.</param>
        /// <param name="data">the data itself.</param>
        /// <param name="expiration">the expiration time for the data specified in milliseconds.
        ///       Beyond this time, the data will no longer be disseminated to any new nodes.
        ///       A value of 0 indicates there is no expiration time.</param>
        /// <param name="ui16HistoryWindow">Reserved.</param>
        /// <param name="ui16Tag">the tag value identifying the type (application defined) for the data.
        /// A value of 0 indicates there is no tag.</param>
        /// <param name="ui8Priority">the priority value for the data, with higher values indicated higher priority</param>
        /// <returns>message ID</returns>
        public string store(string groupName, string objectId, string instanceId, string mimeType, byte[] metaData, byte[] data,
                           TimeSpan expiration, ushort ui16HistoryWindow, ushort ui16Tag, byte ui8Priority)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    return doPushOrStore("store", groupName, objectId, instanceId, mimeType, metaData, data,
                                   expiration, ui16HistoryWindow, ui16Tag, ui8Priority);
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>         
        ///Makes the specified data available and disseminates the associated metadata to nodes belonging to
        ///the specified group that are reachable. The data is stored in the network until the specified
        ///expiration time, given storage constraints. The data may be split into chunks that are distributed
        ///and replicated among several nodes.
        /// </summary>
        /// <param name="groupName">specifies the group to which this data should be disseminated.</param>
        /// <param name="metaData">any associated metadata for this data. May be null.</param>
        /// <param name="data">the data itself.</param>
        /// <param name="dataMIMEType">the MIME type of the data being made available.</param>
        /// <param name="expiration">the expiration time for the data specified in milliseconds.
        /// Beyond this time, the data will no longer be disseminated to any new nodes.
        /// A value of 0 indicates there is no expiration time. The expiration time applies to both the
        /// metadata and the data itself.</param>
        /// <param name="ui16HistoryWindow">Reserved.</param>
        /// <param name="ui16Tag">the tag value identifying the type (application defined) for the data.
        /// A value of 0 indicates there is no tag.</param>
        /// <param name="ui8Priority">the priority value for the data, with higher values indicated higher priority</param>
        /// <returns>message ID</returns>
        public string makeAvailable(string groupName, string objectId, string instanceId, byte[] metadata,
		                            byte[] data, string dataMIMEType, TimeSpan expiration, ushort ui16HistoryWindow,
		                            ushort ui16Tag, byte ui8Priority)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    return doMakeAvailable(groupName, objectId, instanceId, metadata, data, dataMIMEType,
					                       expiration, ui16HistoryWindow, ui16Tag, ui8Priority);
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Cancel a data message that has been pushed or made available in the past
        /// </summary>
        /// <param name="id">the identifier of the message to cancel</param>
        public void cancel(string id)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    doCancel(id);
                    return;
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Cancel all messages tagged with the specified tag
        /// </summary>
        /// <param name="ui16Tag">the tag marking the set of messages that should be cancelled.</param>
        public void cancel(ushort ui16Tag)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    doCancel(ui16Tag);
                    return;
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Filter (i.e., prevent delivery) of any incoming messages that match the specified tag.
        /// </summary>
        /// <param name="groupName">the name - may be null if the filter should apply to all subscribed groups.</param>
        /// <param name="ui16Tag">the tag that identifies messages that should be filtered.
        /// NOTE: The tag cannot be 0 (which is used to indicate that there is no tag)</param>
        /// <returns>true if successful</returns>
        public bool addFilter(string groupName, ushort ui16Tag)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    return doAddFilter(groupName, ui16Tag);
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Remove a previously specified filter.
        /// </summary>
        /// <param name="groupName">the name - may be null if the filter was for all subscribed groups.</param>
        /// <param name="ui16Tag">the tag that was specified earlier as part of a filter.
        /// NOTE: The tag cannot be 0 (which is used to indicate that there is no tag)</param>
        /// <returns></returns>
        public bool removeFilter(string groupName, ushort ui16Tag)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    return doRemoveFilter(groupName, ui16Tag);
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Retrieve data that is identified by the specified id. The data must have been made available
        /// by some (other) node and the local node must have received the metadata associated with the data.
        /// The retrieved data is stored in the specified buffer.
        /// </summary>
        /// <param name="id">the id of the message whose data is to be retrieved.</param>
        /// <param name="buff">the buffer into which the data should be stored. If this is null
        /// a buffer of the proper size will be allocated.</param>
        /// <param name="timeout">the maximum time the data must be retrieved.</param>
        /// <returns>Returns a positive value with the size of the data that has been retrieved or
        /// a negative value in case of error.</returns>
        public int retrieve(string id, ref byte[] buff, TimeSpan timeout)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    return doRetrieve(id, ref buff, timeout);
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Retrieve data that is identified by the specified id. The data must have been made available
        /// by some (other) node and the local node must have received the metadata associated with the data.
        /// The data is stored in a file. The file is written by the server process, so the server
        /// must have permission to write to the file, and if the client & server are on different hosts
        /// the file will be written on the server host.
        /// </summary>
        /// <param name="id">the id of the message whose data is to be retrieved.</param>
        /// <param name="filePath">the path to the file that should be created in order to store the data being retrieved.
        /// If the file exists, the contents will be overwritten.</param>
        /// <returns></returns>
        public int retrieve(string id, string filePath)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    return doRetrieve(id, filePath);
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Requests the retrieval of previosuly-sent messages from the local cache and/or other 
        /// nodes on the network.
        /// </summary>
        /// <param name="groupName">Group name to request messages for.</param>
        /// <param name="ui16Tag">Tag of messages to request.</param>
        /// <param name="ui16HistoryLength">The number of messages to request.</param>
        /// <param name="i64RequestTimeout">the maximum length of time, in milliseconds, for which the request operation will be active</param>
        /// <returns></returns>
        public int request(string groupName, ushort ui16Tag, ushort ui16HistoryLength, ulong i64RequestTimeout)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    return doRequest(groupName, ui16Tag, ui16HistoryLength, i64RequestTimeout);
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Subscribe to the specified group. The application will subsequently receive any messages
        /// addressed to matching groups that are received.
        /// </summary>
        /// <param name="groupName">the name of the group to subscribe / join</param>
        /// <param name="ui8Priority"></param>
        /// <param name="reliable"></param>
        /// <param name="sequenced"></param>
        /// <returns>true if successful</returns>
        public bool subscribe(string groupName, byte ui8Priority, bool groupReliable, bool msgReliable, bool sequenced)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    return doSubscribe(groupName, ui8Priority, groupReliable, msgReliable, sequenced);
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Subscribe to the specified group and the specified tag. Only messages addressed to that group
        /// and that have the specified tag will be delivered.
        /// </summary>
        /// <param name="groupName">the name of the group to subscribe / join.</param>
        /// <param name="ui16Tag">the tag specifiying data the node wants to receive</param>
        /// <param name="ui8Priority"></param>
        /// <param name="reliable"></param>
        /// <param name="sequenced"></param>
        /// <returns>true if successful</returns>
        public bool subscribe(string groupName, ushort ui16Tag, byte ui8Priority, bool groupReliable, bool msgReliable, bool sequenced)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    return doSubscribe(groupName, ui16Tag, ui8Priority, groupReliable, msgReliable, sequenced);
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Unsubscribe to the specified group. 
        /// </summary>
        /// <param name="groupName">the name of the group to unsubscribe</param>
        /// <returns>true if successful</returns>
        public bool unsubscribe(string groupName)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    return doUnsubscribe(groupName);
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Subscribe to the specified group and the specified tag. Only messages addressed to that group
        /// and that have the specified tag will be delivered.
        /// </summary>
        /// <param name="groupName">the name of the group to subscribe / join</param>
        /// <param name="ui16Tag">the tag specifiying data the node wants to receive</param>
        /// <returns>true if successful</returns>
        public bool unsubscribe(string groupName, ushort ui16Tag)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    return doUnsubscribe(groupName, ui16Tag);
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        public string getDisServiceId(string objectId, string instanceId)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    lock (bigDisServiceLock)
                    {
                        throwIfInCallbackHandler();
                        _commHelper.sendLine("getDisServiceId");
                        _commHelper.sendStringBlock(objectId);
                        _commHelper.sendStringBlock(instanceId);

                        string response = _commHelper.receiveLine(_timeout);
                        if (response == "ERROR")
                        {
                            throw new DisServiceException("remote error.");
                        }

                        System.Collections.Generic.LinkedList<string> list = new System.Collections.Generic.LinkedList<string>();
                        byte[] b;
                        while ((b = _commHelper.receiveBlock()) != null)
                        {
                            string id = (new System.Text.ASCIIEncoding()).GetString(b);
                            list.AddFirst(id);
                        }

                        response = _commHelper.receiveLine(_timeout);
                        if (response == "ERROR")
                        {
                            throw new DisServiceException("remote error.");
                        }
                        return (list.Count == 0 ? null : list.First.Value);
                    }
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Returns the id that will be assigned to the next message that will be pushed under the specified group name.
        /// </summary>
        /// <param name="groupName">the name of the group that will be used for the push</param>
        /// <returns>the id of the message if successful</returns>
        public string getNextPushId (string groupName)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    return doGetNextPushId (groupName);
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        // metadata fields. The method returns the query ID, or null in case
        /// of error.
        /// </summary>
        /// <param name="pszGroupName"> a group name for the search
        /// <param name="pszQueryType"> the type of the query. It is used to identify
        ///                the proper search controller to handle the query
        /// <param name="pszQueryQualifiers">
        /// <param name="pQuery"> the query itself
        /// <param name="uiQueryLen"> the length of the query
        /// <param name="ppszQueryId"> a unique identifier that is generated for the query.
        ///
        public String search (string groupName, string queryType, string queryQualifiers, byte[] query)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            if ((groupName == null) || (groupName.Length == 0))
            {
                throw new ArgumentException("groupName cannot be null or the empty string");
            }
            if ((queryType == null) || (queryType.Length == 0))
            {
                throw new ArgumentException("queryType cannot be null or the empty string");
            }
            if (query == null)
            {
                throw new ArgumentNullException("query parameter cannot be null");
            }

            while (true)
            {
                try
                {
                    connectFlag.WaitOne();
                    lock (bigDisServiceLock)
                    {
                        _commHelper.sendLine("search");
                        _commHelper.sendStringBlock(groupName);
                        _commHelper.sendStringBlock(queryType);
                        _commHelper.sendStringBlock(queryQualifiers);
                        _commHelper.write32((uint)query.Length);
                        _commHelper.sendBlob(query);

                        string response = _commHelper.receiveLine(_timeout);
                        if (response == "ERROR")
                        {
                            throw new DisServiceException("remote error.");
                        }

                        // read the ID assigned to the query
                        uint ui32Len = _commHelper.read32();
                        string queryId = null;
                        if (ui32Len > 0)
                        {
                            byte[] buf = _commHelper.receiveBlob((int)ui32Len);
                            queryId = (new System.Text.ASCIIEncoding()).GetString(buf);
                            return queryId;
                        }
                    }
                }
                catch (CommException)
                {
                    reconnect();
                }
            }
        }

        /// <summary>
        /// Method to return the list of IDs that match the query identified
        /// by pszQueryId.
        /// </summary>
        /// <param name="queryId"> the id of the matched query
        /// <param name="disServiceMsgIds"> the list of matching messages IDs
        public void replyToQuery(string queryId, System.Collections.ICollection disServiceMsgIds)
        {
            if (_host == null)
                throw new ObjectDisposedException("DisServiceProxy");

            if (queryId == null || disServiceMsgIds == null)
            {
            }

            try
            {
                 connectFlag.WaitOne();
                 lock (bigDisServiceLock)
                 {
                    // Write query id
                    _commHelper.sendLine("replyToQuery");
                    _commHelper.sendStringBlock(queryId);

                    // Write number of elements in disServiceMsgIds
                    _commHelper.write32((uint)disServiceMsgIds.Count);

                    // Write Ids
                    foreach (string msgId in disServiceMsgIds)
                    {
                        _commHelper.sendStringBlock(msgId);
                    }

                    string response = _commHelper.receiveLine(_timeout);
                    if (response == "ERROR")
                    {
                        throw new DisServiceException("remote error.");
                    }
                }
            }
            catch (Exception e)
            {
                reconnect();
            }
        }

        /// <summary>
        /// Event called when a message is received.
        /// </summary>
        public event DataArrivedCallback DataArrived
        {
            add
            {
                if (_host == null)
                    throw new ObjectDisposedException("DisServiceProxy");

                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }

                lock (bigDisServiceLock)
                {
                    _dataArrivedCallbackDelegates += value;
                }

                if (connected && !dataCallbackRegistered)
                    doRegisterDataCallback();

            }
            remove
            {
                _dataArrivedCallbackDelegates -= value;
            }
        }

		/// <summary>
        /// Event called when chunk is received.
        /// </summary>
        public event ChunkArrivedCallBack ChunkArrived
        {
            add
            {
                if (_host == null)
                    throw new ObjectDisposedException("DisServiceProxy");

                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }

                lock (bigDisServiceLock)
                {
                    _chunkArrivedCallbackDelegates += value;
                }

                if (connected && !chunkCallbackRegistered)
                    doRegisterChunkCallback();
            }
            remove
            {
                _chunkArrivedCallbackDelegates -= value;
            }
		}

        /// <summary>
        /// Event called when metadata is received.
        /// </summary>
        public event MetadataArrivedCallBack MetadataArrived
        {
            add
            {
                if (_host == null)
                    throw new ObjectDisposedException("DisServiceProxy");

                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }

                lock (bigDisServiceLock)
                {
                    _metadataArrivedCallbackDelegates += value;
                }

                if (connected && !metadataCallbackRegistered)
                    doRegisterMetadataCallback();
            }
            remove
            {
                _metadataArrivedCallbackDelegates -= value;
            }
        }

        /// <summary>
        /// Event called when data is available for retrieval.
        /// </summary>
        public event DataAvailableCallback DataAvailable
        {
            add
            {
                if (_host == null)
                    throw new ObjectDisposedException("DisServiceProxy");

                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }

                lock (bigDisServiceLock)
                {
                    _dataAvailableCallbackDelegates += value;
                }

                if (connected && !dataAvailableCallbackRegistered)
                    doRegisterDataAvailableCallback();
            }
            remove
            {
                _dataAvailableCallbackDelegates -= value;
            }
        }

        /// <summary>
        /// Event called when data is available for retrieval.
        /// </summary>
        public event SearchArrivedCallback SearchArrived
        {
            add
            {
                if (_host == null)
                    throw new ObjectDisposedException("DisServiceProxy");

                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }

                lock (bigDisServiceLock)
                {
                    _searchArrivedCallbackDelegates += value;
                }

                if (connected && !searchArrivedCallbackRegistered)
                    doRegisterSearchArrivedCallback();
            }
            remove
            {
                _searchArrivedCallbackDelegates -= value;
            }
        }

        /// <summary>
        /// Called when the proxy client connects or disconnects from the proxy server.
        /// If the client is already connected and a new handler is registered here,
        /// the handler will be called immediately.
        /// </summary>
        public event ConnectCallback ServerConnect
        {
            add
            {
                if (_host == null)
                    throw new ObjectDisposedException("DisServiceProxy");

                _connectCallbackDelegates += value;

                // if we are already connected, call the handler that was just registered
                if (connected)
                    value(this, new ConnectCallbackArgs(true));
            }
            remove
            {
                _connectCallbackDelegates -= value;
            }
        }

        public event ConnectCallback ServerDisconnect
        {
            add
            {
                if (_host == null)
                    throw new ObjectDisposedException("DisServiceProxy");

                _disconnectCallbackDelegates += value;
            }
            remove
            {
                _disconnectCallbackDelegates -= value;
            }
        }

        public ushort getApplicationID()
        {
            return _applicationID;
        }

        /// <summary>
        /// Sets the timeout used when waiting for a command acknowledgement from the
        /// DisService server. Used to help detect deadlocks.
        /// </summary>
        public int Timeout
        {
            get { return _timeout; }
            set { _timeout = value; }
        }

        // ////////////////////////////////////////////////////////////////////////////
        // PRIVATE METHODS ////////////////////////////////////////////////////////////
        // ////////////////////////////////////////////////////////////////////////////

        private static object bigDisServiceLock = new object();

        /// <summary>
        /// should throw exceptions on error
        /// </summary>
        /// <returns>ID of the message</returns>
        /// 
        private string doPushOrStore(string operation, string groupName, string objectId, string instanceId, string mimeType, byte[] metadata, byte[] data, TimeSpan expiration, ushort ui16HistoryWindow, ushort ui16Tag, byte ui8Priority)
        {
            if ((groupName == null) || (groupName.Length == 0))
            {
                throw new ArgumentException("groupName cannot be null or the empty string");
            }
            if (data == null)
            {
                throw new ArgumentNullException("data parameter cannot be null");
            }
            string msgID = "";
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine(operation);
                _commHelper.sendLine(groupName);
                _commHelper.sendStringBlock(objectId);
                _commHelper.sendStringBlock(instanceId);
                _commHelper.sendStringBlock(mimeType);
                if (metadata != null) {
                    _commHelper.write32((uint)metadata.Length);
                    _commHelper.sendBlob(metadata);
                }
                else {
                    _commHelper.write32((uint)0);
                }
                _commHelper.write32((uint)data.Length);
                _commHelper.sendBlob(data);
                _commHelper.write64((ulong)expiration.TotalMilliseconds);
                _commHelper.write16(ui16HistoryWindow);
                _commHelper.write16(ui16Tag);
                _commHelper.write8(ui8Priority);

                string response = _commHelper.receiveLine(_timeout);

                if (response == "ERROR")
                {
                    throw new DisServiceException("remote error.");
                }
                //read the ID
                msgID = _commHelper.receiveLine();
            }

            return msgID;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="groupName"></param>
        /// <param name="metadata"></param>
        /// <param name="data"></param>
        /// <param name="ui32Expiration"></param>
        /// <param name="reliable"></param>
        /// <param name="sequenced"></param>
        /// <param name="ui16Tag"></param>
        /// <param name="ui8Priority"></param>
        /// <returns>ID of message</returns>
        /// 
        private string doMakeAvailable(string groupName, String objectId, String instanceId, byte[] metadata, byte[] data,
		                               string dataMIMEType, TimeSpan expiration, ushort ui16HistoryWindow, ushort ui16Tag,
		                               byte ui8Priority)
        {
            if ((groupName == null) || (groupName.Length == 0))
            {
                throw new ArgumentException("groupName cannot be null or the empty string");
            }
            if (data == null)
            {
                throw new ArgumentNullException("data parameter cannot be null");
            }

            string msgId = "";

            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("makeAvailable");
                _commHelper.sendLine(groupName);
                _commHelper.sendStringBlock(objectId);
				_commHelper.sendStringBlock(instanceId);

                if (metadata != null)
                {
                    _commHelper.write32((uint)metadata.Length);
                    _commHelper.sendBlob(metadata);
                }
                else
                {
                    _commHelper.write32((uint)0);
                }
                _commHelper.write32((uint)data.Length);
                _commHelper.sendBlob(data);
                if ((dataMIMEType == null) || (dataMIMEType.Length == 0))
                {
                    _commHelper.write32(0);
                }
                else
                {
                    byte[] buf = System.Text.Encoding.ASCII.GetBytes(dataMIMEType);
                    _commHelper.write32 ((uint)buf.Length);
                    _commHelper.sendBlob(buf);
                }
                _commHelper.write64((ulong)expiration.TotalMilliseconds);
                _commHelper.write16(ui16HistoryWindow);
                _commHelper.write16(ui16Tag);
                _commHelper.write8(ui8Priority);

                string response = _commHelper.receiveLine(_timeout);
                if (response == "ERROR")
                {
                    throw new DisServiceException("ERROR");
                }

                //read the ID
                msgId = _commHelper.receiveLine();
            }

            return msgId;
        }

        /*
        private string doMakeAvailable (string groupName, byte[] metadata, string filePath, TimeSpan expiration, bool reliable, bool sequenced, ushort ui16Tag, byte ui8Priority)
        {
            String msgId;

            lock (bigDisServiceLock)
            {
                _commHelper.sendLine("makeAvailable_file");
                _commHelper.sendLine(groupName);
                _commHelper.write32((uint)metadata.Length);
                _commHelper.sendBlob(metadata);

                // This won't work if Dissemination Service is on a different computer.
                // File will need to be read in and sent over the connection

                FileInfo fileInfo = new FileInfo(filePath);

                long fileSize = fileInfo.Length;
                
                // send the file size 
                _commHelper.write32((uint)fileSize);

                // send the file contents.
                if (fileSize > 0)
                {
                    FileStream fs = fileInfo.OpenRead();
                    BinaryReader br = new BinaryReader(fs);
                    byte[] buff = new byte[2048];
                    int totalRead = 0;

                    while (totalRead < fileSize) {
                        int read = br.Read(buff, 0, (int)Math.Min(fileSize - totalRead, buff.Length));;
                        if (read == 0) {
                            //TODO:: end of stream reached. we need to do something here!!!
                        }

                        _commHelper.sendBlob(buff, 0, read);
                    }

                    fs.Close();
                }
                
                _commHelper.write32((uint)expiration.TotalMilliseconds);

                byte aux = (byte)(reliable ? 1 : 0);
                _commHelper.write8(aux);

                aux = (byte)(sequenced ? 1 : 0);
                _commHelper.write8(aux);

                _commHelper.write16(ui16Tag);
                _commHelper.write8(ui8Priority);

                string response = _commHelper.receiveLine();
                if (response == "ERROR")
                {
                    throw new ApplicationException("ERROR");
                }

                //read the ID
                msgId = _commHelper.receiveLine();
            }

            return msgId;
        }
        */

        private void doCancel(string id)
        {
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("cancel_str");
                _commHelper.sendLine(id);

                string response = _commHelper.receiveLine(_timeout);
                if (response.StartsWith("ERROR"))
                {
                    throw new DisServiceException("ERROR");
                }
            }
        }

        private void doCancel(ushort ui16Tag)
        {
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("cancel_int");
                _commHelper.write16(ui16Tag);

                string response = _commHelper.receiveLine(_timeout);
                if (response.StartsWith("ERROR"))
                {
                    throw new DisServiceException("ERROR");
                }
            }
        }

        private bool doAddFilter(string groupName, ushort ui16Tag)
        {
            if ((groupName == null) || (groupName.Length == 0))
            {
                throw new ArgumentException("groupName cannot be null or the empty string");
            }
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("addFilter");
                _commHelper.sendLine(groupName);
                _commHelper.write16(ui16Tag);

                string response = _commHelper.receiveLine(_timeout);
                if (response.StartsWith("ERROR"))
                {
                    return false;
                }
            }

            return true;
        }

        private bool doRemoveFilter(string groupName, ushort ui16Tag)
        {
            if ((groupName == null) || (groupName.Length == 0))
            {
                throw new ArgumentException("groupName cannot be null or the empty string");
            }
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("removeFilter");
                _commHelper.sendLine(groupName);
                _commHelper.write16(ui16Tag);

                string response = _commHelper.receiveLine(_timeout);
                if (response.StartsWith("ERROR"))
                {
                    return false;
                }
            }

            return true;
        }

        private int doRetrieve(string id, ref byte[] buff, TimeSpan timeout)
        {
            if ((id == null) || (id.Length == 0))
            {
                throw new ArgumentException("id cannot be null or the empty string");
            }

            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("retrieve");
                _commHelper.sendLine(id);
                _commHelper.write64((ulong)timeout.TotalMilliseconds);

                string response = _commHelper.receiveLine(_timeout);
                if (response.StartsWith("OK"))
                {
                    uint size = _commHelper.read32();

                    if (size > 0)
                        buff = _commHelper.receiveBlob((int)size);

                    response = _commHelper.receiveLine();
                    if (response.StartsWith("OK"))
                    {
                        return (int)size;
                    }
                }
            }

            return -1;
        }

        private int doRetrieve(string id, string filePath)
        {
            if ((id == null) || (id.Length == 0))
            {
                throw new ArgumentException("id cannot be null or the empty string");
            }
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("retrieve_file");
                _commHelper.sendLine(id);
                _commHelper.sendLine(filePath);

                string response = _commHelper.receiveLine(_timeout);
                if (response.StartsWith("OK"))
                {
                    String[] tokens = response.Split(new char[] { ' ' });
                    int size = int.Parse(tokens[1]);

                    return size;
                }
            }

            return -1;
        }

        private int doRequest(string groupName, ushort ui16Tag, ushort ui16HistoryLength, ulong i64RequestTimeout)
        {
            if ((groupName == null) || (groupName.Length == 0))
            {
                throw new ArgumentException("groupName cannot be null or the empty string");
            }
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("request");
                _commHelper.sendLine(groupName);
                _commHelper.write16(ui16Tag);
                _commHelper.write16(ui16HistoryLength);
                _commHelper.write64(i64RequestTimeout);

                string response = _commHelper.receiveLine(_timeout);
                if (response.StartsWith("OK"))
                    return 0;
                else
                    return -1;
            }
        }

        private bool doSubscribe(string groupName, byte ui8Priority, bool groupReliable, bool msgReliable, bool sequenced)
        {
            if ((groupName == null) || (groupName.Length == 0))
            {
                throw new ArgumentException("groupName cannot be null or the empty string");
            }
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("subscribe");
                _commHelper.sendLine(groupName);
                _commHelper.write8(ui8Priority);
                _commHelper.write8((byte)(groupReliable ? 1 : 0));
                _commHelper.write8((byte)(msgReliable ? 1 : 0));
                _commHelper.write8((byte)(sequenced ? 1 : 0));

                string response = _commHelper.receiveLine(_timeout);
                if (response.StartsWith("OK"))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        private bool doSubscribe(string groupName, ushort ui16Tag, byte ui8Priority, bool groupReliable, bool msgReliable, bool sequenced)
        {
            if ((groupName == null) || (groupName.Length == 0))
            {
                throw new ArgumentException("groupName cannot be null or the empty string");
            }
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("subscribe_tag");
                _commHelper.sendLine(groupName);
                _commHelper.write8(ui8Priority);
                _commHelper.write16(ui16Tag);
                _commHelper.write8((byte)(groupReliable ? 1 : 0));
                _commHelper.write8((byte)(msgReliable ? 1 : 0));
                _commHelper.write8((byte)(sequenced ? 1 : 0));

                string response = _commHelper.receiveLine(_timeout);
                if (response.StartsWith("OK"))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        private bool doUnsubscribe(string groupName)
        {
            if ((groupName == null) || (groupName.Length == 0))
            {
                throw new ArgumentException("groupName cannot be null or the empty string");
            }
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("unsubscribe");
                _commHelper.sendLine(groupName);

                string response = _commHelper.receiveLine(_timeout);
                if (response.StartsWith("OK"))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        private bool doUnsubscribe(string groupName, ushort ui16Tag)
        {
            if ((groupName == null) || (groupName.Length == 0))
            {
                throw new ArgumentException("groupName cannot be null or the empty string");
            }
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("unsubscribe_tag");
                _commHelper.sendLine(groupName);
                _commHelper.write16(ui16Tag);

                string response = _commHelper.receiveLine(_timeout);
                if (response.StartsWith("OK"))
                {
                    return true;
                }
            }

            return false;
        }

        private string doGetNextPushId(string groupName)
        {
            if ((groupName == null) || (groupName.Length == 0))
            {
                throw new ArgumentException("groupName cannot be null or the empty string");
            }
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("getNextPushId");
                _commHelper.sendLine(groupName);
                string response = _commHelper.receiveLine(_timeout);
                if (!response.StartsWith ("OK"))
                {
                    throw new DisServiceException("ERROR");
                }

                //read the ID
                string msgId = _commHelper.receiveLine();
                return msgId;
            }
        }

        private bool dataCallbackRegistered = false;
        private bool doRegisterDataCallback()
        {
            lock (bigDisServiceLock) 
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("registerDataArrivedCallback");
                dataCallbackRegistered = true;
                // server doesn't send this response
                //string response = _commHelper.receiveLine();
                //if (response.StartsWith("OK"))
                //{
                //    dataCallbackRegistered = true;
                //    return true;
                //}
            }

            return true;
        }

		private bool chunkCallbackRegistered = false;
        private bool doRegisterChunkCallback()
        {
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("registerChunkArrivedCallback");
                chunkCallbackRegistered = true;
                // not sent by server
                //string response = _commHelper.receiveLine();
                //if (response.StartsWith("OK"))
                //{
                //    chunkCallbackRegistered = true;
                //    return true;
                //}
            }

            return true;
        }

        private bool metadataCallbackRegistered = false;
        private bool doRegisterMetadataCallback()
        {
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("registerMetadataArrivedCallback");
                metadataCallbackRegistered = true;
                // not sent by server
                //string response = _commHelper.receiveLine();
                //if (response.StartsWith("OK"))
                //{
                //    metadataCallbackRegistered = true;
                //    return true;
                //}
            }

            return true;
        }

        private bool dataAvailableCallbackRegistered = false;
        private bool doRegisterDataAvailableCallback()
        {
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("registerDataAvailableCallback");
                dataAvailableCallbackRegistered = true;
                // string response = _commHelper.receiveLine();
                // if (response.StartsWith("OK"))
                // {
                //    dataAvailableCallbackRegistered = true;
                //    return true;
                // }
            }

            return false;
        }

        private bool searchArrivedCallbackRegistered = false;
        private bool doRegisterSearchArrivedCallback()
        {
            lock (bigDisServiceLock)
            {
                throwIfInCallbackHandler();

                _commHelper.sendLine("registerSearchListener");
                searchArrivedCallbackRegistered = true;
            }

            return false;
        }

        internal bool dataArrived(string msgId, string sender, string groupName, uint ui32SeqNum, string objectId, string instanceId, string mimeType,
		                          byte[] data, uint ui32MetadataLength, ushort tag, byte priority, string queryId)
        {
            if (_dataArrivedCallbackDelegates != null)
                _dataArrivedCallbackDelegates(this,
                    new DataArrivedCallbackArgs(msgId, sender, groupName, ui32SeqNum, objectId, instanceId, mimeType,
				                                data, ui32MetadataLength, tag, priority, queryId));
            return true;
        }

        internal bool chunkArrived(string msgId, string sender, string groupName, uint ui32SeqNum, string objectId, string instanceId, string mimeType,
                                   byte[] data, byte ui8NChunks, byte ui8TotNChunks, string chunkedMsgId, ushort tag, byte priority, string queryId)
        {
            if (_chunkArrivedCallbackDelegates != null)
                _chunkArrivedCallbackDelegates(this,
                    new ChunkArrivedCallbackArgs(msgId, sender, groupName, ui32SeqNum, objectId, instanceId, mimeType,
                                                 data, ui8NChunks, ui8TotNChunks, chunkedMsgId, tag, priority, queryId));
            return true;
        }

        internal bool metadataArrived(string msgId, string sender, string groupName, uint ui32SeqNum, string objectId, string instanceId, string mimeType,
                                      byte[] metadata, bool bDataChunked, ushort tag, byte priority, string queryId)
        {
            if (_metadataArrivedCallbackDelegates != null)
                _metadataArrivedCallbackDelegates(this,
                    new MetadataArrivedCallbackArgs(msgId, sender, groupName, ui32SeqNum, objectId, instanceId, mimeType,
				                                    metadata, bDataChunked, tag, priority, queryId));
            return true;
        }

        internal bool dataAvailable(string msgId, string sender, string groupName, uint ui32SeqNum, string objectId, string instanceId, string mimeType,
                                    string refObjId, byte[] metadata, ushort tag, byte priority, string queryId)
        {
            if (_dataAvailableCallbackDelegates != null)
                _dataAvailableCallbackDelegates(this,
                    new DataAvailableCallbackArgs(msgId, sender, groupName, ui32SeqNum, objectId, instanceId, mimeType,
				                                  refObjId, metadata, tag, priority, queryId));
            return true;
        }

        internal bool searchArrived(string searchId, string groupName, string queryType, string queryQualifiers, byte[] query)
        {
            if (_searchArrivedCallbackDelegates != null) {
                _searchArrivedCallbackDelegates(this,
                    new SearchArrivedCallbackArgs(searchId, groupName, queryType, queryQualifiers, query));
                /*
                IAsyncResult result = _searchArrivedCallbackDelegates.BeginInvoke(this,
                    new SearchArrivedCallbackArgs(searchId, groupName, queryType, queryQualifiers, query), null, null);

                _searchArrivedCallbackDelegates.EndInvoke(result);*/
            }
            return true;
        }

        // -------------------------------------------------------------------------------------
        // Connect/reconnect stuff

        protected System.Threading.ManualResetEvent connectFlag = new System.Threading.ManualResetEvent(false);
        public bool connected { get; protected set; } 

        internal System.Threading.AutoResetEvent connectFailFlag = new System.Threading.AutoResetEvent(false);
        private System.Threading.Thread connectThread;

        internal void reconnect()
        {
            disconnect();
            connectFailFlag.Set();
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031")]
        internal void disconnect()
        {
            connectFlag.Reset();

            try {
                if (_commHelper != null) {
                    _commHelper.closeConnection();
                    _commHelper = null;
                }
                if (_callbackCommHelper != null) {
                    _callbackCommHelper.closeConnection();
                    _callbackCommHelper = null;
                }

                _handler = null;
            }
            catch (Exception) {
            }
        }

        private void startConnect()
        {
            connectThread = new System.Threading.Thread(new System.Threading.ThreadStart(connectThreadFunc));
            connectThread.IsBackground = true;
            connectThread.Name = String.Format("dsProxy {0}", _nameForDebugger);
            connectThread.Start();
        }

        private void connectThreadFunc()
        {
            while (true)
            {
                connect();
                connectFlag.Set();
                connected = true;
                if (_connectCallbackDelegates != null)
                    _connectCallbackDelegates(this, new ConnectCallbackArgs(true));
                connectFailFlag.WaitOne();
                Console.WriteLine("[{0}] Lost connection to the DisServiceProxyServer.",
                    System.DateTime.Now.ToString("HH:mm:ss"));
                connected = false;
                if (_disconnectCallbackDelegates != null)
                    _disconnectCallbackDelegates(this, new ConnectCallbackArgs(false));
                connectFlag.Reset();
            }

        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031")]
        private bool connect()
        {
            if ((_commHelper != null) && (_callbackCommHelper != null))
            {
                return true;
            }

            if (System.Threading.Thread.CurrentThread != connectThread)
                throw new NotSupportedException("internal DisService error");

            bool errMsg = false;

            while (_commHelper == null)
            {
                try
                {
                    _commHelper = initCommHelper();
                    _callbackCommHelper = initCallbackCommHelper();
                }
                catch (Exception)
                {
                    if (!errMsg)
                        Console.WriteLine("[{0}] Can't connect to the DisServiceProxyServer.",
                            System.DateTime.Now.ToString("HH:mm:ss"));
                    errMsg = true;
                    _commHelper = null;
                    _callbackCommHelper = null;
                    System.Threading.Thread.Sleep(5000);
                }
            }

            if (errMsg)
                Console.WriteLine("[{0}] Connected to DisServiceProxyServer.",
                    System.DateTime.Now.ToString("HH:mm:ss"));

            _handler = new DisServiceProxyCallbackHandler(this, _callbackCommHelper);
            _handler.start(_nameForDebugger);

            if (_dataArrivedCallbackDelegates != null)
            {
                doRegisterDataCallback();
            }
            if (_chunkArrivedCallbackDelegates != null)
            {
                doRegisterChunkCallback();
            }
            if (_metadataArrivedCallbackDelegates != null)
            {
                doRegisterMetadataCallback();
            }
            if (_dataAvailableCallbackDelegates != null)
            {
                doRegisterDataAvailableCallback();
            }
            if (_searchArrivedCallbackDelegates != null)
            {
                doRegisterSearchArrivedCallback();
            }

            return true;
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000")]
        private CommHelper initCommHelper()
        {
            try  {
                TcpClient connToHost = new TcpClient(_host, _port);
                CommHelper ch = new CommHelper();
                if ( !ch.init(connToHost) ) {
                    throw new IOException("Error establishing the communication");
                }

                ch.sendLine("registerProxy " + _applicationID);
                string line = ch.receiveLine();
                if (line.StartsWith("OK")) {
                    string[] tokens = line.Split(new char[]{' '});
                    if (tokens.Length > 1)
                    {
                        _applicationID = (ushort)Int32.Parse(tokens[1]);
                    }

                    return ch;
                }
            }
            catch (Exception) {
                throw;
            }

            return null;
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000")]
        private CommHelper initCallbackCommHelper()
        {
            try
            {
                TcpClient connToHost = new TcpClient(_host, _port);
                CommHelper ch = new CommHelper();
                if (!ch.init(connToHost)) {
                    throw new IOException("Error establishing the communication");
                }

                ch.sendLine("registerProxyCallback " + _applicationID);
                string line = ch.receiveLine();
                if (line.StartsWith("OK")) {
                    return ch;
                }
            }
            catch (Exception) {
                throw;
            }

            return null;
        }

        private void throwIfInCallbackHandler()
        {
            if (System.Threading.Thread.CurrentThread.ManagedThreadId == _cbackThreadId) {
                throw new NotSupportedException("Cannot call DisService methods from inside a DisService callback handler.");
            }                
        }

        // //////////////////////////////////////////////////////////////////////
        public const string DEFAULT_HOST        = "localhost";
        public const int    DEFAULT_PORT        = 56487;

        public const string OK                  = "OK";
        public const string ERROR               = "ERROR";

        // //////////////////////////////////////////////////////////////////////
        private string _host;
        private int    _port;
        private string _nameForDebugger;

        private CommHelper _commHelper = null;
        private CommHelper _callbackCommHelper = null;

        private DisServiceProxyCallbackHandler _handler = null;

        private event DataArrivedCallback _dataArrivedCallbackDelegates = null;
        private event ChunkArrivedCallBack _chunkArrivedCallbackDelegates = null;
        private event MetadataArrivedCallBack _metadataArrivedCallbackDelegates = null;
        private event DataAvailableCallback _dataAvailableCallbackDelegates = null;
        private event SearchArrivedCallback _searchArrivedCallbackDelegates = null;
        private event ConnectCallback _connectCallbackDelegates = null;
        private event ConnectCallback _disconnectCallbackDelegates = null;

        private ushort _applicationID = 0;

        private int _timeout = -1; // no timeout
        private int _cbackThreadId = -1; // no id
    }
}
