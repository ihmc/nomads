
import java.util.*;

/**
 * CommandListener is an interface used to create callbacks for complex
 * commands in a Robot. This interface contains one method that reperesents
 * an event that occurs when a complex command given to a Robot is stopped,
 * whether or not that command has finished.
 */
public interface CommandListener extends EventListener
{
    /**
     * Called when any complex command is stopped. This method gets run in its
     * own new thread when it is called, so it should ideally be implemented
     * simply, and should definately not get into an infinite loop.
     * @param reason The reason the complex command stopped.
     * @param data The object passed to the complex command called, or null;
     * can be used to identify which complex command is being stopped.
     */
    public void commandStopped(CommandStopReason reason, Object data);
    public enum CommandStopReason { FINISHED, INTERRUPTED, TIMEOUT }
}

