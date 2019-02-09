
/**
 * Represents an exception that occurs when a command in an MrRobot
 * is called while a complex command is running, and if interruptions
 * are not allowed. In this case, a CmdInterruptedException will
 * be thrown, and the interrupted command will continue running in the
 * background.
 */
public class CmdInterruptedException extends Exception
{
}

