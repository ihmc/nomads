/*
* MrSonar.java
* by Kristopher Brosch
*/

/**
 * The MrSonar class represents a Mobile Robots robot with sonar. It should be
 * used in the same way as a MrRobot; this class only adds sonar functionality
 * to MrRobot. Two lists of sonar readings are kept, and can be retrieved: a
 * "current" list, and a "cumulative" list. The maximum size of these two lists
 * can be changed; by default they are 24 and 64, respectively. Note that the
 * moveToXYAvoid complex command uses these lists, so changing their sizes may
 * affect its behavior.
 */
public class MrSonar extends MrRobot
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


    /**
     * Create a new MrSonar.
     */
    public MrSonar()
    {
        super();
        sonar = new ArSonarDevice (24, 64, "sonar");
        robot.addRangeDevice (sonar);
    }

    /**
     * Move to a given x,y position, and avoid obstacles [complex command].
     * @param x The x position to move to (in meters)
     * @param y The y position to move to (in meters)
     * @param data Data to pass through to command listeners when this command
     * is stopped.
     */
    public synchronized void moveToXYAvoid (double x, double y, Object data)
    throws CmdInterruptedException
    {
        commandThread.setCurrentCommand(
                               new MoveToXYAvoidCommand (x*1000, y*1000), data);
    }

    /**
     * Same as moveToXYAvoid, without data.
     * @param x The x position to move to
     * @param y The y position to move to
     */
    public void moveToXYAvoid (double x,
                               double y) throws CmdInterruptedException
    {
        moveToXYAvoid (x, y, null);
    }

    /**
     * Get the distance and angle to the closest "current" sonar reading
     * within a range. The range that is considered is the range from the start
     * angle, counter-clockwise to the end angle.
     * @param startAngle The starting angle defining the range.
     * @param endAngle The end angle defining the range.
     * @return A double[] representing the distance to (at index 0), and the
     * angle to (at index 1) the closest current sonar reading.
     */
    public double[] getSonarClosest (double startAngle, double endAngle)
    {
        //I have to pretty much re-implement this, instead of calling
        //an ARIA method,
        //because the ARIA method for it returns one variable through
        //a pointer as a
        //parameter, which doesn't mix with Java. Ugh.
        double[] info = new double[2];
        ArPoseWithTimeVector buffer;
        sonar.lockDevice();
        buffer = sonar.getCurrentBufferAsVector();
        double maxRange = sonar.getMaxRange();
        sonar.unlockDevice();

        startAngle = ArMath.fixAngle (startAngle);
        endAngle = ArMath.fixAngle (endAngle);
        robot.lock();
        ArPose robotPose = robot.getPose();
        robot.unlock();
        ArPoseWithTime closest = null;
        for (int i = 0; i < buffer.size(); i++) {
            ArPoseWithTime thisReading = buffer.get (i);
            robot.lock();
            double relativeAngle = ArMath.subAngle(
                                 robot.findAngleTo(thisReading), robot.getTh());
            robot.unlock();
            //If the angle of this reading is within the range we're looking at,
            //and either closest doesn't exist yet, or
            //this reading is closer than closest, then closest = thisReading.
            if ((ArMath.angleBetween(relativeAngle, startAngle, endAngle)) &&
                ((closest == null) || (thisReading.findDistanceTo(robotPose) < 
                 closest.findDistanceTo(robotPose)))) {
                closest = thisReading;
            }
        }

        if ((closest == null) || (closest.findDistanceTo(robotPose) > maxRange))
        {
            //There must not have been any readings in range
            info[0] = 0;
            info[1] = 0;
            return info;
        }

        robot.lock();
        info[0] = robot.findDistanceTo (closest);
        info[1] = robot.findAngleTo (closest);
        robot.unlock();
        return info;
    }

    /**
     * Set the maximum number of readings to remember in the "current" list.
     * @param newSize The new maximum number of readings to remember.
     */
    public void setCurrentBufferSize (int newSize)
    {
        sonar.lockDevice();
        sonar.setCurrentBufferSize (newSize);
        sonar.unlockDevice();
    }

    /**
     * Set the maximum number of readings to remember in the "cumulative" list.
     * @param newSize The new maximum number of readings to remember.
     */
    public void setCumulativeBufferSize (int newSize)
    {
        sonar.lockDevice();
        sonar.setCumulativeBufferSize (newSize);
        sonar.unlockDevice();
    }

    /**
     * Get the current information read by the sonar. Returns an array of
     * PointWithAge's representing the latest information read from the sonar.
     * Note that if you call setXYT(), any data you had obtained from this
     * method before the call to setXYT() will be incorrect in the new
     * coordinate system, however, a new call to this method will return
     * good data.
     * @return An array of PointWithAge's, representing information from the
     * sonar.
     */
    public PointWithAge[] getCurrentSonarData()
    {
        double z = getXYZT()[2];
        sonar.lockDevice();
        ArPoseWithTimeVector buffer = sonar.getCurrentBufferAsVector();
        sonar.unlockDevice();
        PointWithAge[] data = new PointWithAge[(int)buffer.size()];
        for (int i = 0; i < buffer.size(); i++) {
            ArPoseWithTime thisPoint = buffer.get(i);
            data[i] = new PointWithAge(thisPoint.getX(),
                                       thisPoint.getY(),
                                       z,
                                       thisPoint.getTime().mSecSince());
        }
        return data;
    }

    /**
     * Get the cumulative information read by the sonar. Returns an array of
     * PointWithAge's representing the cumulative information read from the
     * sonar. Note that if you call setXYT(), any data you had obtained from
     * this method before the call to setXYT() will be incorrect in the new
     * coordinate system, however, a new call to this method will return good
     * data.
     * @return An array of PointWithAge's, representing information from the
     * sonar.
     */
    public PointWithAge[] getCumulativeSonarData()
    {
        double z = getXYZT()[2];
        sonar.lockDevice();
        ArPoseWithTimeVector buffer = sonar.getCumulativeBufferAsVector();
        sonar.unlockDevice();
        PointWithAge[] data = new PointWithAge[(int)buffer.size()];
        for (int i = 0; i < buffer.size(); i++) {
            ArPoseWithTime thisPoint = buffer.get(i);
            data[i] = new PointWithAge(thisPoint.getX(),
               thisPoint.getY(),
               z,
               thisPoint.getTime().mSecSince());
        }
        return data;
    }


    /**
     * Class that represents the complex command to move to a given
     * x,y position, while avoiding obstacles.
     */
    private class MoveToXYAvoidCommand implements RobotCommand
    {
        /**
         * Constructor for MoveToXYAvoidCommand. Set up the complex command.
         * @param newX The X (in millimeters)
         * @param newY The Y (in millimeters)
         */
        public MoveToXYAvoidCommand (double newX, double newY)
        {
            x = newX;
            y = newY;
            finished = false;
            stage = 0;
        }

        /**
         * Either handle the initial turn, or do nothing, because the actual
         * motion is handled by Aria actions.
         */
        public void run()
        {
            if (stage == 0) {
                //First, do a turn towards the target;
                //most of the time this is more efficient than
                //just jumping right into the actions.
                avoidNearAction.deactivate();
                avoidAction.deactivate();
                gotoAction.deactivate();
                robot.lock();
                robot.setHeading (robot.findAngleTo(target));
                robot.unlock();
                stage++;
            }
            else if (stage == 1) //The robot is still doing its initial turn;
            //keep turning.
            {
                robot.lock();
                if (robot.isHeadingDone (0)) {
                    avoidNearAction.activate();
                    avoidAction.activate();
                    gotoAction.activate();
                    robot.clearDirectMotion();
                    stage++;
                }
                robot.unlock();
            }
            //If the stage == 2, do nothing.
        }

        /**
         * Get whether the robot has reached its destination.
         * @return True if the robot has reached its target x,y position.
         */
        public boolean isFinished()
        {
            return gotoAction.haveAchievedGoal(); //finished;
        }

        /**
         * Set up this command: add action code to create the
         * behavior of moving toward a point, and avoiding obstacles.
         */
        public void start()
        {
            doneDist = getMoveDoneDistance();
            target = new ArPose (x, y, 0);
            //The following constructor calls are where some "tweaking" can be
            //done....
            avoidNearAction = new ArActionAvoidFront ("avoid near",
                                                      225,
                                                      0,
                                                      15,
                                                      false);
            avoidAction = new ArActionAvoidFront ("avoid",
                                                  450,
                                                  200,
                                                  15,
                                                  false);
            gotoAction = new ArActionGoto ("goto",
                                           target,
                                           /*doneDist*/100,
                                           /*getMaxVelocity()*/400,
                                           getMaxRotVelocity(),
                                           7);
            robot.lock();
            robot.stop();
            robot.addAction (avoidNearAction, 100);
            robot.addAction (avoidAction, 75);
            robot.addAction (gotoAction, 50);
            robot.unlock();
        }

        /**
         * Clean up the Actions code used to make the robot perform this
         * command.
         */
        public void stop()
        {
            robot.lock();
            robot.remAction (gotoAction);
            robot.remAction (avoidAction);
            robot.remAction (avoidNearAction);
            robot.stop();
            robot.unlock();
        }

        private double x, y, doneDist, maxVel;
        private ArActionAvoidFront avoidAction, avoidNearAction;
        private ArActionGoto gotoAction;
        private boolean finished;
        private ArPose target;
        private int stage;
    }

    protected ArSonarDevice sonar;
}
