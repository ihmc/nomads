package us.ihmc.gst.util;

import java.util.List;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class ElementComparator
{
    /**
     * 
     * @param stack the stack of the XML elements from the root to the current element
     * @param element the XML element that has to be matched
     * @param elements the ancestor elements that the element to be match must have
     *                 in order to be matched (from the closest to the farthest)
     * @return 
     */
    public static boolean matches (List<String> ancestorStack, String element, String ... elementsStack)
    {
        if (ancestorStack.isEmpty()) {
            return false;
        }
        if (!ancestorStack.get(0).equalsIgnoreCase (element)) {
            return false;
        }

        String ancestor;    boolean found;    int i = 0;
        for (String s : elementsStack) {
            found = false;
            for ( ; i < ancestorStack.size(); i++) {
                ancestor = ancestorStack.get(i);
                if (ancestor.matches(s)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
        return true;
    }
}
