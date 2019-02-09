package us.ihmc.aci.kernel;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import com.ibm.JikesRVM.VM_CompiledMethod;
import com.ibm.JikesRVM.VM_FrameInstaller;
import com.ibm.JikesRVM.VM_MobileFrame;
import com.ibm.JikesRVM.VM_MobileObjectInputStream;

class RestoreWorkerThreadState {

    RestoreWorkerThreadState(JavaServiceRequest jsr) throws IOException {
        restoreRequest = jsr;
        invocations = 0;
        serviceInstance = null;
        ois = null;
    }

    Object readServiceInstance(ClassLoader cl) throws IOException, ClassNotFoundException
    {
        if(restoreRequest.stateBuffer == null)
            throw new IOException("State buffer null. Aborting restore request.");

        if(ois == null)
        {
            ois = new VM_MobileObjectInputStream(new ByteArrayInputStream(restoreRequest.stateBuffer), cl);
        }

        if(serviceInstance == null)
            serviceInstance = ois.readObject();

        invocations = ois.readInt();
        binaryRequests = new boolean[invocations];
        states = new VM_MobileFrame[invocations];

        for(int i = 0; i < invocations; i++)
        {
            binaryRequests[i] = ois.readBoolean();
            int numFrames = ois.readInt();
            if(numFrames > 0)
            {
                states[i] = new VM_MobileFrame(numFrames);
                states[i].readExternal(ois);
            }
        }

        return serviceInstance;
    }

    VM_CompiledMethod restoreInvocation(int index) throws IOException
    {
        VM_FrameInstaller installer = new VM_FrameInstaller(states[index]);
        VM_CompiledMethod cm = installer.compileFrameMethods(ois, false);
        return cm;
    }

    // //////////////////////////////////////////////////////////////////////////
    // INSTANCE FIELDS                                                         //
    // //////////////////////////////////////////////////////////////////////////

    /**
     * The request object for the capture request.
     */
    JavaServiceRequest restoreRequest;

    /**
     * The instance of the service to be migrated
     */
    Object serviceInstance;

    /**
     * The VM_MobileObjectInputStream to deserialize objects from (built around a ByteArrayInputStream)
     */
    VM_MobileObjectInputStream ois;

    /**
     * The number of invocations to be restored
     */
    int invocations;

    /**
     * The array of captured states
     */
    VM_MobileFrame[] states;

    /**
     * Array to store the kind of each saved invocation (binary or not)
     */
    boolean[] binaryRequests;

}
