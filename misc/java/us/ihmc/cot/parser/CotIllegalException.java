package us.ihmc.cot.parser;

/**
 * Thrown when a CotEvent is missing required attributes or is malformed when parsing or building to
 * XML.
 * 
 * @author Joe Bergeron
 */
public class CotIllegalException extends Exception {

    public CotIllegalException() {
    }

    public CotIllegalException(String msg) {
        super(msg);
    }

    /**
	 * 
	 */
    private static final long serialVersionUID = 1L;

}
