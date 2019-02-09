package us.ihmc.cot.parser;

import org.xml.sax.ContentHandler;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import java.io.IOException;
import java.io.StringReader;

/**
 * Xml.java
 */
public class Xml
{
    /**
     * @hide
     */
    public Xml ()
    {
    }

    /**
     * {@link org.xmlpull.v1.XmlPullParser} "relaxed" feature name.
     *
     * @see <a href="http://xmlpull.org/v1/doc/features.html#relaxed">
     *  specification</a>
     */

    /**
     * Parses the given xml string and fires events on the given SAX handler.
     */
    public static void parse (String xml, ContentHandler contentHandler)
            throws SAXException
    {
        try {
            SAXParserFactory factory = SAXParserFactory.newInstance();
            SAXParser parser = factory.newSAXParser();
            XMLReader reader = parser.getXMLReader();
            reader.setContentHandler(contentHandler);
            reader.parse(new InputSource(new StringReader(xml)));
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (ParserConfigurationException e) {
            e.printStackTrace();
        }
    }

//    public static void parse (byte[] bytes)
//            throws SAXException
//    {
//        try {
//            SAXParserFactory factory = SAXParserFactory.newInstance();
//            SAXParser parser = factory.newSAXParser();
//            parser.parse(new ByteArrayInputStream(bytes), new MessageParser());
//        }
//        catch (IOException e) {
//            e.printStackTrace();
//        }
//        catch (ParserConfigurationException e) {
//            e.printStackTrace();
//        }
//    }
}
