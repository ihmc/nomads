
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;
import java.util.*;
import java.text.*;

public class RobotTest3Frame extends JFrame
{
    public RobotTest3Frame()
    {
        this.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        MrRobot.init();
        myRobot = new MrSonar();
        setupWindowListener();
        setupButtons();
        setPreferredSize(new Dimension(260, 500));
    }

    private void setupWindowListener()
    {
        class CleanupWindowListener implements WindowListener {
            public void windowActivated(WindowEvent e)
            {}
            public void windowClosed(WindowEvent e)
            {
                myRobot.disconnect();
                MrRobot.cleanup();
                System.exit(0);
            }
            public void windowClosing(WindowEvent e)
            {}
            public void windowDeactivated(WindowEvent e)
            {}
            public void windowDeiconified(WindowEvent e)
            {}
            public void windowIconified(WindowEvent e)
            {}
            public void windowOpened(WindowEvent e)
            {}
        }
        this.addWindowListener(new CleanupWindowListener());
    }

    private void setupButtons()
    {
        JPanel mainPanel = new JPanel();
        getContentPane().add(mainPanel);

        xField = new JTextField("0", 4);
        mainPanel.add(xField);
        yField = new JTextField("0", 4);
        mainPanel.add(yField);
        zField = new JTextField("0", 4);
        mainPanel.add(zField);
        tField = new JTextField("0", 3);
        mainPanel.add(tField);

        JButton connectButton = new JButton("Connect");
        class ConnectListener implements ActionListener {
            public void actionPerformed(ActionEvent e)
            {
                connect();
            }
        }
        connectButton.addActionListener(new ConnectListener());
        mainPanel.add(connectButton);

        /*
        JButton eStopButton = new JButton("Bye");
        class EStopListener implements ActionListener
        {
          public void actionPerformed(ActionEvent e)
          {
        emergencyStop();
          }
        }
        eStopButton.addActionListener(new EStopListener());
        mainPanel.add(eStopButton);
        */

        JButton stopButton = new JButton("Stop!");
        class StopListener implements ActionListener {
            public void actionPerformed(ActionEvent e)
            {
                stop();
            }
        }
        stopButton.addActionListener(new StopListener());
        mainPanel.add(stopButton);

        JButton goButton = new JButton("Go!");
        class GoListener implements ActionListener {
            public void actionPerformed(ActionEvent e)
            {
                go();
            }
        }
        goButton.addActionListener(new GoListener());
        mainPanel.add(goButton);

        JButton backButton = new JButton("Back");
        class BackListener implements ActionListener {
            public void actionPerformed(ActionEvent e)
            {
                back();
            }
        }
        backButton.addActionListener(new BackListener());
        mainPanel.add(backButton);

        /*
        JButton sv2Button = new JButton("Set motor vel");
        class Sv2Listener implements ActionListener
        {
          public void actionPerformed(ActionEvent e)
          {
        setVel2();
          }
        }
        sv2Button.addActionListener(new Sv2Listener());
        mainPanel.add(sv2Button);

        JButton goHomeButton = new JButton("Go home");
        class GoHomeListener implements ActionListener
        {
          public void actionPerformed(ActionEvent e)
          {
        goHome();
          }
        }
        goHomeButton.addActionListener(new GoHomeListener());
        mainPanel.add(goHomeButton);

        JButton turnTowardsButton = new JButton("Turn Towards...");
        class TurnTowardsListener implements ActionListener
        {
          public void actionPerformed(ActionEvent e)
          {
        turnTowards();
          }
        }
        turnTowardsButton.addActionListener(new TurnTowardsListener());
        mainPanel.add(turnTowardsButton);

        JButton turnToButton = new JButton("Turn To...");
        class TurnToListener implements ActionListener
        {
          public void actionPerformed(ActionEvent e)
          {
        turnTo();
          }
        }
        turnToButton.addActionListener(new TurnToListener());
        mainPanel.add(turnToButton);

        JButton turnRButton = new JButton("Turn relative...");
        class TurnRListener implements ActionListener
        {
          public void actionPerformed(ActionEvent e)
          {
        turnRelative();
          }
        }
        turnRButton.addActionListener(new TurnRListener());
        mainPanel.add(turnRButton);
        */

        JButton moveToButton = new JButton("Move To...");
        class MoveToListener implements ActionListener {
            public void actionPerformed(ActionEvent e)
            {
                moveTo();
            }
        }
        moveToButton.addActionListener(new MoveToListener());
        mainPanel.add(moveToButton);

        JButton setXYZTButton = new JButton("SetXYZT");
        class SetXYZTListener implements ActionListener {
            public void actionPerformed(ActionEvent e)
            {
                setXYZT();
            }
        }
        setXYZTButton.addActionListener(new SetXYZTListener());
        mainPanel.add(setXYZTButton);

        JButton getXYZTButton = new JButton("GetXYZT");
        class GetXYZTListener implements ActionListener {
            public void actionPerformed(ActionEvent e)
            {
                getXYZT();
            }
        }
        getXYZTButton.addActionListener(new GetXYZTListener());
        mainPanel.add(getXYZTButton);

        JButton setTButton = new JButton("SetT");
        class SetTListener implements ActionListener {
            public void actionPerformed(ActionEvent e)
            {
                setT();
            }
        }
        setTButton.addActionListener(new SetTListener());
        mainPanel.add(setTButton);

        JCheckBox intsBox = new JCheckBox("Allow Interruptions");
        intsBox.setSelected(false);
        class intsListener implements ItemListener {
            public void itemStateChanged(ItemEvent e)
            {
                myRobot.allowInterruptions(e.getStateChange() == 
		                           ItemEvent.SELECTED);
            }
        }
        intsBox.addItemListener(new intsListener());
        mainPanel.add(intsBox);
    }

    /*
     * stuff
     */

    private void connect()
    {
        if (!(myRobot.connect())) {
            System.out.println("Couldn't connect, exiting.");
            System.exit(1);
        }
        class MyLostListener implements ConnectionLostListener {
            public void connectionLost()
            {
                System.out.println("OH NO!");
                MrRobot.cleanup();
                System.exit(0);
            }
        }
        myRobot.addConnectionLostListener(new MyLostListener());
        myRobot.addCommandListener(new MyListener());
        System.out.println("Current voltage is: " +
	                   myRobot.getCurrentVoltage());
    }

   private void stop()
    {
        myRobot.stopCurrentCommand();
        try {
            myRobot.stop();
        }
        catch (CmdInterruptedException e) {}
    }

    private void go()
    {
        try {
            double v = myRobot.getVelocity();
            myRobot.setVelocity(v + 100);
            System.out.println(v + 100);
        }
        catch (CmdInterruptedException e) {
            System.out.println(e);
        }
    }

    private void back()
    {
        try {
            double v = myRobot.getVelocity();
            myRobot.setVelocity(v - 100);
            System.out.println(v - 100);
        }
        catch (CmdInterruptedException e) {}
    }

    private void moveTo()
    {
        try {
            myRobot.moveToXYZ(getXInput(), getYInput(), getZInput(), "Move to");
        }
        catch (CmdInterruptedException e) {}
    }

    private void setXYZT()
    {
        myRobot.setXYZT(getXInput(), getYInput(), getZInput(), getTInput());
    }

    private void getXYZT()
    {
        double[] pos = myRobot.getXYZT();
        DecimalFormat fmt = new DecimalFormat("###.###");
        xField.setText(fmt.format(pos[0]));
        yField.setText(fmt.format(pos[1]));
        zField.setText(fmt.format(pos[2]));
        tField.setText(Long.toString(Math.round(pos[3])));
    }

    private void setT()
    {
        double[] pos = myRobot.getXYZT();
        myRobot.setXYZT(pos[0], pos[1], pos[2], getTInput());
    }

    /*
     * Other stuff
     */

    private class MyListener implements CommandListener
    {
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

    private double getXInput()
    {
        double x = 0;
        try {
            x = Double.valueOf(xField.getText());
        }
        catch (NumberFormatException e) {
            System.out.println("Couldn't read X position, using 0.");
            x = 0;
        }
        return x;
    }

    private double getYInput()
    {
        double y = 0;
        try {
            y = Double.valueOf(yField.getText());
        }
        catch (NumberFormatException e) {
            System.out.println("Couldn't read Y position, using 0.");
            y = 0;
        }
        return y;
    }

    private double getTInput()
    {
        double t = 0;
        try {
            t = Double.valueOf(tField.getText());
        }
        catch (NumberFormatException e) {
            System.out.println("Couldn't read T position, using 0.");
            t = 0;
        }
        return t;
    }

    private double getZInput()
    {
        double z = 0;
        try {
            z = Double.valueOf(zField.getText());
        }
        catch (NumberFormatException e) {
            System.out.println("Couldn't read Z position, using 0.");
            z = 0;
        }
        return z;
    }

    private Robot myRobot;
    private JTextField xField, yField, zField, tField;
}


