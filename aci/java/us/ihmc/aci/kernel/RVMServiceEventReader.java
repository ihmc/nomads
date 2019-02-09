package us.ihmc.aci.kernel;

import java.io.IOException;
import org.vmmagic.pragma.InlinePragma;
import com.ibm.JikesRVM.VM_FileSystem;
import com.ibm.JikesRVM.VM_SysCall;
import com.ibm.JikesRVM.VM_ThreadIOConstants;
import com.ibm.JikesRVM.VM_ThreadIOWaitData;
import com.ibm.JikesRVM.VM_Wait;

class RVMServiceEventReader 
{
     
    static final int MAX_FIELD_BUF = 512;
    static final int MAX_DATA_LEN = 65536;
    static final int MAX_PARAM_LEN = 65536;
    
    private int _fd;
    private byte[] _nodeUUID;
    private byte[] _groupName;
    private byte[] _memberUUID;
    private byte[] _searchUUID;
    private byte[] _data;
    private byte[] _param;

    /** 
      * values[0] = se._eventType;                                                                                                                                                       
      * values[1] = se._nodeUUID.length;                                                                                                                                                      
      * values[2] = se._groupName.length;                                                                                                                                                
      * values[3] = se._memberUUID.length;                                                                                                                                        
      * values[4] = se._searchUUID.length;                                                                                                                                               
      * values[5] = se._data.length;
      * values[6] = se._param.length; */
    private int[] _values;

      
    RVMServiceEventReader(int fd)
    {
        _fd = fd;
        
        /* Make IO on this file descriptor NON BLOCKING*/
        VM_FileSystem.onCreateFileDescriptor(_fd, true);        
              
        _nodeUUID = new byte[MAX_FIELD_BUF];
        _groupName = new byte[MAX_FIELD_BUF]; 
        _memberUUID = new byte[MAX_FIELD_BUF];
        _searchUUID = new byte[MAX_FIELD_BUF];
        _data = new byte[MAX_DATA_LEN];
        _param = new byte[MAX_PARAM_LEN];
        _values = new int[7];
        
        /* Send an ack to the ACI kernel to notify that the initialization of this socket is successfull*/
        VM_FileSystem.writeByte(_fd, 0);
        
    }
    
    private static void printServiceEvent(ServiceEvent se)
    {
            System.out.print("\n === RVM ===\nse.eventType=");
            switch(se.eventType)
            {
            case ServiceEvent.CONFLICS_WITH_PRIVATE_PEER_GROUP:         
                System.out.print("CONFLICS_WITH_PRIVATE_PEER_GROUP");
                break;
            case ServiceEvent.DEAD_PEER:            
                System.out.print("DEAD_PEER");
                break;
            case ServiceEvent.GROUP_LIST_CHANGE:            
                System.out.print("GROUP_LIST_CHANGE");
                break;
            case ServiceEvent.GROUP_MEMBER_LEFT:            
                System.out.print("GROUP_MEMBER_LEFT");
                break;
            case ServiceEvent.NEW_GROUP_MEMBER:         
                System.out.print("NEW_GROUP_MEMBER");
                break;
            case ServiceEvent.NEW_PEER:         
                System.out.print("NEW_PEER");
                break;
            case ServiceEvent.PEER_GROUP_DATA_CHANGED:          
                System.out.print("PEER_GROUP_DATA_CHANGED");
                break;
            case ServiceEvent.PEER_SEARCH_REQUEST_RECEIVED:         
                System.out.print("PEER_SEARCH_REQUEST_RECEIVED");
                break;
            case ServiceEvent.PEER_SEARCH_RESULT_RECEIVED:          
                System.out.print("PEER_SEARCH_RESULT_RECEIVED");
                break;
            case ServiceEvent.PERSISTENT_PEER_SEARCH_TERMINATED:            
                System.out.print("PERSISTENT_PEER_SEARCH_TERMINATED");
                break;
            case ServiceEvent.UNDEFINED:            
                System.out.print("UNDEFINED");
                break;          
            }
            
            System.out.println("\nse.groupName="+se.groupName+
                  "\nse.memberUUID="+se.memberUUID+"\nse.nodeUUID="+se.nodeUUID+
                  "\nse.searchUUID="+se.searchUUID+"\nse.data="+se.data
                  +"\nse.param="+se.param+"\n =========\n");
    }
    
    /**
       * Is the given fd returned from an ioWaitRead() or ioWaitWrite()
       * ready?
       */
    private static boolean isFdReady(int fd) throws InlinePragma  {
        return (fd & VM_ThreadIOConstants.FD_READY_BIT) != 0;
    }
      
    ServiceEvent getNextEvent() throws IOException
    {
        
        ServiceEvent se = new ServiceEvent();       
        
        // 1. Check that the socket descriptor is still valid
        if(VM_SysCall.sysIsValidFD(_fd)==1)
          throw new IOException("Lost communication with the ACIKernel");

        // 2. Start non blocking IO cycle
        // The canonical read loop.  Try the read repeatedly until either
        //   - it succeeds,
        //   - it returns with an error.
        // If the read fails because it would have blocked, then
        // put this thread on the IO queue, then try again if it
        // looks like the fd is ready.
        for (;;) {
            
          int rc = VM_SysCall.sysNetSocketReadEvent(_fd, _nodeUUID, _groupName, _memberUUID,
                                                    _searchUUID, _data, _param, _values);
          
          if(rc > 0) // Read successfull
          {                     
            se.eventType = _values[0];         
            se.nodeUUID = (_values[1] > 0) ? new String(_nodeUUID, 0, _values[1]) : null;
            se.groupName = (_values[2] > 0) ? new String(_groupName, 0, _values[2]) : null;
            se.memberUUID = (_values[3] > 0) ? new String(_memberUUID, 0, _values[3]) : null;
            se.searchUUID = (_values[4] > 0) ? new String(_searchUUID, 0, _values[4]) :  null;        
            
            if(_values[5] > 0)
            {
                se.data = new byte[_values[5]];
                System.arraycopy(_data, 0, se.data, 0, _values[5]);
            }
            else
                se.data = null;
            
            if(_values[6] > 0)
            {
                se.param = new byte[_values[6]];
                System.arraycopy(_param, 0, se.param, 0, _values[6]);
            }
            else
                se.param = null;
            
//          printServiceEvent(se);
            
            return se;
          }      
          else if (rc == -2) {               
              // Operation would have blocked
              VM_ThreadIOWaitData waitData = VM_Wait.ioWaitRead(_fd);
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
     
}
