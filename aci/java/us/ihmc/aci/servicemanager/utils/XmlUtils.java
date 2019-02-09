/*
 * XmlUtils.java
 *
 * Created on September 18, 2006, 2:57 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */
package us.ihmc.aci.servicemanager.utils;

import org.w3c.dom.*;
import java.io.*;
import org.apache.xml.serialize.XMLSerializer;
import org.apache.xml.serialize.OutputFormat;

/**
 *
 * @author sstabellini
 */
public class XmlUtils {
    
    /** Creates a new instance of XmlUtils */
    public XmlUtils() {
    }
    
    private static final String XML_ENCODING       = "UTF-8";
    private static final String XML_VERSION        = "1.0";
    
    public static byte[] generateByteArray(Document doc) throws IOException {
        
        XMLSerializer probeMsgSerializer   = null;
        OutputFormat  outFormat    = null;
        ByteArrayOutputStream baos=new java.io.ByteArrayOutputStream();
        
        probeMsgSerializer = new XMLSerializer();
        outFormat = new OutputFormat();
        
        // Setup format settings
        outFormat.setEncoding(XML_ENCODING);
        outFormat.setVersion(XML_VERSION);
        outFormat.setIndenting(true);
        outFormat.setIndent(4);
        
        // Define a Writer
        probeMsgSerializer.setOutputByteStream(baos);
        // Apply the format settings
        probeMsgSerializer.setOutputFormat(outFormat);
        
        // Serialize XML Document
        probeMsgSerializer.serialize(doc);
        return(baos.toByteArray());
    }
    
    public static StringBuffer generateStringBuffer(Document doc) throws IOException {
        
        XMLSerializer probeMsgSerializer   = null;
        OutputFormat  outFormat    = null;
        StringWriter sw=new StringWriter();
        
        probeMsgSerializer = new XMLSerializer();
        outFormat = new OutputFormat();
        
        // Setup format settings
        outFormat.setEncoding(XML_ENCODING);
        outFormat.setVersion(XML_VERSION);
        outFormat.setIndenting(true);
        outFormat.setIndent(4);
        
        // Define a Writer
        probeMsgSerializer.setOutputCharStream(sw);
        // Apply the format settings
        probeMsgSerializer.setOutputFormat(outFormat);
        
        // Serialize XML Document
        probeMsgSerializer.serialize(doc);
        return(sw.getBuffer());
    }
    
}
