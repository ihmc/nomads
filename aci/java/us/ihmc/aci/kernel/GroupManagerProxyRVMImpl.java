package us.ihmc.aci.kernel;

import org.vmmagic.pragma.InlinePragma;

import com.ibm.JikesRVM.*;

import us.ihmc.aci.grpMgrOld.GroupManagerListener;
import us.ihmc.aci.grpMgrOld.GroupManagerProxy;

class GroupManagerProxyRVMImpl extends GroupManagerProxy 
{
    int _sd;
    private GroupManagerListener _listener = null;
    private static final boolean DEBUG = true;
    private byte[] _buffer;
    int[] _bufLen;
        
    public static final int T_Undefined = 0x00;
    public static final int T_GetNodeUUID = 0x01;
    public static final int T_GetNodeName = 0x02;
    public static final int T_CreatePublicPeerGroup = 0x03;
    public static final int T_CreatePrivatePeerGroup = 0x04;
    public static final int T_StartPersistentPeerSearch = 0x05;
    public static final int T_StopPersistentPeerSearch = 0x06;
    public static final int T_RespondToPeerSearch = 0x07;
    public static final int T_UpdatePeerGroupData = 0x08;
    public static final int T_StatusChanged = 0x09;
    public static final int T_RemoveGroup = 0x10;
    public static final int T_CreatePublicManagedGroup = 0x11;
    public static final int T_CreatePrivateManagedGroup = 0x12;
    public static final int T_JoinPublicManagedGroup = 0x13;
    public static final int T_JoinPrivateManagedGroup = 0x14;
    public static final int T_LeaveGroup = 0x15;
    
    public static final int T_lookupService = 0x16;
    public static final int T_registerServiceAttribute = 0x17;
    public static final int T_deregisterServiceAttribute = 0x18;
    public static final int T_hasServiceAttribute = 0x19;
    public static final int T_startPeerSearch = 0x20;   
    public static final int T_stopPeerSearch = 0x21;
    public static final int T_sendPeerMessage = 0x22;
    public static final int T_broadcastPeerMessageToGroup = 0x23;
    /**
    *
    */
   public void setListener (GroupManagerListener gml)
   {
       log ("setListener:: " + gml);
       _listener = gml;
   } 

   /**
    *
    */
   public GroupManagerListener getListener()
   {
       return _listener;
   } 

   /**
    * @param msg String
    */
   private static void log (String msg)
   {
       if (DEBUG) {
           System.out.println ("[GrpMgrProxyRVMImpl] " + msg);
       }
   }

    public GroupManagerProxyRVMImpl(int socketFD) 
    {       
        _sd = socketFD;
        _buffer = new byte[RVMServiceEventReader.MAX_FIELD_BUF];
        _bufLen = new int[1];
         /* Make IO on this file descriptor NON BLOCKING*/
        VM_FileSystem.onCreateFileDescriptor(_sd, true); 
        
        /* Send an ack to the ACI kernel to notify that the initialization of this socket is successfull*/
        VM_FileSystem.writeByte(_sd, 0);
      
    }
    
    private void invokeGroupMgrMethod(int type, byte[] param1, byte[] param2, byte[] param3, long param4)
    {
         for (;;) {
              int rc = VM_SysCall.sysWriteGroupManagerMsg(_sd, type, 
                                param1, (param1 != null) ? param1.length : 0, 
                                param2, (param2 != null) ? param2.length : 0, 
                                param3, (param3 != null) ? param3.length : 0,
                                param4);                  
              if (rc > 0) {
                // Write succeeded
                return;
              } else if (rc == -1) {
                // Write would have blocked
                VM_ThreadIOWaitData waitData = VM_Wait.ioWaitWrite(_sd);
                if (!isFdReady(waitData.writeFds[0]))
                  // Fd is not ready, so presumably an error occurred while on IO queue
                  return;
                else
                  // Fd apprears to be ready, so try write again
                  continue;
              }
              else
                // Write returned with an error
                return;
            }           
    }
    
    /**
       * Is the given fd returned from an ioWaitRead() or ioWaitWrite()
       * ready?
       */
    private static boolean isFdReady(int fd) throws InlinePragma  
    {
        return (fd & VM_ThreadIOConstants.FD_READY_BIT) != 0;
    }
    
    private String getGroupMgrMethodResult()
    {               
        
        // Start non blocking IO cycle
        for (;;) {
            
          int rc = VM_SysCall.sysReadGroupManagerMsg(_sd, _buffer, _bufLen);
          
          if(rc > 0) // Read successfull
          {
              return (_bufLen[0] > 0) ? new String(_buffer, 0, _bufLen[0]) : null;
          }
          else if (rc == -2) {               
              // Operation would have blocked
              VM_ThreadIOWaitData waitData = VM_Wait.ioWaitRead(_sd);
              if (!isFdReady(waitData.readFds[0]))
                // Hmm, the wait returned, but the fd is not ready.
                // Assume an error was detected (such as the fd becoming invalid).
                return null;
              else                  
                // Fd seems to be ready now, so retry the read
                continue;                               
         } 
         else
              return null;            
        }
    }
    
    public void createPrivateManagedGroup(String groupName, String password) 
    {       
//      log ("createPrivateManagedGroup");
        invokeGroupMgrMethod (T_CreatePrivateManagedGroup, groupName.getBytes(), password.getBytes(), null, 0);
    }

    public void createPrivatePeerGroup(String groupName, String password, byte[] data) 
    {
//      log ("createPrivatePeerGroup");
        invokeGroupMgrMethod (T_CreatePrivatePeerGroup, groupName.getBytes(), password.getBytes(), data, 0);
    }

    public void createPublicManagedGroup(String groupName) 
    {
//      log ("createPublicManagedGroup");
        invokeGroupMgrMethod (T_CreatePublicManagedGroup, groupName.getBytes(), null, null, 0);
    }

    public void createPublicPeerGroup(String groupName, byte[] data) 
    {
//      log ("createPublicPeerGroup");
        invokeGroupMgrMethod (T_CreatePublicPeerGroup, groupName.getBytes(), data, null, 0);    
    }

    public String getNodeName(String nodeUUID) 
    {
//      log("getNodeName");
        invokeGroupMgrMethod(T_GetNodeName, nodeUUID.getBytes(), null, null, 0);        
        return getGroupMgrMethodResult();
    }

    public String getNodeUUID() 
    {       
        invokeGroupMgrMethod (T_GetNodeUUID, null, null, null, 0);              
        return getGroupMgrMethodResult();
    }

    public void joinPrivateManagedGroup(String groupName, String creatorUUID, String password) 
    {
//      log ("joinPrivateManagedGroup");
        invokeGroupMgrMethod (T_JoinPrivateManagedGroup, groupName.getBytes(), creatorUUID.getBytes(), password.getBytes(), 0);
    }

    public void joinPublicManagedGroup (String groupName, String creatorUUID) 
    {
//      log ("joinPublicManagedGroup");
        invokeGroupMgrMethod (T_JoinPublicManagedGroup, groupName.getBytes(), creatorUUID.getBytes(), null, 0);     
    }

    public void leaveGroup(String groupName) 
    {
//      log ("leaveGroup");
        invokeGroupMgrMethod (T_LeaveGroup, groupName.getBytes(), null, null, 0);
    }

    public void removeGroup(String groupName) 
    {
//      log ("removeGroup");
        invokeGroupMgrMethod (T_RemoveGroup, groupName.getBytes(), null, null, 0);
    }

    public void respondToPeerSearch(String searchUUID, byte[] param) 
    {
//      log ("respondToPeerSearch");
        invokeGroupMgrMethod (T_RespondToPeerSearch, searchUUID.getBytes(), param, null, 0);
    }

    public String startPersistentPeerSearch(String groupName, byte[] param) 
    {
        log ("startPersistentPeerSearch");
        invokeGroupMgrMethod (T_StartPersistentPeerSearch, groupName.getBytes(), param, null, 0);
        log (" after startPersistentPeerSearch");
        return getGroupMgrMethodResult(); 
    }

    public void statusChanged() 
    {
//      log ("statusChanged");
        invokeGroupMgrMethod (T_StatusChanged, null, null, null, 0);
    }

    
    public void stopPersistentPeerSearch(String searchUUID) 
    {
//      log ("stopPersistentPeerSearch");
        invokeGroupMgrMethod (T_StopPersistentPeerSearch, searchUUID.getBytes(), null, null, 0);
    }

    public void updatePeerGroupData(String groupName, byte[] data) 
    {
//      log ("updatePeerGroupData");
        invokeGroupMgrMethod (T_UpdatePeerGroupData, groupName.getBytes(), data, null, 0);
    }
    
    
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //                                                                                               //
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    public String lookupService (String serviceName, String attrKey, String attrValue, long timeout)
    {
//      log ("lookupService");
        invokeGroupMgrMethod (T_lookupService, serviceName.getBytes(), attrKey.getBytes(), attrValue.getBytes(), timeout);
        return getGroupMgrMethodResult();
    }
    
    void registerServiceAttribute (String serviceUUID, String attrKey, String attrVal)
    {
//      log ("registerServiceAttribute");
        invokeGroupMgrMethod (T_registerServiceAttribute, serviceUUID.getBytes(), attrKey.getBytes(), attrVal.getBytes(), 0);
    }
    
    void deregisterServiceAttribute (String serviceUUID, String attrKey, String attrVal)
    {
//      log ("deregisterServiceAttribute");
        invokeGroupMgrMethod (T_deregisterServiceAttribute, serviceUUID.getBytes(), attrKey.getBytes(), attrVal.getBytes(), 0);
    }
    
    boolean hasServiceAttribute (String serviceUUID, String attrKey, String attrVal)
    {
//      log ("hasServiceAttribute");
        invokeGroupMgrMethod (T_hasServiceAttribute, serviceUUID.getBytes(), attrKey.getBytes(), attrVal.getBytes(), 0);
        return getGroupMgrMethodResult().equals("yes");
    }
    
    // TODO: THESE METHODS SEEM TO BE IMPLEMENTED IN JavaServiceManager
    
    String startPeerSearch (String serviceUUID, byte[] param, long TTL)
    {
//      log ("startPeerSearch");
//      invokeGroupMgrMethod (T_startPeerSearch, serviceUUID.getBytes(), param, null, TTL);
//      return getGroupMgrMethodResult();
        return null;
    }
    
    void stopPeerSearch (String serviceUUID, String searchUUID)
    {
//      log("stopPeerSearch");
//      invokeGroupMgrMethod(T_stopPeerSearch, serviceUUID.getBytes(), searchUUID.getBytes(), null, 0);
    }

    public void sendPeerMessage (String destNodeUUID, byte[] data, boolean reliable, int timeout)
    {
        log ("sendPeerMessage");
        //invokeGroupMgrMethod (T_sendPeerMessage, destNodeUUID.getBytes(), data, reliable, timeout);
    }

    public void broadcastPeerMessageToGroup (String groupName, byte[] data) 
    {
        log ("broadcastPeerMessageToGroup");
        //invokeGroupMgrMethod (T_broadcastPeerMessageToGroup, groupName.getBytes(), data);
    }
}

