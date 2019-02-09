package us.ihmc.aci.kernel;

import java.io.IOException;
import java.util.Hashtable;

import com.ibm.JikesRVM.*;

import org.vmmagic.pragma.*;

class RVMServiceRequestReader
{

  private static final int MAX_FIELD_BUF = 512;
  private static final int MAX_STATE_BUF = 500;

  private static RVMServiceRequestReader _instance = null;

  private Object ackSynchObject;

  private int _fd;
  private byte[] _methodName;
  private byte[] _classPathForService;
  private byte[] _classNameForService;
  private byte[] _UUID;
  private byte[] _stateBuff;

  /**
   * values[0] = sr.requestType
   * values[1] = int reference to the C++ ServiceRequest object
   * values[2] = sr.UUID.length;
   * values[3] = sr.methodName.length;
   * values[4] = sr.classNameForService.length
   * values[5] = sr.classPathForService
   * values[6] = sr.ui32BufLen
   * values[7] = sr.asynchronous
   * values[8] = client socket descriptor
   */
  private int[] values;

  static RVMServiceRequestReader getInstance(int fd)
  {
      if(_instance == null)
          _instance = new RVMServiceRequestReader(fd);

      return _instance;
  }

  private RVMServiceRequestReader(int fd)
  {
    _fd = fd;

    /* Make IO on this file descriptor NON BLOCKING*/
    VM_FileSystem.onCreateFileDescriptor(_fd, true);

    _methodName = new byte[MAX_FIELD_BUF];
    _classPathForService = new byte[MAX_FIELD_BUF];
    _classNameForService = new byte[MAX_FIELD_BUF];
    _UUID = new byte[MAX_FIELD_BUF];
    _stateBuff = new byte[MAX_STATE_BUF];
    values = new int[9];

    ackSynchObject = new Object();

    /* Send an ack to the ACI kernel to notify that the initialization of this socket is successfull*/
    VM_FileSystem.writeByte(_fd, 0);

  }

  /**
   * Is the given fd returned from an ioWaitRead() or ioWaitWrite()
   * ready?
   */
  private static boolean isFdReady(int fd) throws InlinePragma  {
    return (fd & VM_ThreadIOConstants.FD_READY_BIT) != 0;
  }

  static void printServiceRequest(JavaServiceRequest sr)
  {
        System.out.print("\n === RVM ===\nsr.requestType=");
        switch(sr.requestType)
        {
        case ServiceRequest.REQUEST_TYPE_ACTIVATE:
            System.out.print("ACTIVATE");
            break;
        case ServiceRequest.REQUEST_TYPE_CONNECT:
            System.out.print("CONNECT");
            break;
        case ServiceRequest.REQUEST_TYPE_INVOKE:
            System.out.print("INVOKE");
            break;
        case ServiceRequest.REQUEST_TYPE_TERMINATE:
            System.out.print("TERMINATE");
            break;
        case ServiceRequest.REQUEST_TYPE_CAPTURE_STATE:
            System.out.print("CAPTURE");
            break;
        case ServiceRequest.REQUEST_TYPE_RESTORE_STATE:
            System.out.print("RESTORE");
            break;
        default:
            System.out.print("UNKNOWN - rt = "+sr.requestType);
        }

        System.out.println("\nsr.methodName="+sr.methodName+
              "\nsr.UUID="+sr.UUID+"\nsr.classNameForService="+sr.classNameForService+
              "\nsr.classPathForService="+sr.classPathForService+"\nsr.stateBuffer="+sr.stateBuffer+"\n =========\n");
  }

  synchronized JavaServiceRequest getNextRequest() throws IOException
  {
    JavaServiceRequest sr = new JavaServiceRequest();
    int streamFD = -1;

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

      int rc = VM_SysCall.sysNetSocketReadMsg(_fd, _methodName, _classPathForService, _classNameForService, _UUID, _stateBuff, values);

      if(rc > 0) // Read successfull
      {
        sr.requestType = values[0];
        sr.originalServiceRequestRef = values[1];
        sr.asynchronous = (values[7] == 1) ? true : false;
        streamFD = values[8];
        sr.UUID = (values[2] > 0) ? new String(_UUID, 0, values[2]) : null;
        sr.methodName = (values[3] > 0) ? new String(_methodName, 0, values[3]) : null;
        sr.classNameForService = (values[4] > 0) ? new String(_classNameForService, 0, values[4]) : null;
        sr.classPathForService = (values[5] > 0) ? new String(_classPathForService, 0, values[5]) :  null;

        if ((sr.requestType == ServiceRequest.REQUEST_TYPE_RESTORE_STATE) && (values[6] > 0))
        {
            sr.stateBuffer = new byte[values[6]];

//            System.out.println("Reading "+values[6]+" bytes from Java...");
            int off = 0;
            int cnt = values[6];
            int read = 0;

            while((cnt > 0) && (read >= 0))
            {
                read = VM_FileSystem.readBytes(_fd, sr.stateBuffer, off, cnt);
                cnt -= read;
                off += read;
            }
        }
        else
            sr.stateBuffer = null;

    //        printServiceRequest(sr);

        if(streamFD > 3) {
            VM_FileSystem.onCreateFileDescriptor(streamFD, true);

            //Waiting for the passed socked descriptor to be ready
            while(VM_FileSystem.bytesAvailable(streamFD)==0)
        Thread.yield();
        //            try{Thread.sleep(20);}catch(InterruptedException e){}

            sr.sis = new RVMServiceInputStream(streamFD);
            sr.sos = new RVMServiceOutputStream(streamFD);
        }
        else
        {
            sr.sis = null;
            sr.sos = null;
        }

        return sr;
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

  void notifyRequestCompleted(JavaServiceRequest sr, boolean success)
  {
      synchronized (ackSynchObject)
      {
          byte[] dword = new byte[4];

          /* Writes an int as four bytes, high bytes first*/
          dword[0] = (byte) (sr.originalServiceRequestRef & 0x00FF);
          dword[1] = (byte) ((sr.originalServiceRequestRef >> 8) & 0x000000FF);
          dword[2] = (byte) ((sr.originalServiceRequestRef >> 16) & 0x000000FF);
          dword[3] = (byte) ((sr.originalServiceRequestRef >> 24) & 0x000000FF);

          VM_FileSystem.writeBytes(_fd, dword, 0, 4);

          if(sr.requestType == JavaServiceRequest.REQUEST_TYPE_CAPTURE_STATE)
          {
              int len = (sr.stateBuffer != null) ? sr.stateBuffer.length : 0;
              dword[0] = (byte) (len & 0x00FF);
              dword[1] = (byte) ((len >> 8) & 0x000000FF);
              dword[2] = (byte) ((len >> 16) & 0x000000FF);
              dword[3] = (byte) ((len >> 24) & 0x000000FF);
              VM_FileSystem.writeBytes(_fd, dword, 0, 4);

              VM_FileSystem.writeBytes(_fd, sr.stateBuffer, 0, len);
          }
          else if(sr.requestType == JavaServiceRequest.REQUEST_TYPE_RESTORE_STATE)
          {
              int rc = (success) ? 0 : -1;

              dword[0] = (byte) (rc & 0x00FF);
              dword[1] = (byte) ((rc >> 8) & 0x000000FF);
              dword[2] = (byte) ((rc >> 16) & 0x000000FF);
              dword[3] = (byte) ((rc >> 24) & 0x000000FF);
              VM_FileSystem.writeBytes(_fd, dword, 0, 4);
          }
      }
  }
}
