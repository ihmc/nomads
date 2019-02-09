package us.ihmc.cot;

import org.apache.log4j.PropertyConfigurator;
import org.xml.sax.SAXException;
import us.ihmc.cot.parser.CotEvent;
import us.ihmc.cot.parser.CotUtilities;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Properties;

/**
 * CotParserTest.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class CotParserTest
{
    public static void main (String[] args)
    {
        PropertyConfigurator.configure(getLog4jProperties("../conf/cot.log4j.properties"));

        System.out.println("*** TESTING COT PARSER ");
        try {
            String xml = readFile(args[0], StandardCharsets.UTF_8);

            CotEvent event = null;
            try {
                event = CotEvent.parse(xml);
            }
            catch (SAXException e) {
                e.printStackTrace();
                return;
            }
            if (event == null) {
                System.out.println("Parsed event is NULL !!! - ERROR");
                return;
            }
            System.out.println("MIL-STD-2525 code for this message: " + CotUtilities.getSymbolFromCot(event.getType()));
            //System.out.println("Found CoT message: " + event.toString());
            System.out.println("UID: " + event.getUid());
            System.out.println("time: " + event.getTime());
            System.out.println("start: " + event.getStart());
            System.out.println("stale: " + event.getStale());
            System.out.println("type: " + event.getTime());
            System.out.println("how: " + event.getHow());
            System.out.println("Point Lat: " + event.getCotPoint().getLat());
            System.out.println("Point Lon: " + event.getCotPoint().getLon());
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static String readFile (String path, Charset encoding)
            throws IOException
    {
        byte[] encoded = Files.readAllBytes(Paths.get(path));
        return new String(encoded, encoding);
    }

    /**
     * Reads the log4j configuration file and converts that into a <code>Properties</code> instance appending the
     * date and time to the output log file
     *
     * @param configFilePath log4j configuration file name (including path)
     * @return the <code>Properties</code> instance representing the log4j configuration file
     */
    public static Properties getLog4jProperties (String configFilePath)
    {
        Properties log4jProperties = new Properties();
        try {
            log4jProperties.load(new FileInputStream(configFilePath));
            String logFileName = log4jProperties.getProperty("log4j.appender.rollingFile.File");
            Date day = new Date();
            String formattedDate = new SimpleDateFormat("yyyyMMddhhmm").format(day);
            log4jProperties.setProperty("log4j.appender.rollingFile.File",
                    String.format("../logs/%s-%s", formattedDate, logFileName));
        }
        catch (FileNotFoundException e) {
            System.out.println("Unable to load log4j configuration, file not found.");
            e.printStackTrace();
        }
        catch (IOException e) {
            System.out.println("Unable to load log4j configuration, error while I/O on disk.");
            e.printStackTrace();
        }
        return log4jProperties;
    }

}