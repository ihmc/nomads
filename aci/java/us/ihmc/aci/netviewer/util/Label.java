package us.ihmc.aci.netviewer.util;

/**
 * List of labels used by the netviewer
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public enum Label
{
    left_red_arrow_fives ("<font color=\"red\" size=\"5\">\u2190</font><font color=\"red\" size=\"2\">5s</font>"),
    right_green_arrow_fives ("<font color=\"green\" size=\"5\">\u2192</font><font color=\"green\" size=\"2\">5s</font>"),
    left_blue_arrow_fives ("<font color=\"dodger blue\" size=\"5\">\u2190</font><font color=\"dodger blue\" size=\"2\">5s</font>"),
    right_orange_arrow_fives ("<font color=\"orange\" size=\"5\">\u2192</font><font color=\"orange\" size=\"2\">5s</font>"),
    left_red_arrow_min ("<font color=\"red\" size=\"5\">\u2190</font><font color=\"red\" size=\"2\">min</font>"),
    right_green_arrow_min ("<font color=\"green\" size=\"5\">\u2192</font><font color=\"green\" size=\"2\">min</font>"),
    left_blue_arrow_min ("<font color=\"dodger blue\" size=\"5\">\u2190</font><font color=\"dodger blue\" size=\"2\">min</font>"),
    right_orange_arrow_min ("<font color=\"orange\" size=\"5\">\u2192</font><font color=\"orange\" size=\"2\">min</font>"),
    play ("<html><font size=\"6\">\u25B6</font></html>"),
    pause ("<html><font size=\"4\">\u2759\u2759</font></html>"),
    stop ("<html><font size=\"4\">\u25A0</font></html>");

    Label (String l)
    {
        _label = l;
    }

    public String get()
    {
        return _label;
    }

    private final String _label;
}
