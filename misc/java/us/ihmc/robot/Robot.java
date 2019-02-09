/*
* Robot.java
* by Kristopher Brosch
*/

import java.util.ArrayList;

/**
 * <p>The Robot class is an abstract class representing a generic robot. It
 * contains code to handle background processing of "complex commands," and
 * several generic motion commands. </p><p>All methods that represent
 * commands to the robot will return immediately, whether or not they
 * continue running in the background. Commands that continue running in
 * the background are "complex commands." After calling a complex command,
 * by default, a call to ANY command before the complex command is finished
 * will throw a CmdInterruptedException, the running complex command will
 * continue, and the command that was just called will not run. If you call 
 * allowInterruptions(true), however, a call to any command while a complex
 * command is running will immediately kill the running complex command, and
 * start the new command. Note that methods like getCurrentVoltage() or
 * setXYT() are not commands, while methods like setVelocity() are commands,
 * and methods like moveToXYZ() are complex commands. You can check whether
 * the last complex command you called is done yet by calling isDone(). 
 * You can make the current thread wait until the last complex command called
 * is done by calling waitUntilDone(). You can also use an implementation of
 * the CommandListener interface by passing an instance of it to the
 * addCommandListener method, in order to get a callback every time a complex
 * command stops. A timeout for complex commands can be set using setTimeout().
 * Complex commands will stop when they finish, when they are interrupted by a
 * command call, or when their timeout expires. The callback is passed the
 * reason the complex command was stopped as well as an optional object which
 * can be passed into any complex command call, and can be used to identify the
 * call that just stopped.</p>
 */
public abstract class Robot
{
    /*
     * Control methods
     */

    /**
     * Create a new Robot.
     */
    public Robot()
    {
        commandInterruptionsAllowed = false;
    }

    /**
     * Wait a given ammount of time. Provided for convenience.
     * @param t The ammount of time to wait (in milliseconds).
     */
    public static void sleep (long t)
    {
        try {
            Thread.sleep (t);
        }
        catch (InterruptedException e) {}
    }

    /**
     * Connect the robot in a default way. Remember, an implimentation
     * of a connect method must make sure to set up the commandThread (say
     * commandThread = new RobotThread, and run commandThread.start().
     */
    public abstract boolean connect();

    /**
     * Disconnect from the robot (if it is connected).
     */
    public abstract void disconnect();

    /**
     * Get whether the robot is connected or not.
     * @return True if the robot is connected.
     */
    public abstract boolean isConnected();

    /**
     * Get the current battery voltage in the robot.
     * @return The current voltage.
     */
    public abstract double getCurrentVoltage();

    /**
     * Set whether the current complex command should be interrupted or
     * an exception should be thrown when a command is called while
     * another complex command is still running.
     * @param interruptionsAllowed True if commands should be interrupted;
     * false if exceptions should be thrown.
     */
    public void allowInterruptions (boolean interruptionsAllowed)
    {
        commandInterruptionsAllowed = interruptionsAllowed;
    }

    /**
     * Set a command listener up so that its commandStopped method will
     * get called whenever a complex command ends. All command listeners
     * will be lost if the robot is disconnected then reconnected.
     * @param listener The CommandListener to add.
     */
    public void addCommandListener (CommandListener listener)
    {
        commandThread.addCommandListener (listener);
    }

    /**
     * Set a timeout for the last complex command called. This timeout only
     * applies to the last complex command called before this method is called.
     * If this ammount of time expires <b>from the time the complex command was
     * called, <i>not</i> from the time this method was called</b> without the
     * complex command finishing, the complex command will automatically be
     * interrupted, all the CommandListeners will be notified that the command
     * stopped without finishing, and the robot will be stopped. If this method
     * is called more than once during the same complex command, the latest
     * value given will be used. Call this method <i>after</i> the call to the
     * complex command you want to associate this timeout with. A timeout of
     * zero is the same as no timeout, and of course a negative timeout doesn't
     * make sense. You don't need to set a timeout every time you call a
     * complex command; there is by default no timeout on any complex command.
     * @param newTimeout The timeout (in milliseconds).
     */
    public void setTimeout (double newTimeout)
    {
        commandThread.setTimeout (newTimeout);
    }

    /**
     * Interrupts the current complex command (if any). This method interrupts
     * the current complex command no matter what; it does not matter whether
     * interruptions are allowed or not, and this method will not
     * throw an exception.
     */
    public void stopCurrentCommand()
    {
        commandThread.stopCommand(
                                 CommandListener.CommandStopReason.INTERRUPTED);
    }

    /**
     * Remove a previously added command listener from the current list of
     * CommandListeners, so that its commandStopped method will not get
     * called when complex commands end.
     * @param listener The command listener to remove.
     */
    public void removeCommandListener (CommandListener listener)
    {
        commandThread.removeCommandListener (listener);
    }

    /**
     * Add a ConnectionLostListener so that its connectionLost method will get
     * called in case the robot is unexpectedly disconnected. This callback will
     * only get called once; if you re-connect, you will have to add a new
     * ConnectionLostListener to watch for another unexpected disconnect.
     * @param listener The ConnectionLostListener to add.
     */
    public void addConnectionLostListener (ConnectionLostListener listener)
    {
        commandThread.addConnectionLostListener (listener);
    }

    /**
     * Remove a ConnectionLostListener from the list of listeners to be notified
     * in case the robot gets unexpectedly disconnected.
     * @param listener The listener to remove.
     */
    public void removeConnectionLostListener (ConnectionLostListener listener)
    {
        commandThread.removeConnectionLostListener (listener);
    }

    /**
     * Gets the time since the last complex command was called.
     * @return The time (in milliseconds) since the last complex command was
     * called.
     */
    public double getTimeSinceLastComplexCommand()
    {
        return commandThread.getTimeSinceLastCommand();
    }

    /*
     * Motion methods
     */

    /**
     * Move the robot to a given location [complex command]. There
     * is no obstacle avoidance.
     * @param x The target x location in meters.
     * @param y The target y location in meters.
     * @param z The target z location in meters.
     * @param data Data to pass through to command listeners when this command
     * is stopped.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public abstract void moveToXYZ (double x,
                                    double y,
                                    double z,
                                    Object data)
    throws CmdInterruptedException;

    /**
     * Same as moveToXYZ without data [complex command].
     * @param x The target x location in millimeters.
     * @param y The target y location in millimeters.
     * @param z The target z location in meters.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void moveToXYZ (double x,
                           double y,
                           double z) throws CmdInterruptedException
    {
        moveToXYZ (x, y, z, null);
    }

    /**
     * Stop the robot.
     * @throws CmdInterruptedException If a complex command is running,
     * and interruptions are not allowed.
     */
    public abstract void stop() throws CmdInterruptedException;

    /**
     * Move forward a given distance [complex command]. 
     * @param distance The distance to travel (in meters).
     * @param data Data to pass through to command listeners when this command
     * is stopped.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public abstract void moveForward (double distance,
                                      Object data)
                                      throws CmdInterruptedException;

    /**
     * Same as moveForward, without data [complex command].
     * @param distance The distance to travel (in meters).
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void moveForward (double distance) throws CmdInterruptedException
    {
        moveForward (distance, null);
    }

    /**
     * Move backward a given distance [complex command]. Same as
     * moveForward (-distance).
     * @param distance The distance to travel (in meters).
     * @param data Data to pass through to command listeners when this command
     * is stopped.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void moveBackward (double distance,
                             Object data) throws CmdInterruptedException
    {
        moveForward ( -distance, data);
    }

    /**
     * same as moveBackward, without data [complex command].
     * @param distance The distance to travel (in meters).
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void moveBackward (double distance) throws CmdInterruptedException
    {
        moveBackward (distance, null);
    }

    /**
     * Set the velocity of the robot (immediately).
     * @param newVelocity The new velocity (in meters per second).
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public abstract void setVelocity (double newVelocity)
    throws CmdInterruptedException;

    /**
     * Get the current veocty. Note that this number is the velocity
     * measured by the robot, not the velocity the robot was set to,
     * so the number returned may not be exactly the same as the number passed
     * in a setVelocity() call.
     * @return The current velocty (in meters per second).
     */
    public abstract double getVelocity();

    /**
     * Get the current position of the robot.
     * @return An array of double values representing:
     * <br>[0] The X position (in meters)
     * <br>[1] The Y position (in meters)
     * <br>[2] The Z position (in meters)
     * <br>[3] The heading (in degrees).
     */
    public abstract double[] getXYZT();

    /**
     * Tell the robot where it currently is.
     * @param newX The X position of the robot's current location, in meters.
     * @param newY The Y position of the robot's current location, in meters.
     * @param newT The angle that the robot is currently heading, in degrees.
     */
    public abstract void setXYZT (double newX,
                                  double newY,
                                  double newZ,
                                  double newT);

    /*
     * Methods to determine when complex commands finish.
     */

    /**
     * Get whether the robot is done with the current complex command.
     * @return True if the command is finished.
     */
    public boolean isDone()
    {
        return commandThread.isLastCommandDone();
    }

    /**
     * Wait until the last complex command given has finished.
     * Make sure that no other threads start new complex commands before
     * you call waitUntilDone(), or else you might be waiting for the
     * wrong command.
     */
    public void waitUntilDone()
    {
        class waitNotifyListener implements CommandListener {
            public waitNotifyListener()
            {
                done = false;
            }
            public void commandStopped (
                          CommandListener.CommandStopReason reason, Object data)
            {
                done = true;
            }
            public boolean isDone()
            {
                return done;
            }
            private boolean done;
        }
        waitNotifyListener myListener = new waitNotifyListener();
        commandThread.addCommandListener (myListener);
        while (!myListener.isDone()) {
            if ((timeout > 0) &&
                (commandThread.getTimeSinceLastCommand() > timeout)) {
                commandThread.removeCommandListener (myListener);
                return ;
            }
            sleep (50);
        }
        commandThread.removeCommandListener (myListener);
    }

    //Private/protected classes/interfaces:
    /**
     * An interface that represents a complex command to the robot. The run()
     * method is called repeatedly, with about 50ms between calls, until
     * isFinished() returns true, or another command is given by the user of
     * Robot. This interface is designed to be implemented internally and in
     * subclasses of Robot which add complex commands.
     */
    protected interface RobotCommand
    {
        /**
         * Run one loop of the command. This method is called by the RobotThread
         * commandThread repeatedly between the time it is added and it is
         * finished or stopped.
         */
        public void run();
        /**
         * Determine whether this command is finished. This method is checked by
         * the RobotThread.
         * @return Whether or not this command is finished.
         */
        public boolean isFinished();
        /**
         * Called when this command is started.
         */
        public void start();
        /**
         * Called when this command is stopped, whether it finished or not. 
         */
        public void stop();
    }

    /**
     * A class representing the internal thread that processes complex commands.
     * setCurrentCommand() is called whenever a new command is given,
     * stopCommand() is called to stop the current command, and stopRunning()
     * is called to stop this thread. Everything that deals with currentCommand
     * is synchronized because another thread could call methods that assign it
     * to a new object or to null while it is being handled internally.
     */
    protected class RobotThread extends Thread
    {
        /**
         * Create a new RobotThread.
         * @param theRobot The robot this robot thread is associated with.
         */
        public RobotThread (Robot theRobot)
        {
            super();
            myRobot = theRobot;
            done = false;
            startTime = 0;
            commandListeners = new ArrayList<CommandListener>();
            connectionLostListeners = new ArrayList<ConnectionLostListener>();
            timeout = 0;
        }

        /**
         * Run the robot thread. Loops until stopRunning() is called, constantly
         * calling the run() method of the current command, or if there is no
         * current command, doing nothing. Also checks for a lost connection.
         */
        public void run()
        {
            while (!done) {
                synchronized (this) {
                    if (currentCommand != null) {
                        currentCommand.run();
                        if (currentCommand.isFinished()) {
                            stopCommand(
                                    CommandListener.CommandStopReason.FINISHED);
                        }
                        if ((timeout > 0) &&
                            (getTimeSinceLastCommand() > timeout)) {
                            stopCommand(
                                     CommandListener.CommandStopReason.TIMEOUT);
                            try {
                                myRobot.stop();
                            }
                            catch (CmdInterruptedException e) {}
                        }
                    }
                }
                //Check to make sure the robot has not been
                //disconnected unexpectedly
                if ((!isConnected()) && (connected == true)) {
                    done = true;
                    synchronized (connectionLostListeners) {
                        for (int i = 0; i < connectionLostListeners.size(); i++)
                        {
                            connectionLostListeners.get (i).connectionLost();
                        }
                        connectionLostListeners.clear();
                    }
                    disconnect();
                }
                Robot.sleep (50);
            }
        }

        /**
         * Determine whether the last command passed in setCurrentCommand() is
         * finished, or was stopped.
         * @return True if the last command has finished.
         */
        public synchronized boolean isLastCommandDone()
        {
            return (currentCommand == null);
        }

        /**
         * Sets the current command to a new command; throws an exception if
         * interruptions are not being allowed, and another complex command is
         * already running.
         * @param newCommand The new RobotCommand to run.
         * @param data The data to associate with this complex command.
         * @throws CmdInterruptedException If a complex command is running, and
         * interruptions are not allowed.
         */
        public synchronized void setCurrentCommand (RobotCommand newCommand,
                Object data)
        throws CmdInterruptedException
        {
            if (currentCommand != null) //There is another command running
            {
                if (!commandInterruptionsAllowed) {
                    throw new CmdInterruptedException();
		}
                else {
                    //This ensures the callback is called
                    stopCommand (CommandListener.CommandStopReason.INTERRUPTED);
		}
            }
            timeout = 0;
            currentCommand = newCommand;
            startTime = System.currentTimeMillis();
            currentData = data;
            if (currentCommand != null) {
                currentCommand.start();
	    }
        }

        /**
         * Stops the current complex command, and calls all the callbacks. This
         * method is called by run() when the current command is finished, and 
         * by setCurrentCommand, when a command was interrupted, to ensure the
         * callbacks are called. isLastCommandDone() will return true after this
         * is called, until setCurrentCommand() is called again.
         * @param reason The reason for stopping the current command to pass
         * to the command listeners.
         */
        public synchronized void stopCommand(
                                       CommandListener.CommandStopReason reason)
        {
            if (currentCommand != null) {
                currentCommand.stop();
                synchronized (commandListeners) {
                    for (int i = 0; i < commandListeners.size(); i++) {
                        CommandListener thisListener =
                            (CommandListener) commandListeners.get (i);
                        CallbackThread newThread =
                                               new CallbackThread (thisListener,
                                                   reason,
                                                   currentData);
                        newThread.start();
                    }
                }
                currentCommand = null;
            }
        }

        /**
         * Return the ammount of time since setCurrentCommand() was called.
         * @return The ammount of time (in milliseconds) since
         * setCurrentCommand() was last called.
         */
        public double getTimeSinceLastCommand()
        {
            return System.currentTimeMillis() - startTime;
        }

        /**
         * Set the timeout for the currently running complex command.
         */
        public synchronized void setTimeout (double newTimeout)
        {
            timeout = newTimeout;
        }

        /**
         * Stop this thread.
         */
        public void stopRunning()
        {
            done = true;
        }

        /**
         * Add a command listener to have its commandStopped method be called
         * the next time a complex command is stopped. Accepts (and ignores)
         * null values for listener, and doesn't let duplicates of the same
         * reference into the list.
         * @param listener The CommandListener to add.
         */
        public void addCommandListener (CommandListener listener)
        {
            synchronized (commandListeners) {
                if ((listener != null) &&
                    (commandListeners.indexOf(listener) == -1)) {
                    commandListeners.add (listener);
		}
            }
        }

        /**
         * Remove a command listener from the current list of command
         * listeners.
         * @param listener The command listener to remove.
         */
        public void removeCommandListener (CommandListener listener)
        {
            synchronized (commandListeners) {
                commandListeners.remove (listener);
            }
        }

        /**
         * Clear the list of command listeners.
         */
        public void clearCommandListeners()
        {
            synchronized (commandListeners) {
                commandListeners.clear();
            }
        }

        /**
         * Add a disconnect listener, so that its connectionLost method is
         * called when the robot is unexpectedly disconnected. Accepts
         * (and ignores) null values for listener, and doesn't let duplicates
         * of the same reference into the list.
         * @param listener The MrDisconnectLilstener to add.
         */
        public void addConnectionLostListener (ConnectionLostListener listener)
        {
            synchronized (connectionLostListeners) {
                if ((listener != null) &&
                    (connectionLostListeners.indexOf(listener) == -1)) {
                    connectionLostListeners.add(listener);
		}
            }
        }

        /**
         * Remove a connection lost listener.
         * @param listener The listener to remove.
         */
        public void removeConnectionLostListener(
                                                ConnectionLostListener listener)
        {
            synchronized (connectionLostListeners) {
                connectionLostListeners.remove(listener);
            }
        }

        /**
         * A class representing a new thread for a callback to run in.
         * This is so that long callbacks don't stop up the RobotThread.
         */
        private class CallbackThread extends Thread
        {
            /**
             * Create a new CallbackThread.
             * @param listener The listener whose method should be called.
             * @param newReason The reason this complex command was stopped
             * @param newData The data to pass through to the callback.
             */
            public CallbackThread (CommandListener listener,
                                   CommandListener.CommandStopReason newReason,
                                   Object newData)
            {
                reason = newReason;
                data = newData;
                callbackListener = listener;
            }

            /**
             * Run method of this thread. Simply call the callback, then exit.
             */
            public void run()
            {
                callbackListener.commandStopped (reason, data);
            }

            private boolean finished;
            private CommandListener.CommandStopReason reason;
            private Object data;
            private CommandListener callbackListener;
        }

        private ArrayList<CommandListener> commandListeners;
        private ArrayList<ConnectionLostListener> connectionLostListeners;
        private boolean done;
        private RobotCommand currentCommand;
        private Object currentData;
        private long startTime;
        private Robot myRobot;
    }

    //Variables
    protected boolean connected; //Are we connected?
    protected RobotThread commandThread; //The internal thread
    protected double timeout; //The timeout for the current complex command.
    protected boolean commandInterruptionsAllowed; //false=throw exceptions
    //instead of allowing commands to be interrupted.
}

