package us.ihmc.aci.kernel;

import java.io.IOException;
import com.ibm.JikesRVM.VM_FileSystem;
import com.ibm.JikesRVM.VM_SysCall;

public class RVMServiceOutputStream extends ServiceOutputStream
{

    private int socketFD;
    private boolean closed;

    RVMServiceOutputStream(int sd) {
        socketFD = sd;
        closed = false;
    }

    public void write (int b) throws IOException
    {
        if (closed) throw new IOException("stream closed");
         byte[] buffer = new byte[]{ (byte)b };
         arrayWrite(buffer, 0, 1);
    }

    public void write (byte b[], int off, int len) throws IOException
    {
        if (closed) throw new IOException("stream closed");
        arrayWrite (b, off, len);
    }

    public synchronized void flush() throws IOException{
        if (closed) throw new IOException("stream closed");
         VM_FileSystem.sync( socketFD );
    }

    public synchronized void close() throws IOException {
        if(!closed)
        {
        //  if(VM_SysCall.sysIsValidFD(socketFD)==0)
                VM_SysCall.sysNetSocketClose(socketFD);
            closed = true;
        }
    }

    public synchronized void arrayWrite (byte b[], int off, int len) throws IOException{
        if (closed) throw new IOException("stream closed");
        if (len == 0) return;

        //int index = 0;
        int written = 0;

    //        while (index < len) {
    //   try {
                 written = VM_FileSystem.writeBytes(socketFD, b, off, len);
         //      System.out.println(Thread.currentThread().getName()+":[Written on fd="+socketFD+"] = "+written);
         //      if (written < 0) {
         //     throw new IOException("Error writing into socket descriptor n."+socketFD);
         // }
      //           index += written;
      //       }
      //        catch (IOException ex) {
      //           throw ex;
      //       }
      //   }

    }

}
