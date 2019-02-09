package us.ihmc.cot.parser;


import java.io.IOException;
import java.util.*;
import java.util.regex.Pattern;

/**
 * A Cursor on Target root detail tag or sub tag
 */
public class CotDetail {

    private final static Pattern pattern = Pattern.compile("/"); // thread safe

    private Map<String, String> _attrs = new HashMap<String, String>();
    private List<CotDetail> _children = new ArrayList<CotDetail>();
    private Map<String, CotDetail> firstNode = new HashMap<String, CotDetail>();

    private String _elemName;
    private String _innerText;

    /**
     * Create a default detail tag
     */
    public CotDetail() {
        this("detail");
    }

    /**
     * Copy Constructor.
     */
    public CotDetail(final CotDetail d) {
        _elemName = d.getElementName();
        _innerText = d.getInnerText();
        d.copyAttributesInternal(_attrs);
        d.copyChildrenInternal(_children);
    }

    /**
     * Get the tag element name
     * 
     * @return
     */
    public String getElementName() {
        return _elemName;
    }

    /**
     * Get an attribute by name
     * 
     * @param name attribute name
     * @return
     */
    public String getAttribute(final String name) {
        return _attrs.get(name);
    }

    /**
     * Internal copy _attrs -- only used in the copy contructor
     */
    void copyAttributesInternal(Map<String, String> destination) {
        destination.putAll(_attrs);
    }

    /**
     * Internal copy _children -- only used in the copy contructor
     */
    void copyChildrenInternal(List<CotDetail> destination) {
        synchronized (_children) {
            destination.addAll(_children);
        }
    }

    /**
     * Get the inner text of the tag if any. This does not return XML representation of sub tags.
     * Inner-text and sub-tags are mutually exclusive.
     * 
     * @return
     */
    public String getInnerText() {
        return _innerText;
    }

    /**
     * Create a detail tag with a element name
     * 
     * @param elementName the element name
     */
    public CotDetail(String elementName) {
        if (!_validateName(elementName)) {
            throw new IllegalArgumentException("invalid element name");
        }
        _elemName = elementName;
    }

    /**
     * Get an array of the immutable attributes of the detail tag. The order is the same as the add
     * order.
     * 
     * @return
     */
    public CotAttribute[] getAttributes() {
        CotAttribute[] attrs = new CotAttribute[_attrs.size()];
        Set<Map.Entry<String, String>> entries = _attrs.entrySet();
        Iterator<Map.Entry<String, String>> it = entries.iterator();
        int index = 0;
        while (it.hasNext()) {
            Map.Entry<String, String> entry = it.next();
            attrs[index++] = new CotAttribute(entry.getKey(), entry.getValue());
        }
        return attrs;
    }

    @Override
    public String toString() {
        return "<" + _elemName + " .../>";
    }

    /**
     * The number of sub tags in this detail tag
     * 
     * @return
     */
    public int childCount() {
        return _children.size();
    }

    /**
     * Get a child detail tag at a given index or null if the index is invalid at the time of the
     * call.
     * 
     * @param index
     * @return
     */
    public CotDetail getChild(final int index) {
        try {
            return _children.get(index);
        } catch (IndexOutOfBoundsException oobe) {
            return null;
        }
    }

    /**
     * Set an attribute. There is no need to do any special escaping of the value. When an XML
     * string is built, escaping of illegal characters is done automatically.
     * 
     * @throws IllegalArgumentException if the attribute name is not a legal XML name.
     * @param name the attribute name
     * @param value the value
     */
    public void setAttribute(String name, String value) {
        if (value != null)
            _attrs.put(name, value);
    }

    /**
     * Remove an attribute.
     * 
     * @param name the attribute name (can be anything)
     * @return get the value removed if any
     */
    public String removeAttribute(String name) {
        return _attrs.remove(name);
    }

    /**
     * Remove all attributes
     */
    public void clearAttributes() {
        _attrs.clear();
    }

    /**
     * Set the element name of the tag
     * 
     * @throws IllegalArgumentException if the attribute name is not a legal XML name
     * @param name
     */
    public void setElementName(String name) {
        if (!_validateName(name)) {
            throw new IllegalArgumentException("attribute name is invalid ('" + name + "')");
        }
        _elemName = name;
    }

    /**
     * Add a sub detail tag. Since sub tags and inner text are mutually exclusive, a successful
     * calling will clear any inner text.
     * 
     * @throws IllegalArgumentException if detail is null
     * @param detail
     */
    public void addChild(CotDetail detail) {

        synchronized (_children) {
            if (detail == null) {
                throw new IllegalArgumentException("detail is null");
            }
            _innerText = null;
            _children.add(detail);
        }
    }

    /**
     * Set a sub detail tag
     * 
     * @throws IllegalArgumentException if detail is null
     * @throws ArrayIndexOutOfBoundsException if index is out of bounds
     * @param index
     * @param detail
     */
    public void setChild(final int index, final CotDetail detail) {
        synchronized (_children) {
            firstNode.clear(); // children are changing, clear the cache.
            if (detail == null) {
                throw new IllegalArgumentException("detail cannot be null");
            }
            _children.set(index, detail);
        }
    }

    /**
     * Remove a child.
     */
    void removeChild(CotDetail detail) {
        synchronized (_children) {
            firstNode.clear(); // children are changing, clear the cache.
            _children.remove(detail);
        }
    }

    /**
     * Set the inner text of this tag. Since sub tags and inner text are mutually exclusive, calling
     * this removes any sub tags
     * 
     * @param text
     */
    public void setInnerText(String text) {
        synchronized (_children) {
            firstNode.clear(); // children are changing, clear the cache.
            _children.clear();
            _innerText = text;
        }
    }

    /**
     * Get the first sub tag of a certain name starting at some index or null if the childElement is
     * not found or the startIndex is out of bounds.
     * 
     * @throws
     * @param startIndex the start index
     * @param childElementName the element name
     * @return
     */
    public CotDetail getFirstChildByName(final int startIndex, final String childElementName) {
        CotDetail cd = null;

        if (startIndex == 0) {
            // check the cache for the first node
            cd = firstNode.get(childElementName);
            if (cd != null) {
                return cd;
            }
        }

        synchronized (_children) {
            for (int i = startIndex; i < _children.size(); ++i) {
                cd = getChild(i); // includes protection for an index out of bound exception
                if (cd != null && cd.getElementName().equals(childElementName)) {
                    if (startIndex == 0) {
                        // populate the cache for the first found node.
                        firstNode.put(childElementName, cd);
                    }
                    return cd;
                }
            }
        }
        return null;
    }

    public void buildXml(StringBuffer b) {
        try {
            this.buildXmlImpl(b);
        } catch (IOException e) {
            throw new IllegalStateException(e);
        }
    }

    public void buildXml(StringBuilder b) {
        try {
            this.buildXmlImpl(b);
        } catch (IOException e) {
            throw new IllegalStateException(e);
        }
    }

    /**
     * Build the XML representation of the detail tag
     * 
     * @param b
     */
    public void buildXml(Appendable b) throws IOException {
        this.buildXmlImpl(b);
    }

    private void buildXmlImpl(Appendable b) throws IOException {
        b.append("<");
        b.append(_elemName);
        Set<Map.Entry<String, String>> entries = _attrs.entrySet();
        for (Map.Entry<String, String> entry : entries) {
            b.append(" ");
            b.append(entry.getKey());
            b.append("='");
            String valueText = entry.getValue();
            if (valueText != null) {
                b.append(CotEvent.escapeXmlText(entry.getValue()));
            }
            b.append("'");
        }
        if (_innerText != null) {
            b.append(">");
            b.append(CotEvent.escapeXmlText(_innerText));
            b.append("</");
            b.append(_elemName);
            b.append(">");
        }
        else if (_children.size() > 0) {
            b.append(">");
            synchronized (_children) {
                for (int i = 0; i < _children.size(); ++i) {
                    CotDetail child = getChild(i);
                    if (child != null)
                        child.buildXml(b);
                }
            }
            b.append("</");
            b.append(_elemName);
            b.append(">");
        }
        else {
            b.append("/>");
        }
    }

    /**
     * Get the number of attributes in the tag
     * 
     * @return
     */
    public int getAttributeCount() {
        return _attrs.size();
    }

    private static boolean _validateName(String name) {
        return true;
    }
}
