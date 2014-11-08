// ManagedMocketStats.cs
// P/Invoke wrapper for C++ mockets library
// jk, 11/2008

using System;
using System.Runtime.InteropServices;

namespace us.ihmc.mockets
{
    /// <summary>
    /// Provides an interface for applications to get traffic counts and other statistics related to a ManagedMocket.
    /// To obtain an instance of this class for a particular mocket, call Mocket.getStatistics().
    /// </summary>
    public class ManagedMocketStats
    {
        /// <summary>
        /// Constructor creates a ManagedMocketStats wrapper around an unmanaged
        /// MocketStatsCtx pointer.
        /// </summary>
        /// <param name="mock">The mocket that is associated with the statistics object.</param>
        /// <param name="ctx">MocketStatsCtx pointer returned from MocketGetStatistics().</param>
        internal ManagedMocketStats(ManagedMocket mock, IntPtr ctx)
        {
            this.mocket = mock;
            this.statctx = ctx;
        }

        /// <summary>
        /// Returns the number of retransmitted packets.
        /// </summary>
        /// <returns>The number of retransmitted packets.</returns>
        public UInt32 getRetransmitCount()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetRetransmitCount(statctx);
        }

        /// <summary>
        /// Returns the number of sent packets.
        /// </summary>
        /// <returns>The number of sent packets.</returns>
        public UInt32 getSentPacketCount()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetSentPacketCount(statctx);
        }

        /// <summary>
        /// Returns the number of bytes transmitted. 
        /// </summary>
        /// <returns>The number of bytes transmitted.</returns>
        public UInt32 getSentByteCount()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetSentByteCount(statctx);
        }

        /// <summary>
        /// Returns the number of packets received.
        /// </summary>
        /// <returns>The number of packets received.</returns>
        public UInt32 getReceivedPacketCount()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetReceivedPacketCount(statctx);
        }

        /// <summary>
        /// Returns the number of bytes received.
        /// </summary>
        /// <returns>The number of bytes received.</returns>
        public UInt32 getReceivedByteCount()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetReceivedByteCount(statctx);
        }

        /// <summary>
        /// Returns the number of incoming packets that were discarded because they were duplicates.
        /// </summary>
        /// <returns>The number of incoming packets that were discarded because they were duplicates.</returns>
        public UInt32 getDuplicatedDiscardedPacketCount()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetDuplicatedDiscardedPacketCount(statctx);
        }

        /// <summary>
        /// Returns the number of incoming packets that were discarded because there was no room to buffer them. 
        /// </summary>
        /// <returns>The number of incoming packets that were discarded because there was no room to buffer them.</returns>
        public UInt32 getNoRoomDiscardedPacketCount()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetNoRoomDiscardedPacketCount(statctx);
        }

        /// <summary>
        /// Returns the number of incoming packets that are discarded because a message was not reassembled.
        /// This occurs when reassembly of a message from packet fragments is abandoned due to a timeout
        /// The packets discarded are the fragments of the message that were received.
        /// </summary>
        /// <returns>The number of incoming packets that are discarded because a message was not reassembled.</returns>
        public UInt32 getReassemblySkippedDiscardedPacketCount()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetReassemblySkippedDiscardedPacketCount(statctx);
        }

        /// <summary>
        /// Returns the estimated round-trip-time in milliseconds.
        /// </summary>
        /// <returns>The estimated round-trip-time in milliseconds.</returns>
        public float getEstimatedRTT()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetEstimatedRTT(statctx);
        }

        /// <summary>
        /// Returns the size (in bytes) of the data that is enqueued in the pending packet queue awaiting transmission.
        /// </summary>
        /// <returns>The size (in bytes) of the data that is enqueued in the pending packet queue awaiting transmission.</returns>
        public UInt32 getPendingDataSize()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetPendingDataSize(statctx);
        }

        /// <summary>
        /// Returns the number of packets in the pending packet queue awaiting retransmission.
        /// </summary>
        /// <returns>The number of packets in the pending packet queue awaiting retransmission.</returns>
        public UInt32 getPendingPacketQueueSize()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetPendingPacketQueueSize(statctx);
        }

        /// <summary>
        /// Returns the size (in bytes) of the data that is in the reliable, sequenced packet queue awaiting acknowledgement.
        /// </summary>
        /// <returns>The size (in bytes) of the data that is in the reliable, sequenced packet queue awaiting acknowledgement.</returns>
        public UInt32 getReliableSequencedDataSize()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetReliableSequencedDataSize(statctx);
        }

        /// <summary>
        /// Returns the number of packets in the reliable, sequenced packet queue awaiting acknowledgement.
        /// </summary>
        /// <returns>The number of packets in the reliable, sequenced packet queue awaiting acknowledgement.</returns>
        public UInt32 getReliableSequencedPacketQueueSize()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetReliableSequencedPacketQueueSize(statctx);
        }

        /// <summary>
        /// Returns the size (in bytes) of the data that is in the reliable, unsequenced packet queue awaiting acknowledgement.
        /// </summary>
        /// <returns>The size (in bytes) of the data that is in the reliable, unsequenced packet queue awaiting acknowledgement.</returns>
        public UInt32 getReliableUnsequencedDataSize()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetReliableUnsequencedDataSize(statctx);
        }

        /// <summary>
        /// Returns the number of packets in the reliable, unsequenced packet queue awaiting acknowledgement.
        /// </summary>
        /// <returns>The number of packets in the reliable, unsequenced packet queue awaiting acknowledgement.</returns>
        public UInt32 getReliableUnsequencedPacketQueueSize()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetReliableUnsequencedPacketQueueSize(statctx);
        }

        // Returns statistics at the level of messages (instead of packets)
        // Includes cumulative numbers for messages of all types
        //MessageStats not wrapped yet...
        //MessageStats * getOverallMessageStatistics ();

        /// <summary>
        /// Returns the highest tag value that has been used by the application
        /// for identifying a particular message type.
        /// </summary>
        /// <remarks>
        /// Useful when iterating and obtaining statistics per type of message.
        /// </remarks>
        /// <returns>The highest tag value used.</returns>
        public UInt16 getHighestTag()
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            return NativeMethods.MocketStatsGetHighestTag(statctx);
        }

        /// <summary>
        /// Returns true if the application has sent any messages so far using the
        /// specified tag value to identify a message.
        /// </summary>
        /// <remarks>
        /// Useful when iterating and obtaining statistics per type of message.
        /// </remarks>
        /// <param name="ui16Tag">Tag value to look for.</param>
        /// <returns>True if the specified tag value has been used.</returns>
        public bool isTagUsed(UInt16 ui16Tag)
        {
            if (mocket.ctx == IntPtr.Zero)
                throw new ObjectDisposedException("ManagedMocket");

            int rval = NativeMethods.MocketStatsIsTagUsed(statctx, ui16Tag);

            return (rval != 0);
        }

        // Returns the statistics at the level of messages (instead of packets)
        // for messages of the specified type
        //MessageStats not wrapped yet...
        //MessageStats * getMessageStatisticsForType (UInt16 ui16Tag);

        /// <summary>
        /// The unmanaged pointer to the MocketStats (this doesn't need to be freed)
        /// </summary>
        private IntPtr statctx;

        /// <summary>
        /// The mocket that the statistics object is associated with (used to check to see if the mocket has
        /// been disposed)
        /// </summary>
        protected ManagedMocket mocket;

    }
}
