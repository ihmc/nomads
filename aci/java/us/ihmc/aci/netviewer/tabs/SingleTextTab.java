package us.ihmc.aci.netviewer.tabs;

import javax.swing.*;
import javax.swing.text.*;
import java.awt.*;
import java.util.Scanner;

/**
 * Wrapper class for single text tabs to include an unique id
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
class SingleTextTab extends JPanel implements SingleTab
{
    /**
     * Constructor
     * @param id id associated to the tab
     * @param text text to be displayed in the tab
     */
    SingleTextTab (String id, String text)
    {
        super();
        _id = id;
        _txtPane = new JTextPane();
        _txtPane.addStyle ("bold", null);

        JScrollPane scrollPane = new JScrollPane (_txtPane);
        scrollPane.setVerticalScrollBarPolicy (JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
        scrollPane.setHorizontalScrollBarPolicy (JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
        updateText (text);

        setLayout (new BoxLayout (this, BoxLayout.PAGE_AXIS));
        add (scrollPane);
    }

    @Override
    public String getId()
    {
        return _id;
    }

    /**
     * Updates the tab text
     * @param text text to be displayed in the tab
     */
    void updateText (String text)
    {
        _txtPane.setText ("");

        if (text == null) {
            return;
        }

        Scanner scanner = new Scanner (text);
        while (scanner.hasNextLine()) {
            String line = scanner.nextLine();
            if (line.contains ("<bold>")) {
                String boldText = line.substring (0, line.indexOf ("</bold>"));
                boldText = boldText.replace ("<bold>", "");
                String plainText = line.substring (line.indexOf("</bold>"));
                plainText = plainText.replace ("</bold>", "");

                addBoldText (_txtPane, boldText, Color.BLACK);
                addPlainText (_txtPane, plainText + "\n", Color.BLACK);
            }
            else if (line.isEmpty()) {
                addPlainText (_txtPane, "\n", Color.BLACK);
            }
            else {
                addPlainText (_txtPane, line + "\n", Color.BLACK);
            }
        }
    }

    private void addBoldText (JTextPane tp, String text, Color c)
    {
        MutableAttributeSet aSet = new SimpleAttributeSet();
        aSet.addAttribute (StyleConstants.Foreground, c);
        StyleConstants.setBold (aSet, true);
        int len = tp.getDocument().getLength();
        tp.setCaretPosition (len);
        tp.setCharacterAttributes (aSet, false);
        tp.replaceSelection (text);
    }

    private void addPlainText (JTextPane tp, String text, Color c)
    {
        MutableAttributeSet aSet = new SimpleAttributeSet();
        aSet.addAttribute (StyleConstants.Foreground, c);
        StyleConstants.setBold (aSet, false);
        int len = tp.getDocument().getLength();
        tp.setCaretPosition (len);
        tp.setCharacterAttributes (aSet, false);
        tp.replaceSelection (text);
    }

    private final String _id;
    private final JTextPane _txtPane;
}
