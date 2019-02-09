package us.ihmc.aci.netviewer.views.graph;

import edu.uci.ics.jung.visualization.control.ModalGraphMouse;

/**
 * List of allowed <code>ModalGraphMode</code> modes
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
enum ModalGraphMouseSelection
{
    picking ("Move Single Node", ModalGraphMouse.Mode.PICKING),
    transforming ("Move Whole Graph", ModalGraphMouse.Mode.TRANSFORMING);

    /**
     * Constructor
     * @param actionCommand action command associated to the mode
     * @param mode <code>ModalGraphMode</code> mode
     */
    ModalGraphMouseSelection (String actionCommand, ModalGraphMouse.Mode mode)
    {
        _actionCommand = actionCommand;
        _mode = mode;
    }

    /**
     * Gets the action command
     * @return the action command
     */
    String getActionCommand()
    {
        return _actionCommand;
    }

    /**
     * Gets the mode
     * @return the mode
     */
    ModalGraphMouse.Mode getMode()
    {
        return _mode;
    }

    /**
     * Gets the mode associated to a given action command
     * @param actionCommand action command to look for
     * @return the mode associated to the action command
     */
    static ModalGraphMouse.Mode getMode (String actionCommand)
    {
        for (ModalGraphMouseSelection mgms : values()) {
            if (mgms.getActionCommand().equals (actionCommand)) {
                return mgms.getMode();
            }
        }

        return null;
    }

    private final String _actionCommand;
    private final ModalGraphMouse.Mode _mode;
}
