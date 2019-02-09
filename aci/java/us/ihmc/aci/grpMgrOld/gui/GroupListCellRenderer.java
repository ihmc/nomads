package us.ihmc.aci.grpMgrOld.gui;

import java.awt.Component;

import javax.swing.DefaultListCellRenderer;
import javax.swing.ImageIcon;
import javax.swing.JList;

/**
 * Cell Renderer for the GroupManager Local and Remote Lists.
 * 
 * @author Niranjan Suri
 * modify by:
 * @author Maggie Breedy <NOMADS team>
 * 
 * @version $Revision$
 */

public class GroupListCellRenderer extends DefaultListCellRenderer
{
    public Component getListCellRendererComponent (JList list, Object value, 
                                                   int index, boolean isSelected, 
                                                   boolean cellHasFocus)
    {
        GroupObject valueObj = (GroupObject) value;
        String groupName = valueObj._groupName;
        String statusStr = valueObj._status;
        String _isMember = valueObj._member;
        String parName = valueObj._parsedName;
        setText (parName);
        
        if (statusStr.equals ("PRIVATE")) {
            if (_isMember.equals ("MEMBER")) {
                setIcon (rm);
            }
            else {
                setIcon (r);
            }
        }
        else if (statusStr.equals ("PUBLIC")) {
            if (_isMember.equals ("MEMBER")) {
                setIcon (m);
            }
            else {
                setIcon (blank);
            }
        }
        else if (statusStr.equals ("PUBLIC_PEER")) {
            if (_isMember.equals ("MEMBER")) {
                setIcon (pm);
            }
            else {
                setIcon (p);
            }
        }
        else if (statusStr.equals ("PRIVATE_PUBLIC_PEER")) {
            if (_isMember.equals ("MEMBER")) {
                setIcon (prm);
            }
            else {
                setIcon (pr);
            }
        }
        else if (statusStr.equals ("REMOTE_PEER")) {
            if (_isMember.equals ("MEMBER")) {
                setIcon (pm);
            }
            else {
                setIcon (p);
            }
        }
        else if (statusStr.equals ("REMOTE_PRIVATE_PEER")) {
            if (_isMember.equals ("MEMBER")) {
                setIcon (prm);
            }
            else {
                setIcon (pr);
            }
        }
        else {
            System.out.println ("-->No remote or local group instance");
            setIcon (blank);
        }

        if (isSelected) {
            setBackground (list.getSelectionBackground());
            setForeground (list.getSelectionForeground());
        }
        else {
            setBackground (list.getBackground());
            setForeground (list.getForeground());
        }

        setEnabled (list.isEnabled());
        setFont (list.getFont());
        setOpaque (true);
        
        return this;
    }

    private ImageIcon blank = new ImageIcon (getClass().getResource ("blank.gif"));
    private ImageIcon p = new ImageIcon (getClass().getResource ("P.gif"));
    private ImageIcon rm = new ImageIcon (getClass().getResource ("RM.gif"));
    private ImageIcon pm = new ImageIcon (getClass().getResource ("PM.gif"));
    private ImageIcon r = new ImageIcon (getClass().getResource ("R.gif"));
    private ImageIcon pr = new ImageIcon (getClass().getResource ("PR.gif"));
    private ImageIcon prm = new ImageIcon (getClass().getResource ("PRM.gif"));
    private ImageIcon m = new ImageIcon (getClass().getResource ("M.gif"));
}
