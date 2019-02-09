
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import javax.swing.*;

public class RobotTest
{
    public static void main(String[] args)
    {

        final MrSonar myRobot = new MrSonar();

        MrRobot.init();

        if (!myRobot.connect()) {
            System.out.println("Couldn't connect, exiting....");
            System.exit(1);
        }

        class MyLostListener implements ConnectionLostListener {
            public void connectionLost()
            {
                System.out.println("OH NO!");
                System.exit(1);
            }
        }
        myRobot.addConnectionLostListener(new MyLostListener());

        JFrame mainFrame = new JFrame();
        class EmergencyStopListener implements KeyListener {
            public void keyPressed(KeyEvent e)
            {
                myRobot.stopCurrentCommand();
                try {
                    myRobot.emergencyStop();
                }
                catch (CmdInterruptedException ex) {}
                myRobot.disconnect();
                MrRobot.cleanup();
                System.exit(0);
            }

            public void keyReleased(KeyEvent e)
            {}
            public void keyTyped(KeyEvent e)
            {}
        }

        mainFrame.addKeyListener(new EmergencyStopListener());
        mainFrame.add(new JLabel("Press any key to emergency stop."));
        mainFrame.pack();
        mainFrame.show();

        System.out.println("Current voltage: " + myRobot.getCurrentVoltage());

        double[] it;

        class MyListener implements CommandListener {
            public void commandStopped(CommandListener.CommandStopReason reason,
	                               Object data)
            {
                String s = "Command [" + data + "] stopped because it ";
                switch (reason) {
                case FINISHED:
                    s += "finished.";
                    break;
                case INTERRUPTED:
                    s += "was interrupted.";
                    break;
                case TIMEOUT:
                    s += "timed out.";
                    break;
                default:
                    s += "???";
                }
                System.out.println(s);
            }
        }
        myRobot.addCommandListener(new MyListener());
        try {
            myRobot.moveToXYZ(5000, 5000, 0, "move far");
            Robot.sleep(500);
            myRobot.turnTo(0);
        }
        catch (CmdInterruptedException e) {
            System.out.println("Command already running!");
        }
        Robot.sleep(500);
        myRobot.allowInterruptions(true);
        try {
            System.out.println("Stopping now.");
            myRobot.stop();
            Robot.sleep(1000);

            System.out.println("move to 0, 2");
            myRobot.moveToXYZ(0, 2, 0, "moveToXYZ");
            myRobot.waitUntilDone();
            myRobot.setXYZT(0, 0, 0, 0);
            System.out.println("turn left 90 degerees");
            myRobot.turnTo(90);
            myRobot.waitUntilDone();
            myRobot.setTurnDoneAngle(100);
            myRobot.turnTo(0, "...doing nothing...");
            myRobot.waitUntilDone();
            System.out.println("turn right 90 degrees, then left");
            myRobot.setTurnDoneAngle(3);
            myRobot.turnRelative( -90, "right");
            myRobot.waitUntilDone();
            myRobot.turnRelative(90, "left");
            myRobot.waitUntilDone();
            System.out.println("Left");
            myRobot.turnTowards( -1, 0);
            myRobot.waitUntilDone();
            System.out.println(myRobot.getDistanceTo( -1, 0) + " ~ 1");
            myRobot.setMoveDoneDistance(5);
            myRobot.moveForward(1, "...nothing again...");
            myRobot.waitUntilDone();
            myRobot.setMoveDoneDistance(.100);
            System.out.println("backward, forward");
            myRobot.moveBackward(2, "moving backward");
            myRobot.setTimeout(1000);
            myRobot.waitUntilDone();
            myRobot.stop();
            Robot.sleep(1000);
            myRobot.moveForward(2, "moving forward");
            myRobot.setTimeout(6000);
            myRobot.waitUntilDone();
            Robot.sleep(1000);
            System.out.println("crazy");
            myRobot.setMotorVelocity(.1, .2);
            Robot.sleep(2000);
            myRobot.stop();
            Robot.sleep(500);
            System.out.println("turning <-");
            myRobot.setRotationalVelocity(100);
            Robot.sleep(5000);
            myRobot.stop();
            System.out.println("before+after:");
            it = myRobot.getXYZT();
            System.out.println(it[0] + ", " + it[1] + ", " + it[3]);
            myRobot.setXYZT(10, 20, 0, 30);
            it = myRobot.getXYZT();
            System.out.println(it[0] + ", " + it[1] + ", " + it[3]);
        }
        catch (CmdInterruptedException e) {}

        myRobot.disconnect();
        MrRobot.cleanup();

        System.exit(0);
    }

    private static int mode;
}

