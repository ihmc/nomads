
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;
import java.util.*;

public class RobotTest4
{
    public static void main(String[] args)
    {
        MrRobot.init();

        TestViewPanel4 viewPanel = new TestViewPanel4();
        JFrame viewFrame = new JFrame();
        viewFrame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        viewFrame.getContentPane().add(viewPanel);
        viewFrame.pack();
        viewFrame.show();

        for (int i = 0; i < 4; i++) {
            RobotTest4Frame thisFrame = new RobotTest4Frame(viewPanel);
            thisFrame.pack();
            thisFrame.show();
        }
    }
}


