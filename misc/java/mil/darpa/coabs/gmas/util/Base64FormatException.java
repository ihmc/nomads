package mil.darpa.coabs.gmas.util;

/**
 * Base64FormatException.java
 * $Id$
 * (c) COPYRIGHT MIT and INRIA, 1996.
 * Please first read the full copyright statement in file COPYRIGHT.html
 * Exception for invalid BASE64 streams.
 */

public class Base64FormatException extends Exception {

/**
   * Create that kind of exception
   * @param msg The associated error message 
   */
  
  public Base64FormatException(String msg) {
	super(msg) ;
  }

  private static final long serialVersionUID = 1L;
}
