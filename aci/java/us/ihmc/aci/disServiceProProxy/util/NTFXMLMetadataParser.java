package us.ihmc.aci.disServiceProProxy.util;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.StringReader;
import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Set;
import java.util.Stack;
import javax.xml.stream.XMLInputFactory;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.XMLStreamReader;
import us.ihmc.util.ByteArray;

/**
 * <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
 *  <RespositoryMetadataRoot xmlns="http://dmap.nrlssc.navy.mil/RepositorySchema" xmlns:ns2="urn:us:gov:ic:ism:v2">
 *   <Title>obs37_obj12653_tn.ntf</Title>
 *   <ID>03b25a4f-4307-4a9b-a89d-c9b3093b5472</ID>
 *   <Creator>default</Creator>
 *   <security ns2:releasableTo="USA" ns2:ownerProducer="USA" ns2:classification="U"/>
 *   <Description>obs37_obj12653_tn.ntf</Description>
 *   <GeographicExtent>
 *       <BoundingBox>
 *           <minx>-180.0</minx>
 *           <maxx>180.0</maxx>
 *           <miny>-90.0</miny>
 *           <maxy>90.0</maxy>
 *       </BoundingBox>
 *   </GeographicExtent>
 *   <CreationDate>2010-10-05T19:25:27.722Z</CreationDate>
 *   <SetType>NITF</SetType>
 *   <SearchRank>0</SearchRank>
 *  </RespositoryMetadataRoot>
 *
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class NTFXMLMetadataParser
{
    public static HashMap<String, Object> parse (byte[] xml) throws XMLStreamException
    {
        return parse (ByteArray.byteArrayToString(xml, 0, xml.length));
    }

    public static HashMap<String, Object> parse (String xml) throws XMLStreamException
    {
        return parse (XMLInputFactory.newInstance().createXMLStreamReader(new StringReader(xml)));
    }

    public static HashMap<String, Object> parse (File xmlFile) throws XMLStreamException, FileNotFoundException
    {
        return parse (XMLInputFactory.newInstance().createXMLStreamReader(new FileReader(xmlFile)));
    }

    public static HashMap<String, Object> parse (XMLStreamReader reader) throws XMLStreamException
    {
        while (reader.hasNext()) {
            int type = reader.next();
            if (reader.getEventType() == XMLStreamReader.START_ELEMENT && "RespositoryMetadataRoot".equalsIgnoreCase(reader.getLocalName())) {
                HashMap<String, Object> properties = new HashMap<String, Object>();
                String propertyName = null;
                String value = null;
                NTFMetadataElement property;
                Stack<String> elements = new Stack<String>();
                while (reader.hasNext()) {
                    type = reader.next();
                    try {
                        if (type == XMLStreamReader.START_ELEMENT) {
                            elements.add(reader.getLocalName());
                            // Parse the attributes, before going to the next element
                            for (int i = 0; i < reader.getAttributeCount(); i++) {
                                propertyName = reader.getAttributeLocalName(i);
                                property = NTFMetadataElement.valueOf(propertyName);
                                value = reader.getAttributeValue(i).trim();
                                Object obj = parseElement (property, value);
                                if (obj != null) {
                                    properties.put(propertyName, obj);
                                }
                                else {
                                    System.out.println("Did not add " + propertyName);
                                }
                            }
                            propertyName = reader.getLocalName();
                            value = null;
                        }
                        if (type == XMLStreamReader.END_ELEMENT) {
                            if (!elements.isEmpty()) {
                                propertyName = elements.pop();
                            }
                            else {
                                // End of the XML document
                                return properties;
                            }
                        }
                        else if ((type == XMLStreamReader.CHARACTERS) && (propertyName != null)) {
                            value = reader.getText().trim();
                        }

                        if (propertyName != null && value != null && value.length() > 0) {
                            property = NTFMetadataElement.valueOf(propertyName);
                            Object obj = parseElement (property, value);
                            if (obj != null) {
                                properties.put(propertyName, obj);
                            }
                            else {
                                    System.out.println("Did not add " + propertyName);
                                }
                            propertyName = value = null;
                        }
                    }
                    catch (IllegalArgumentException e) {
                        System.err.println("Property " + propertyName + " not in " + NTFMetadataElement.class.getName());
                    }
                    catch (ParseException e) {
                        System.err.println("Cannot parse " + value);
                    }
                    catch (Exception e) {
                        System.err.println(e.getMessage());
                    }
                }
                return properties;
            }
        }
        return null;
    }

    private static Object parseElement (NTFMetadataElement property, String value) throws XMLStreamException, ParseException, Exception
    {
        System.out.println("Parsing " + property + " " + value);
        switch (property) {
            case Title:
            case ID:
            case Creator:
            case Description:
            case SetType:
            case releasableTo:
            case ownerProducer:
            case classification:
                return value;
            case minx:
            case maxx: {
                // Longitude is in the ] -180.0, 180.0[ range
                Double coord = Double.parseDouble(value);
                if ((coord < 180.0) && (coord > -180.0)) {
                    return coord;
                }
                throw new Exception ("Longitude value out of range ] -180.0, 180.0[ : " + value);
            }
            case miny:
            case maxy: {
                // Latitude is in the ] -90.0, 90.0[ range
                Double coord = Double.parseDouble(value);
                if ((coord < 90.0) && (coord > -90.0)) {
                    return coord;
                }
                throw new Exception ("Latitude value out of range  ] -90.0, 90.0[: " + value);
            }
            case CreationDate:
                return (Date)_dateFormat.parse(value);
            case SearchRank:
                return Integer.parseInt(value);
            case GeographicExtent:
            case BoundingBox:
            case security:
                // Do nothing
                break;
        }
        return null;
    }

    public static void main(String[] args) throws XMLStreamException, FileNotFoundException, IOException
    {
        HashMap<String, Object> properties = null;
        if (args.length > 0) {
            File file = new File (args[0]);
            if (file.exists()) {
                if (file.isDirectory()) {
                    
                }
                BufferedReader br = new BufferedReader(new InputStreamReader(new DataInputStream(new FileInputStream(file))));
                String line;
                while ((line = br.readLine()) != null) {
                    System.out.println (line);
                }
                properties = parse (file);
            }
        }
        else {
            String xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
                       + "<RespositoryMetadataRoot xmlns=\"http://dmap.nrlssc.navy.mil/RepositorySchema\" xmlns:ns2=\"urn:us:gov:ic:ism:v2\">\n"
                       + "<Title>obs37_obj12653_tn.ntf</Title>\n"
                       + "<ID>03b25a4f-4307-4a9b-a89d-c9b3093b5472</ID>\n"
                       + "<Creator>default</Creator>\n"
                       + "<security ns2:releasableTo=\"USA\" ns2:ownerProducer=\"USA\" ns2:classification=\"U\"/>\n"
                       + "<Description>obs37_obj12653_tn.ntf</Description>\n"
                       + "<GeographicExtent>\n"
                       + "<BoundingBox>\n"
                       + "<minx>-150.0</minx>\n"
                       + "<maxx>130.0</maxx>\n"
                       + "<miny>-50.0</miny>\n"
                       + "<maxy>40.0</maxy>\n"
                       + "</BoundingBox>\n"
                       + "</GeographicExtent>\n"
                       + "<CreationDate>2010-10-05T19:25:27.722Z</CreationDate>\n"
                       + "<SetType>NITF</SetType>\n"
                       + "<SearchRank>0</SearchRank>\n"
                       + "</RespositoryMetadataRoot>\n";
            System.out.println(xml);
            properties = parse (xml);
        }
        System.out.println("===============================================\nPARSED DOC:\n");
        Set<String> keys = properties.keySet();
        for (String property : keys) {
            System.out.println(property + ": <" + (properties.get(property)) + ">");
        }
    }

    private static DateFormat _dateFormat = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'");
}
