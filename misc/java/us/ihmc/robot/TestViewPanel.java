
import javax.swing.*;
import java.awt.*;

public class TestViewPanel extends JPanel
{
    public TestViewPanel(MrSonar robot)
    {
        setPreferredSize(new Dimension(500, 500));
        myRobot = robot;
    }

    public void start()
    {
        myThread = new ViewUpdateThread();
        myThread.start();
    }

    public void stop()
    {
        myThread.stopRunning();
    }

    public void paint(Graphics g)
    {
        super.paint(g);
        PointWithAge[] data = myRobot.getCumulativeSonarData();
        g.setColor(Color.WHITE);
        g.fillRect(0, 0, 500, 500);
        int[] pos;
        for (int i = 0; i < data.length; i++) {
            PointWithAge thisReading = data[i];
            int newRed = 255;
            if (thisReading.getAge() <= 100) {
                newRed = Math.round((255 / 100) * thisReading.getAge());
	    }
            g.setColor(new Color(newRed, 0, 0));
            pos = convertXY(thisReading.getX(), thisReading.getY());
            g.fillOval(pos[0] - 3, pos[1] - 3, 6, 6);
        }
        g.setColor(Color.BLUE);
        double[] robotPos = myRobot.getXYZT();
        pos = convertXY(robotPos[0] * 1000, robotPos[1] * 1000);
        g.fillOval(pos[0] - 5, pos[1] - 5, 10, 10);
    }

    private int[] convertXY(double inX, double inY) //TAKES mm !!
    {
        int[] result = new int[2];
        result[0] = ((int)Math.round(inX / 50)) + 250;
        result[1] = -((int)Math.round(inY / 50)) + 250;
        return result;
    }

    private class ViewUpdateThread extends Thread
    {
        public ViewUpdateThread()
        {
            done = false;
        }

        public void run()
        {
            while (!done) {
                repaint();
                try {
                    sleep(500);
                }
                catch (InterruptedException e) {}
            }
        }

        public void stopRunning()
        {
            done = true;
        }

        private boolean done;
    }

    private MrSonar myRobot;
    private ViewUpdateThread myThread;
}

