/*
* MrRobot.java
* by Kristopher Brosch
*/

import java.util.ArrayList;

/**
 * <p>The MrRobot class represents a Mobile Robot robot, which can be controlled
 * using ARIA. Normally, one would call MrRobot.init() at the beginning of the
 * program, create a new robot, then connect() it, use it, and when done with
 * the robot, disconnect() it. At the end of the program, MrRobot.cleanup()
 * should be called. init() and cleanup() only need to be called once each;
 * each individual MrRobot needs to be connect()ed and disconnect()ed
 * seperately.</p><p>Also see the documentation on Robot.</p>
 */
public class MrRobot extends Robot
{
    static
    {
        try
        {
            System.loadLibrary ("AriaJava");
        }
        catch (UnsatisfiedLinkError e)
        {
            System.out.println ("Couldn't load AriaJava library, exiting.");
            System.exit (1);
        }
    }

    /*
     * Control methods
     */

    /**
     * Creates a new MrRobot. Make sure you have called init(), to initialize
     * Aria, before creating or using any robots.
     */
    public MrRobot()
    {
        robot = new ArRobot ("robot", true, true, true, true);
        zPosition = 0;
        velL = velR = 0;
        commandInterruptionsAllowed = false;
    }

    /**
     * <p>Initializes Aria. This method should be called before creating any
     * MrRobots; it only needs to be called once.</p><p>It would be nice to
     * have "abstract static" init() and cleanup() methods in Robot (so that
     * you could say Robot.init(), and have it call MrRobot.init()), but Java
     * does not allow this; you're going to have to call MrRobot.init() and
     * MrRobot.cleanup() after all.</p>
     */
    public static void init()
    {
        Aria.init (Aria.SigHandleMethod.SIGHANDLE_NONE, true, true);
    }

    /**
     * Shuts down Aria. This method must be called before the end
     * of the program, but after all MrRobots are disconnected.
     */
    public static void cleanup()
    {
        Aria.shutdown();
    }

    /**
     * Tries to connect to the simulator on port 8101, and if that doesn't work,
     * tries to connect to COM1.
     * @return True if the connect to either the simulator or COM1 was 
     * succesful.
     */
    public boolean connect()
    {
        if (connect ("localhost", 8101)) {
            return true;
        }
        return connect (1);
    }

    /**
     * Connects to a robot over the network. One of the connect 
     * methods must be called before any other methods except
     * init() and the constructor.
     * @param host The host to connect to.
     * @param port The port to connect on; Aria usually uses port 8101.
     * @return True if the connect was succesful.
     */
    public boolean connect (String host, int port)
    {
        if (isConnected()) {
            disconnect();
        }
        int code;
        connection = new ArTcpConnection();
        robot.setDeviceConnection (connection);
        connected = (((code = ((ArTcpConnection)connection).open(host, port)) ==
                      0) && (robot.blockingConnect()));
        if (connected) {
            connectedSetup();
        }
        return connected;
    }

    /**
     * Connects to a robot on a serial port. One of the connect
     * methods must be called before any other methods except
     * init() and the constructor.
     * @param com A number representing the COM port to connect using
     * (1, 2, 3, or 4, representing COM1, COM2, COM3, or COM4, respectively).
     * @return True if the connect was succesful.
     */
    public boolean connect (int com)
    {
        if (isConnected()) {
            disconnect();
        }
        int code;
        String port = ArUtil.getCOM1();
        switch (com) {
        case 1:
            port = ArUtil.getCOM1();
            break;
        case 2:
            port = ArUtil.getCOM2();
            break;
        case 3:
            port = ArUtil.getCOM3();
            break;
        case 4:
            port = ArUtil.getCOM4();
            break;
        default:
            System.out.println("Can't use COM" + com + ". Defaulting to COM1.");
            break;
        }
        connection = new ArSerialConnection();
        robot.setDeviceConnection (connection);
        connected = (((code = ((ArSerialConnection)connection).open(port)) ==
                     0) && (robot.blockingConnect()));
        if (connected) {
            connectedSetup();
        }
        return connected;
    }

    /**
     * Setup internal data after succesfully connected.
     */
    private void connectedSetup()
    {
        robot.runAsync (false);
        //Run internal thread
        commandThread = new RobotThread(this);
        commandThread.start();
        robot.lock();
        robot.comInt ((short)92, (short)0); //Turn off sounds (for Amigo)
        robot.enableMotors();
        robot.setVel (0);
        robot.unlock();
    }

    /**
     * Disconnects from the actual robot or simulator.
     */
    public void disconnect()
    {
        if (connected) {
            connected = false;
            commandThread.stopRunning(); //Stop the internal thread
            robot.stopRunning (true); //stop running and disconnect
        }
    }

    /**
     * Get whether the robot is connected or not.
     * @return True if the robot is connected.
     */
    public boolean isConnected()
    {
        if (connection != null) {
            return connection.getStatus() ==
                   (int) (ArDeviceConnection.Status.STATUS_OPEN.swigValue());
        }
        else {
            return false;
        }
    }

    /**
     * Get the instentanious battery voltage, normalized to a 12 volt system.
     * Note, since the value is normalized, this value may not be the actual
     * voltage.
     * @return The current voltage.
     */
    public double getCurrentVoltage()
    {
        robot.lock();
        double voltage = robot.getBatteryVoltageNow();
        robot.unlock();
        return voltage;
    }

    /**
     * Set the turn done angle, in degrees. When turning, the
     * robot must be less than this ammount away from the target angle to
     * consider itslef done turning.
     * @param angle The new turn done angle, in degrees.
     */
    public void setTurnDoneAngle (double angle)
    {
        robot.lock();
        robot.setHeadingDoneDiff (angle);
        robot.unlock();
    }

    /**
     * Get the turn done angle. When turning to a given angle, the
     * robot must be less than this ammount away from the target angle to
     * consider itslef done turning.
     * @return The turn done angle, in degrees.
     */
    public double getTurnDoneAngle()
    {
        robot.lock();
        double angle = robot.getHeadingDoneDiff();
        robot.unlock();
        return angle;
    }

    /**
     * Set the move done distance, in milimeters. When moving a given distance,
     * the robot must be less than this ammout away from the target distance to
     * consider itself done moving.
     * @param distance The new move done distance, in meters.
     */
    public void setMoveDoneDistance (double distance)
    {
        robot.lock();
        robot.setMoveDoneDist (distance * 1000);
        robot.unlock();
    }

    /**
     * Get the move done distance, in meters. When moving a given distance, the
     * robot must be less than this ammout away from the target distance to
     * consider itself done moving.
     * @return The move done distance, in meters.
    */
    public double getMoveDoneDistance()
    {
        robot.lock();
        double dist = robot.getMoveDoneDist() / 1000;
        robot.unlock();
        return dist;
    }

    /**
     * Set the maximum translational velocity of the robot. This is also the
     * velocity the robot will move at if given a moveForward/Backward command.
     * @param vel The new maximum translational velocity, in meters/second.
     */
    public void setMaxVelocity (double vel)
    {
        robot.lock();
        robot.setTransVelMax (vel * 1000);
        robot.unlock();
    }

    /**
     * Gets the maximum translational velocity of the robot.
     * @return The maximum translational velocity of the robot, in
     * meters/second.
     */
    public double getMaxVelocity()
    {
        robot.lock();
        double vel = robot.getTransVelMax() / 1000;
        robot.unlock();
        return vel;
    }

    /**
     * Set the maximum rotational velocity of the robot.
     * @param vel The new maximum rotational velocity in degrees/second.
     */
    public void setMaxRotVelocity (double vel)
    {
        robot.lock();
        robot.setRotVelMax (vel);
        robot.unlock();
    }

    /**
     * Gets the maximum rotational velocity of the robot.
     * @return The maximum rotational velocity of the robot in degrees/second.
     */
    public double getMaxRotVelocity()
    {
        robot.lock();
        double vel = robot.getRotVelMax();
        robot.unlock();
        return vel;
    }

    /*
     * Motion methods
     */

    /**
     * A simple method that moves the robot toward a given location
     * [complex command]. There is no obstacle avoidance.
     * @param x The target x location in meters.
     * @param y The target y location in meters.
     * @param z The z coordinate is meaningless for a land robot.
     * @param data Data to pass through to command listeners when this command
     * is stopped.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void moveToXYZ (double x,
                          double y,
                          double z,
                          Object data) throws CmdInterruptedException
    {
        zPosition = z;
        commandThread.setCurrentCommand (
                               new MoveToXYZCommand (x * 1000, y * 1000), data);
    }

    /**
     * Stop the robot.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void stop() throws CmdInterruptedException
    {
        commandThread.setCurrentCommand (null, null);
        robot.lock();
        robot.stop();
        robot.unlock();
    }

    /**
     * Stop the robot immediately (lock the motors to force a stop). You
     * probably will want to call stopCurrentCommand() before calling this
     * method; otherwise, if interruptions are not allowed,
     * you will get an exception instead of stopping the robot.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void emergencyStop() throws CmdInterruptedException
    {
        commandThread.setCurrentCommand (null, null);
        robot.lock();
        robot.stop(); //Just in case
        robot.com ((short)55); //Emergency stop signal
        robot.unlock();
    }

    /**
     * Get the distance to a given position on the same plane as the robot. 
     * @param x The x coordinate of the given position, in meters.
     * @param y The y coordinate of the given position, in meters.
     * @return The distance (in meters) to that position.
     */
    public double getDistanceTo (double x, double y)
    {
        ArPose targetPose = new ArPose (x * 1000, y * 1000, 0);
        robot.lock();
        double dist = robot.findDistanceTo (targetPose) / 1000;
        robot.unlock();
        return dist;
    }

    /**
     * Get the absolute angle to a given position. Note this is the angle from
     * zero, not from the robot's current angle.
     * @param x The x coordinate of the given position, in meters.
     * @param y The y coordinate of the given position, in meters.
     * @return The absolute angle, in degrees, that points toward the given
     * location from the robot's current location.
     */
    public double getAngleTo (double x, double y)
    {
        ArPose targetPose = new ArPose (x * 1000, y * 1000, 0);
        robot.lock();
        double angle = robot.findAngleTo (targetPose);
        robot.unlock();
        return angle;
    }

    /**
     * Get the relative angle to a given position. Note this is the angle from
     * the robot's current heading, not from zero. An angle to the left of the
     * robot is positive.
     * @param x The x coordinate of the given position, in meters.
     * @param y The y coordinate of the given position, in meters.
     * @return The relative angle, in degrees, that points toward the given
     * location from the robot's current location.
     */
    public double getRelativeAngleTo (double x, double y)
    {
        ArPose targetPose = new ArPose (x * 1000, y * 1000, 0);
        robot.lock();
        double angle = robot.findDeltaHeadingTo (targetPose);
        robot.unlock();
        return angle;
    }

    /**
     * Turn to a given absolute angle [complex command]. Use isDone(),
     * waitUntilDone(), or a CommandListener to determine when the robot is
     * done turning.
     * @param angle The angle to turn to (in degrees).
     * @param data Data to pass through to command listeners when this command
     * is stopped.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void turnTo (double angle,
                        Object data) throws CmdInterruptedException
    {
        commandThread.setCurrentCommand (new TurnCommand(angle, false), data);
    }

    /**
     * Same as turnTo, without data [complex command].
     * @param angle The angle to turn to (in degrees).
     * @throws CmdInterruptedException If a complex command is running,
     * and interruptions are not allowed.
     */
    public void turnTo (double angle) throws CmdInterruptedException
    {
        turnTo (angle, null);
    }

    /**
     * Turn the robot towards a given position [complex command]. Use isDone(),
     * waitUntilDone(), or a CommandListener to determine when the robot is
     * done turning.
     * @param x The x coordinate of the position to turn to, in meters.
     * @param y the y coordinate of the position to turn to, in meters.
     * @param data Data to pass through to command listeners when this command
     * is stopped.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void turnTowards (double x,
                             double y,
                             Object data) throws CmdInterruptedException
    {
        turnTo (getAngleTo(x, y), data);
    }

    /**
     * Same as turnTowards without data [complex command].
     * @param x The x coordinate of the position to turn to, in meters.
     * @param y the y coordinate of the position to turn to, in meters.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void turnTowards (double x, double y) throws CmdInterruptedException
    {
        turnTowards (x, y, null);
    }

    /**
     * Turn a specified number of degrees [complex command]. Positive angles
     * are to the left of the robot;
     * negative angles are to the right of the robot. The robot will decide
     * which direction to turn.
     * Use isDone(), waitUntilDone(), or a CommandListener
     * to determine when the robot is done turning. 
     * @param angle The number of degrees to turn.
     * @param data Data to pass through to command listeners when this command
     * is stopped.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void turnRelative (double angle,
                             Object data) throws CmdInterruptedException
    {
        commandThread.setCurrentCommand (new TurnCommand(angle, true), data);
    }

    /**
     * Same as turnRelative, without data [complex command].
     * @param angle The number of degrees to turn.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void turnRelative (double angle) throws CmdInterruptedException
    {
        turnRelative (angle, null);
    }

    /**
     * Move forward a given distance [complex command]. 
     * @param distance The distance to travel (in meters).
     * @param data Data to pass through to command listeners when this command
     * is stopped.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void moveForward (double distance,
                            Object data) throws CmdInterruptedException
    {
        commandThread.setCurrentCommand (
                                 new MoveForwardCommand(distance * 1000), data);
    }

    /**
     * Set the velocity of the robot (immediately).
     * @param newVelocity The new velocity (in meters per second).
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void setVelocity (double newVelocity) throws CmdInterruptedException
    {
        commandThread.setCurrentCommand (null, null);
        velL = velR = newVelocity * 1000;
        fixVels();
        robot.lock();
        robot.setVel (velL); //Use one of the values that was fixed
        robot.unlock();
    }

    /**
     * Get the current veocty. Note that this number is the velocity measured by
     * the robot, not the velocity the robot was set to, so the number returned
     * may not be exactly the same as the number passed in a setVelocity() call.
     * @return The current velocty (in meters per second).
     */
    public double getVelocity()
    {
        robot.lock();
        double vel = robot.getVel() / 1000;
        robot.unlock();
        return vel;
    }

    /**
     * Set the velocity of each motor seperately.
     * @param newVelL The new velocity of the left motor (meters/second).
     * @param newVelR The new velocity of the right motor (meters/second).
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void setMotorVelocity (double newVelL, double newVelR)
                                                  throws CmdInterruptedException
    {
        commandThread.setCurrentCommand (null, null);
        velL = newVelL * 1000;
        velR = newVelR * 1000;
        fixVels();
        robot.lock();
        robot.setVel2 (velL, velR);
        robot.unlock();
    }

    /**
     * Set the rotational velocity (immediately). A positive value turns left.
     * @param newVel The new rotational velocity, in degrees/second.
     * @throws CmdInterruptedException If a complex command is running, and
     * interruptions are not allowed.
     */
    public void setRotationalVelocity (double newVel)
                                                  throws CmdInterruptedException
    {
        commandThread.setCurrentCommand (null, null);
        //For some reason, I don't think Aria checks whether the new rot.
        //velocity is in range.
        double maxRotVel = getMaxRotVelocity();
        if (newVel > maxRotVel)
            newVel = maxRotVel;
        if (newVel < -maxRotVel)
            newVel = -maxRotVel;
        robot.lock();
        robot.setRotVel (newVel);
        robot.unlock();
    }

    /**
     * Get the current position of the robot.
     * @return An array of double values representing:
     * <br>[0] The X position (in meters)
     * <br>[1] The Y position (in meters)
     * <br>[2] The Z position (in meters) (always is zero for land robots)
     * <br>[3] The heading (in degrees).
     */
    public double[] getXYZT()
    {
        robot.lock();
        ArPose robotPose = robot.getPose();
        robot.unlock();
        double[] data = new double[4];
        data[0] = robotPose.getX() / 1000;
        data[1] = robotPose.getY() / 1000;
        data[2] = zPosition;
        data[3] = robotPose.getTh();
        return data;
    }

    /**
     * Tell the robot where it currently is.
     * @param newX The X position of the robot's current location, in meters.
     * @param newY The Y position of the robot's current location, in meters.
     * @param newZ Ignored.
     * @param newT The angle that the robot is currently heading, in degrees.
     */
    public void setXYZT (double newX, double newY, double newZ, double newT)
    {
        ArPose newPose = new ArPose (newX * 1000, newY * 1000, newT);
        robot.lock();
        robot.moveTo (newPose, true);
        robot.unlock();
        zPosition = newZ;
    }

    //Private methods:
    /**
     * Ensure that velL and velR are within the range [-getMaxVelocity(),
     * getMaxVelocity()].
     */
    private void fixVels()
    {
        double maxVel = getMaxVelocity() * 1000;
        if (velL > maxVel)
            velL = maxVel;
        else if (velL < -maxVel)
            velL = -maxVel;
        if (velR > maxVel)
            velR = maxVel;
        else if (velR < -maxVel)
            velR = -maxVel;
    }

    //Private/protected classes/interfaces:

    /**
     * RobotCommand that represents a turn command. There is no implementation
     * of run(), as Aria takes care of the turn command.
     */
    private class TurnCommand implements RobotCommand
    {
        /**
         * Create a new TurnCommand
         * @param newAngle The angle to turn.
         * @param turnRelative Whether or not to turn the given angle relative
         * to the current angle.
         */
        public TurnCommand (double newAngle, boolean turnRelative)
        {
            angle = newAngle;
            delta = turnRelative;
        }
        /**
         * Called by the RobotTread; do nothing, since Aria is handling the
         * turn.
         */
        public void run ()
        {}
        /**
         * Determine whether the turn is finished.
         * @return True if the turn is finished.
         */
        public boolean isFinished()
        {
            robot.lock();
            boolean finished = robot.isHeadingDone (0);
            robot.unlock();
            return finished;
        }
        /**
         * Set up the turn.
         */
        public void start()
        {
            robot.lock();
            //It's more consistent to stop before and after turning.
            robot.stop();
            if (delta)
                robot.setDeltaHeading (angle);
            else
                robot.setHeading (angle);
            robot.unlock();
        }
        /**
         * stop.
         */
        public void stop()
        {
            robot.lock();
            //Stop the robot after the turn is stopped; if we don't call stop
            //here, a call to setXYZT() might cause the robot to turn.
            robot.stop();
            robot.unlock();
        }

        private double angle;
        private boolean delta;
    }

    /**
     * RobotCommand representing a command to move forward a given distance.
     */
    private class MoveForwardCommand implements RobotCommand
    {
        /**
         * Create a new MoveForwardCommand.
         * @param newDistance The distance to move forward, or backward
         * (if given a negative value), in millimeters.
         */
        public MoveForwardCommand (double newDistance)
        {
            distance = newDistance;
        }

        /**
         * Continue moving the robot.
         */
        public void run()
        {
            robot.lock();
            double newVel = Math.sqrt ((Math.abs(distance) -
                                        robot.findDistanceTo(startPose)) * 400);
            robot.unlock();
            if (newVel > maxVel)
                newVel = maxVel;
            if (distance < 0)
                newVel = -newVel;
            robot.lock();
            robot.setVel (newVel);
            robot.unlock();
        }

        /**
         * Determine whether the robot is done moving forward.
         * @return true if the robot is done.
         */
        public boolean isFinished()
        {
            return (Math.abs(distance) - robot.findDistanceTo(startPose) <
                    doneDist);
        }

        /**
         * Set up the move forward command.
         */
        public void start()
        {
            robot.lock();
            startPose = robot.getPose();
            robot.unlock();
            maxVel = getMaxVelocity() * 1000;
            doneDist = getMoveDoneDistance() * 1000;
        }

        /**
         * Stop the robot, when it's done.
         */
        public void stop()
        {
            robot.lock();
            robot.stop();
            robot.unlock();
        }

        private ArPose startPose;
        private double maxVel;
        private double doneDist;
        private double distance;
    }

    /**
     * A RobotCommand representing a command to move to a given (x,y) position.
     */
    private class MoveToXYZCommand implements RobotCommand
    {
        /**
         * create a new MoveToXYZCommand.
         * @param newX The target X position (in millimeters).
         * @param newY The target Y position (in millimeters).
         */
        public MoveToXYZCommand (double newX, double newY)
        {
            x = newX;
            y = newY;
        }

        /**
         * Continue moving toward the given position.
         */
        public void run()
        {
            if (stage == 0) //This the first time run() is called. Turn the
                            //robot towards the target.
            {
                robot.lock();
                robot.setHeading (robot.findAngleTo(target));
                robot.unlock();
                stage++;
            }
            else if (stage == 1) //The robot is still doing its initial turn;
                                 //keep turning.
            {
                robot.lock();
                if (robot.isHeadingDone(0))
                    stage++;
                robot.unlock();
            }
            else //The robot should be moving and turning actively towards
                 //the target.
            {
                double newVel = Math.sqrt (getDistanceTo(
                                              x / 1000, y / 1000) * 1000 * 400);
                if (newVel > maxVel)
                    newVel = maxVel;
                robot.lock();
                robot.setVel (newVel);
                robot.setHeading (robot.findAngleTo(target));
                robot.unlock();
            }
        }

        /**
         * Determine whether the robot has reached the target position.
         * @return True if the robot is done moving to the target position.
         */
        public boolean isFinished()
        {
            return (getDistanceTo(x / 1000, y / 1000) * 1000 < doneDist);
        }

        /**
         * Set up this command
         */
        public void start()
        {
            maxVel = getMaxVelocity() * 1000;
            doneDist = getMoveDoneDistance() * 1000;
            target = new ArPose (x, y, 0);
            stage = 0;
            robot.lock();
            robot.stop();
            robot.unlock();
        }

        /**
         * Stop the robot when it's done
         */
        public void stop()
        {
            robot.lock();
            robot.stop();
            robot.unlock();
        }

        private ArPose target;
        private double x, y;
        private double doneDist;
        private double maxVel;
        private int stage;
    }

    //Variables
    protected double zPosition; //The Z position this MrRobot says it is at
    //(meters).
    protected ArRobot robot; //The main robot
    private ArDeviceConnection connection; //The connection for the robot;
    //must be here or else it will be garbage-collected!
    private double velL, velR; //Velocities of left and right wheels
}

