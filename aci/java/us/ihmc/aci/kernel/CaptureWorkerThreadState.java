package us.ihmc.aci.kernel;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;
import com.ibm.JikesRVM.CaptureStateRequest;
import com.ibm.JikesRVM.VM_MobileFrame;

/**
 *
 * @author rquitadamo
 */
class CaptureWorkerThreadState implements CaptureStateRequest{

    CaptureWorkerThreadState(JavaServiceRequest jsr, Object instance, int numThreads) throws IOException
    {
        threadsToCapture = numThreads;
        captureRequest = jsr;
        serviceInstance = instance;

        baos.reset();
        oos = new ObjectOutputStream(baos);
        invocations = 0;
        states = new VM_MobileFrame[threadsToCapture];
        binaryRequests = new boolean[threadsToCapture];
        captureFailed = false;
        keepOnRunning = true;
        migrationCompleted = false;
        abortionCause = null;

        invokersQueue = new Object();
    }

    /**
     * Main state serialization method
     *
     * @return the serialized byte[] to be passed to the underlying ACI kernel
     */
    byte[] waitForCompletion() throws Exception
    {
        try {

            // 0. Serialize the service instance
            oos.writeObject(serviceInstance);

            synchronized(this)
            {
	            // 1. Wait for a maximum number of milliseconds before starting serialization
	            if(migrationCompleted == false)
	                try { wait(captureRequest.timeOut); } catch (InterruptedException e) { }            
            
	            // If we are here we own the lock on the state object. No other thread is allowed to enter the synchronized
	            // methods of this object until this method returns.
	
	            // 2. Check if everything went ok
	            if(captureFailed)
	            {
	                throw new IOException("Service capture failure due to the following cause: " + abortionCause);
	            }
            
            }

            // 3. Ok! Serialize the number of invocations
            oos.writeInt(invocations);

            // 4. Serialize each invocation state
            // TODO: USE A SMARTER WAY TO SERIALIZE AN INVOCATION!! E.G. A SPECIAL OBJECT "Invocation State"
            for(int i = 0; i < invocations; i++)
            {
                oos.writeBoolean(binaryRequests[i]);
                int numFrames = countFrames(states[i]);
                oos.writeInt(numFrames);
                if(numFrames > 0)
                    states[i].writeExternal(oos);

            }

            // 4. Close the stream and return the byte[] to the caller
            oos.flush();
            oos.close();
            return baos.toByteArray();
        }
        catch(Exception e)
        {
            keepOnRunning = true;
            migrationCompleted = true;
            synchronized(invokersQueue)
            {
                invokersQueue.notifyAll();
            }
            throw e;
        }
    }

    /**
     * Simple method to count the number of frames into a chain
     *
     * @param chain The back-linked list of frames for an invocation
     *
     * @return the number of frames calculated
     */
    private int countFrames(VM_MobileFrame chain){
        int i=0;
        VM_MobileFrame frame = chain;

        while(frame!=null)
        {
            i++;
            frame = frame.callerFrame;
        }
        return i;
    }

    /**
     * This method sets the completion status of this capture request
     * and has the side effect of waking up the invokers waiting for the
     * migration to be finished.
     *
     * @param success The result returned by the ACI Kernel
     */
    synchronized void setCompletionResult(boolean success)
    {
        // 1. Set completion flag
        migrationCompleted = true;

        // 2. Decide what to do with the invoker threads
        if(success)
            keepOnRunning = false;
        else
            keepOnRunning = true;

        // 3. Wake them up know about completion
        synchronized(invokersQueue)
        {
            invokersQueue.notifyAll();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // CaptureStateRequest INTERFACE IMPLEMENTATION                               //
    ////////////////////////////////////////////////////////////////////////////////

    /**
     * Method that is invoked by invoker threads, right after they have collected their frames
     * in response to a capture notification from the kernel.
     *
     * @param frameChain The backward linked list of captured frames
     *
     */
    public synchronized void writeState(VM_MobileFrame frameChain)
    {
        Thread t = Thread.currentThread();

        // 0. If the service instance is a thread, do not store its chain into the invocations
        if(t == serviceInstance)
            return;

        // 1. Store the chain into the array of states
        states[invocations] = frameChain;
        binaryRequests[invocations] = ((ServiceInvokerThread) t).isBinaryRequest();

        // 2. Increase the number of invocations written in this state object
        invocations++;
    }

    /**
     * Method called by every invoker who fails to capture its execution state
     * for some reason.
     *
     * @param cause The exception raised by the migration process
     * @return true if the invoker should keep on running, false if it should die
     */
    public synchronized boolean onCaptureFailed(Throwable cause)
    {

//        cause.printStackTrace();

        // 0. If I'm the first invoker to experience a failure, set the abortionCause object and the captureFailed flag
        if(captureFailed == false)
        {
            captureFailed = true;
            abortionCause = cause;
        }

        // 1. If the capture has failed there's no need to wait for other threads to
        // finished their job. We simply wake up the main thread and let him
        // handle the failure.
        notify();

        // 2. and return true to make this invocation continue normally
        return true;
    }

    /**
     * Method called by each invoker thread when state capture is successfull.
     *
     * @return true if the invoker should keep on running, false if it should die
     */
    public boolean onCaptureSucceded()
    {
        // 1. If state capture was successfull, we have to decide whether to wake up or not the main
        // service manager thread.
        synchronized(this)
        {
            if(invocations == threadsToCapture)
                notify();
        }

        // 2. Then wait for the final decision
        try {
            if(migrationCompleted == false)
            {
                synchronized (invokersQueue)
                {
                    invokersQueue.wait();
                }
            }
        } catch (InterruptedException e) {
        }

        // 3. Consider what to do (run or die)
        return keepOnRunning;
    }

    // //////////////////////////////////////////////////////////////////////////
    // INSTANCE FIELDS                                                         //
    // //////////////////////////////////////////////////////////////////////////

    /**
     * The request object for the capture request.
     */
    JavaServiceRequest captureRequest;

    /**
     * The instance of the service to be migrated
     */
    Object serviceInstance;

    /**
     * The maximum number of threads that should be captured
     */
    int threadsToCapture;

    /**
     * The ObjectOutputStream to serialize objects into (built around an ByteArrayOutputStream)
     */
    ObjectOutputStream oos;

    /**
     * The number of invocations written into this state object
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

    /**
     * This flag is set by an invoker thread when its state capture has failed
     */
    boolean captureFailed;

    /**
     * Flag set by the Main ServiceManager thread to inform the invoker threads of its
     * decision after migration (die or keep on running on this machine)
     */
    boolean keepOnRunning;

    /**
     * Flag that sets when the migration process is completed (with success or aborted)
     */
    boolean migrationCompleted;

    /**
     * If migration failed, this field reports the first error occurred
     */
    Throwable abortionCause;

    /**
     * The object on which invokers synchronize
     */
    Object invokersQueue;

    // //////////////////////////////////////////////////////////////////////////
    // STATIC FIELDS                                                           //
    // //////////////////////////////////////////////////////////////////////////
    private static final int INITIAL_STATE_BUF = 65536;

    /**
     * The byte[] output stream for the state buffer
     */
    static ByteArrayOutputStream baos;

    /**
     * Static initializer that creates the byte[] input stream
     */
    static{
        baos = new ByteArrayOutputStream(INITIAL_STATE_BUF);
    }
}
