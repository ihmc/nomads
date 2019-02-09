
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.util.*;

public class TestViewPanel4 extends JPanel
{
    public TestViewPanel4()
    {
        setPreferredSize(new Dimension(500, 500));
        robots = new ArrayList();
        trackers = new ArrayList();
        myThread = new ViewUpdateThread();
        centerX = centerY = 250;
        tracking = false;
        setupMouseStuff();
        myThread.start();
    }

    private void setupMouseStuff()
    {
        class MyListener implements MouseListener {
            public void mouseClicked(MouseEvent e)
            {
                if (e.getButton() == MouseEvent.BUTTON1) {
                    centerX = e.getX();
                    centerY = e.getY();
                }
                else {
                    tracking = !tracking;
                    System.out.println(tracking);
                }
            }
            public void mouseEntered(MouseEvent e)
            {}
            public void mouseExited(MouseEvent e)
            {}
            public void mouseReleased(MouseEvent e)
            {}
            public void mousePressed(MouseEvent e)
            {}
        }
        addMouseListener(new MyListener());
    }

    public void stop()
    {
        myThread.stopRunning();
    }

    public void add
        (MrSonar newRobot)
    {
        robots.add(newRobot);
        trackers.add(new PosTrack(newRobot));
    }

    /*
    public void remove(MrSonar robot)
    {
      robots.remove(robot);
    }
    */

    public void paint(Graphics g)
    {
        super.paint(g);
        g.setColor(Color.WHITE);
        g.fillRect(0, 0, 500, 500);
        for (int z = 0; z < robots.size(); z++) {
            MrSonar myRobot = robots.get(z);
            PointWithAge[] data = myRobot.getCumulativeSonarData();
            int[] pos;
            for (int i = 0; i < data.length; i++) {
                PointWithAge thisReading = data[i];
                boolean isRobot = false;
                if (tracking) {
                    for (int a = 0; a < trackers.size(); a++) {
                        if (trackers.get(a).checkFor(thisReading)) {
                            isRobot = true;
                            //break;
                        }
                    }
                }
                pos = convertXY(thisReading.getX(), thisReading.getY());
                if (!isRobot) {
                    int newRed = 255;
                    if (thisReading.getAge() <= 100)
                        newRed = Math.round((255 / 100) * thisReading.getAge());
                    g.setColor(new Color(newRed, (255 / robots.size())*z, 0));
                    g.fillOval(pos[0] - 4, pos[1] - 4, 8, 8);
                }
                else {
                    g.setColor(Color.BLACK);
                    g.fillOval(pos[0] - 1, pos[1] - 1, 2, 2);
                }
            }
            g.setColor(new Color(0, 0, (255 / robots.size())*z));
            double[] robotPos = myRobot.getXYZT();
            pos = convertXY(robotPos[0] * 1000, robotPos[1] * 1000);
            g.fillOval(pos[0] - 5, pos[1] - 5, 10, 10);
        }
    }

    private int[] convertXY(double inX, double inY) //TAKES mm !!
    {
        int[] result = new int[2];
        result[0] = ((int)Math.round(inX / 50)) + centerX;
        result[1] = -((int)Math.round(inY / 50)) + centerY;
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
                //data = myRobot.getCumulativeSonarData();
                repaint();
                for (int i = 0; i < trackers.size(); i++)
                    trackers.get(i).update();
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

    private class PosTrack
    {
        public PosTrack(MrSonar robot)
        {
            myRobot = robot;
            points = new ArrayList();
        }

        public void update()
        {
            double[] pos = myRobot.getXYZT();
            points.add(new PointWithAge(pos[0], pos[1], pos[2]));
            if (points.size() > 30) {
                points.remove(0);
	    }
        }

        public boolean checkFor(PointWithAge p)
        {
            for (int i = 0; i < points.size(); i++) {
                PointWithAge t = points.get(i);
                if ( (Math.abs(t.getX() - p.getX()) < 800) &&
		     (Math.abs(t.getY() - p.getY()) < 800) ) {
                    return true;
                }
            }
            return false;
        }

        private MrSonar myRobot;
        private ArrayList<PointWithAge> points;
    }


    private ArrayList<MrSonar> robots;
    private ArrayList<PosTrack> trackers;
    private ViewUpdateThread myThread;
    private int centerX, centerY;
    private boolean tracking;
}

