package us.ihmc.aci.nodemon.util;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.util.JsonFormat;
import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.util.Config;

import java.io.*;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.ByteOrder;
import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
//import java.time.Instant;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Utils.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Utils
{
    public static String getAutoConfigFile (String configFile)
    {
        return ((new File(configFile).getParentFile().getAbsolutePath() + File.separator + Strings.Files
                .GROUP_MANAGER_AUTO_CONFIG));
    }

    public static void initLogger (String log4jConf, String logFolder)
    {
//        Properties log4jProperties = Utils.getLog4jProperties(Utils.getConfDir() + Strings.Files.LOG4J_CONFIG,
//                Utils.getLogDir());
        Properties log4jProperties = Utils.getLog4jProperties(log4jConf, logFolder);
        PropertyConfigurator.configure(log4jProperties);

        log.info("Loaded Log4J config: " + log4jConf);
    }

    public static void printWorldState (NodeMon nodeMon)
    {
        log.debug("*** WorldState Dump ***");

        for (Node node : nodeMon.getWorldState().getNodes()) {
            try {
                log.debug("---------------------------------------------");
                log.debug("Id: " + node.getId() + " Name: " + node.getName());

                //convert traffic to a human readable version
                ReadableTraffic rt = ProtoUtils.toReadable(node.getTraffic());
                log.debug(node.getId() + " Traffic: \n" + JsonFormat.printer().print(rt));
                for (NetworkHealth nh : node.getNetworkHealth().values()) {
                    log.debug(node.getId() + " NetworkHealth: \n" + JsonFormat.printer().print(nh));
                }

                //convert topology to a human readable version
                for (Topology topo : node.getTopology().values()) {
                    ReadableTopology readTopo = ProtoUtils.toReadable(topo);
                    log.debug(node.getId() + " Topology: \n" + JsonFormat.printer().print(readTopo));
                }

                log.debug(node.getId() + " Info: \n" + JsonFormat.printer().print(node.getInfo()));
                log.debug(node.getId() + " Grump: \n" + JsonFormat.printer().print(node.getGrump()));

            }
            catch (InvalidProtocolBufferException e) {
                e.printStackTrace();
            }
        }
        log.debug("***");
    }

    public static void writeWorldStateToFile (NodeMon nodeMon, String dirPath)
    {
        StringBuilder sb = new StringBuilder();
        for (Node node : nodeMon.getWorldState().getNodes()) {

            try {
                sb.append("---------------------------------------------");
                sb.append("Id: ").append(node.getId()).append(" Name: ").append(node.getName());
                //convert traffic to a human readable version
                ReadableTraffic rt = ProtoUtils.toReadable(node.getTraffic());
                sb.append(node.getId()).append(" Traffic: \n").append(JsonFormat.printer().print(rt));

                for (NetworkHealth nh : node.getNetworkHealth().values()) {
                    log.debug(node.getId() + " NetworkHealth: \n" + JsonFormat.printer().print(nh));
                }

                //convert topology to a human readable version
                for (Topology topo : node.getTopology().values()) {
                    ReadableTopology readTopo = ProtoUtils.toReadable(topo);
                    sb.append(node.getId()).append(" Topology: \n").append(JsonFormat.printer().print(readTopo));
                }

                sb.append(node.getId()).append(" Info: \n").append(JsonFormat.printer().print(node.getInfo()));
                sb.append(node.getId()).append(" Grump: \n").append(JsonFormat.printer().print(node.getGrump()));
            }
            catch (InvalidProtocolBufferException e) {
                e.printStackTrace();
            }
        }
        String filePath = dirPath + "/" + "worldstate.log";
        writeToFile(dirPath, filePath, sb.toString().getBytes());
    }

    public static void writeWorldStateToJSON (NodeMon nodeMon, String dirPath)
    {
        for (Node node : nodeMon.getWorldState().getNodes()) {
            try {
                String filePath = dirPath + "/" + node.getId() + ".json";
                writeToFile(dirPath, filePath, JsonFormat.printer().print(ProtoUtils.toReadable(node)).getBytes());
            }
            catch (InvalidProtocolBufferException e) {
                e.printStackTrace();
            }
        }
    }

    public static boolean containsIP (String text)
    {
        Pattern p = Pattern.compile("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}" +
                "(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
        Matcher m = p.matcher(text);
        return m.find();
    }


//    /**
//     * Method for converting an existing IP in Integer format to String format.
//     *
//     * @param integerIp The IP Address in Integer format
//     * @return A String representation of the IP Address
//     */
//    public static String intToIp (int integerIp)
//    {
//        //    return ((integerIp >> 24) & 0xFF) + "." + ((integerIp >> 16) & 0xFF)
//        //            + "." + ((integerIp >> 8) & 0xFF) + "." + (integerIp & 0xFF);
//
//        return (integerIp & 0xFF) + "." + ((integerIp >> 8) & 0xFF)
//                + "." + ((integerIp >> 16) & 0xFF) + "." + ((integerIp >> 24) & 0xFF);
//    }


    /**
     * Converts an Integer IP into its String representation.
     *
     * @param ip the Integer representation of this IP
     * @return
     */
    public static String convertIPToString (int ip)
    {

        ip = (ByteOrder.nativeOrder().equals(ByteOrder.LITTLE_ENDIAN)) ?
                Integer.reverseBytes(ip) : ip;
        try {
            return InetAddress.getByAddress(unpack(ip)).getHostAddress();
        }
        catch (UnknownHostException e) {
            log.error("Unable to convert " + ip + " to dotted string representation", e);
            return null;
        }
    }

    /**
     * Converts a String IP into its int representation.
     * E.g. usage: pack(InetAddress.getByName(dottedString).getAddress());
     *
     * @param ip the String representation of this IP
     * @return
     */
    public static Integer convertIPToInteger (String ip)
    {
        Objects.requireNonNull(ip, "the ip can't be null");

        try {
            return pack(InetAddress.getByName(ip).getAddress());
        }
        catch (UnknownHostException e) {
            e.printStackTrace();
            log.error("Unable to convert " + ip + " to Integer representation", e);
            return null;
        }
    }


    /**
     * Converts this byte IP into its int representation.
     * E.g. usage: pack(InetAddress.getByName(dottedString).getAddress());
     *
     * @param bytes the byte representation of this IP
     * @return
     */
    static int pack (byte[] bytes)
    {
        int val = 0;
        for (int i = 0; i < bytes.length; i++) {
            val <<= 8;
            val |= bytes[i] & 0xff;
        }
        return val;
    }

    /**
     * Converts this int IP to it's string representation.
     * E.g. usage: InetAddress.getByAddress(unpack(packedBytes)).getHostAddress()
     *
     * @param bytes the int representation of this IP
     * @return
     */
    static byte[] unpack (int bytes)
    {
        return new byte[]{
                (byte) ((bytes) & 0xff),
                (byte) ((bytes >>> 8) & 0xff),
                (byte) ((bytes >>> 16) & 0xff),
                (byte) ((bytes >>> 24) & 0xff)

        };
    }


    /**
     * Method for the validation of the input IP v4 Address.
     *
     * @param ipAddress The IP Address input string
     * @return True if the IP is valid, false otherwise
     */
    public static boolean isValidIPv4Address (String ipAddress)
    {
        if (ipAddress == null) {
            return false;
        }
        String[] parts = ipAddress.split("\\.");

        if (parts.length != 4) {
            return false;
        }

        for (String s : parts) {
            int i;
            try {
                i = Integer.parseInt(s);
            }
            catch (NumberFormatException e) {
                return false;
            }

            if ((i < 0) || (i > 255)) {
                return false;
            }
        }

        return true;
    }

    /**
     * Method for the validation of the input port;
     *
     * @param port The IP Address input string
     * @return True if the IP is valid, false otherwise
     */
    public static boolean isValidPort (String port)
    {
        if (port == null || port.equals(""))
            return false;

        try {
            int iPort = Integer.parseInt(port);
            if (iPort < 0 || iPort > 65535) {
                return false;
            }
        }
        catch (NumberFormatException e) {
            return false;
        }

        return true;
    }

    public static String getPrimaryIP (Node node)
    {
        Objects.requireNonNull(node, "node can't be null");
        Objects.requireNonNull(node.getInfo(), "Info can't be null");

        if (node.getInfo().getNicsList() != null) {

            if (node.getInfo().getNicsList().size() == 1) {
                String ipAddress = node.getInfo().getNicsList().get(0).getIpAddress();

                if (isValidIPv4Address(ipAddress) && !ipAddress.equals("127.0.0.1")) {
                    return ipAddress;
                }
            }

            for (Network n : node.getInfo().getNicsList()) {
                if (n.getIsPrimary()) {
                    return n.getIpAddress();
                }
            }
        }


        return null;
    }

    public static void printAllNetworkInterfaces ()
    {
        try {
            log.info("Full list of Network Interfaces:");
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements(); ) {
                NetworkInterface intface = en.nextElement();
                log.info("  " + intface.getName() + " " + intface.getDisplayName());
                for (Enumeration<InetAddress> enumIpAddr = intface.getInetAddresses(); enumIpAddr.hasMoreElements(); ) {
                    log.info("    " + enumIpAddr.nextElement().toString());
                }
            }
        }
        catch (SocketException e) {
            log.info("(Error retrieving network interface list)");
        }
    }

    public static String getPrimaryNetworkInterfaceIP ()
    {
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements(); ) {
                NetworkInterface intface = en.nextElement();
                for (Enumeration<InetAddress> enumIpAddr = intface.getInetAddresses(); enumIpAddr.hasMoreElements(); ) {
                    String currentIp = enumIpAddr.nextElement().toString().replace("/", "");
                    if (!currentIp.equals("127.0.0.1") && isValidIPv4Address(currentIp)) {
                        log.info("Primary IP for machine detected as: " + currentIp);
                        return currentIp;
                    }
                }
            }
        }
        catch (SocketException e) {
            log.info("(Error retrieving network interface list)");
        }

        return null;
    }

    /**
     * Returns a <code>List</code> of <code>String</code> objects representing IPs
     * parsing a String containing a list of comma separated IPs.
     *
     * @param commaSeparatedIPs a list of comma separated IPs
     * @return a <code>List</code> of <code>String</code> objects representing IPs
     */
    public static List<String> getIPList (String commaSeparatedIPs)
    {
        String cleanIPs = commaSeparatedIPs.replaceAll("\\s+", "");

        String[] arrayIPs = cleanIPs.split(",");

        List<String> listIPs = new ArrayList<>();
        for (String s : arrayIPs) {
            if (isValidIPv4Address(s)) {
                listIPs.add(s);
            }
        }

        return listIPs;
    }


    /**
     * Gets the current timestamp in ISO 8601 format, e.g. 2014-02-15T01:02:03Z
     * <p>
     * (See: http://www.mscharhag.com/java/java-8-date-time-api)
     *
     * @return a <code>String</code> representing current timestamp
     */
    public static String getCurrentTimestamp ()
    {

        // current time
        //Instant now = Instant.now(); //Java 8 ONLY
        //return now.toString();
        return new SimpleDateFormat(Strings.Formats.DATE, Locale.US).format(new Date());
    }

    /**
     * Converts a timestamp in ISO 8601 format, e.g. 2014-02-15T01:02:03Z
     * to a Java <code>Data</code> object.
     * <p>
     * (See: http://www.mscharhag.com/java/java-8-date-time-api)
     *
     * @return a <code>String</code> representing current timestamp, null if parsing
     */
    public static Date fromTimestampToDate (String timestamp)
    {
        Objects.requireNonNull(timestamp, "Timestamp can't be null");
        DateFormat df = new SimpleDateFormat(Strings.Formats.DATE, Locale.US);
        Date result = null;
        try {
            result = df.parse(timestamp);
        }
        catch (ParseException e) {
            log.error("Unable to parse timestamp: " + timestamp
                    + " not in the correct format: "
                    + Strings.Formats.DATE);
        }

        return result;
    }

    /**
     * Verifies whether the new timestamp is more recent than the old timestamp.
     *
     * @param newTimestamp the new time stamp
     * @param oldTimestamp the old time stamp
     * @return true if the timestamp is more recent, false otherwise
     */
    public static boolean isTimestampMoreRecent (String newTimestamp, String oldTimestamp)
    {
        Objects.requireNonNull(newTimestamp, "NewTimestamp can't be null");
        Objects.requireNonNull(oldTimestamp, "OldTimestamp can't be null");
        Date newDate = Utils.fromTimestampToDate(newTimestamp);
        if (newDate == null) {
            return false;
        }
        Date oldDate = Utils.fromTimestampToDate(oldTimestamp);

        //TODO check why we need to assert equals too
        //TODO make sure to update timestamp ad every update of either NodeCore, NodeInfo or NodeStats
        if (newDate.after(oldDate) || newDate.equals(oldDate) || oldDate == null) {
            log.debug(newTimestamp + " is equal or more recent than " + oldTimestamp + ". Replacing..");
            return true;
        }
        log.debug(newTimestamp + " is NOT more recent than " + oldTimestamp);
        return false;
    }


    /**
     * Returns a human readable version of a byte count.
     *
     * @param bytes the bytes value
     * @param si    the format to use (SI = 1000, BI = 1024)
     * @return a <code>String</code> repre
     * senting the byte count.
     */
    public static String humanReadableByteCount (long bytes, boolean si)
    {
        int unit = si ? 1000 : 1024;
        if (bytes < unit) return bytes + " B";
        int exp = (int) (Math.log(bytes) / Math.log(unit));
        String pre = (si ? "kMGTPE" : "KMGTPE").charAt(exp - 1) + (si ? "" : "i");
        return String.format("%.1f %sB", bytes / Math.pow(unit, exp), pre);
    }

    /**
     * Extracts the log4j properties from the configuration file
     *
     * @param configFilePath log4j configuration file
     * @param logFolder      path of the folder containing the log files
     * @return a <code>Property</code> instance containing the log4j configuration
     */
    public static Properties getLog4jProperties (String configFilePath, String logFolder)
    {
        Properties log4jProperties = new Properties();
        try {
            log4jProperties.load(new FileInputStream(configFilePath));

            Date day = new Date();
            String formattedDate = new SimpleDateFormat("yyyyMMddHHmm").format(day);

            String logFileProperty = "log4j.appender.rollingFile.File";
            String logFileName = log4jProperties.getProperty(logFileProperty);
            log4jProperties.setProperty(logFileProperty, String.format(logFolder + File.separator + "%s-%s",
                    formattedDate,
                    logFileName));
        }
        catch (FileNotFoundException e) {
            System.err.println("Unable to load log4j configuration, file not found");
            e.printStackTrace();
        }
        catch (IOException e) {
            System.err.println("Unable to load log4j configuration, error while I/O on disk");
            e.printStackTrace();
        }

        return log4jProperties;
    }

    /**
     * Reads a <code>File</code> as a byte array from a specific path.
     *
     * @param filePath the path of the <code>File</code>
     * @return the byte array content
     */
    public static byte[] readFile (String filePath)
    {
        if (filePath == null) {
            throw new NullPointerException("File path can't be null");
        }

        byte[] fileContent;
        File f;
        FileInputStream fin;
        try {
            f = new File(filePath);
            fin = new FileInputStream(f);
            fileContent = new byte[(int) f.length()];
            fin.read(fileContent);
            log.info("File read successfully!");
            return fileContent;
        }
        catch (FileNotFoundException e) {
            log.error("File not found.");
            return null;
        }
        catch (IOException e) {
            log.error("Error while reading file content.");
            return null;
        }
    }

    /**
     * Writes a specific byte array to a <code>File</code> on the disk at the specific path.
     *
     * @param directoryPath the path of the directory of this <code>File</code>
     * @param filePath      the path of the <code>File</code>
     * @param data          the byte array content
     * @return true if write was successful, false otherwise
     */
    public static boolean writeToFile (String directoryPath, String filePath, byte[] data)
    {
        if (filePath == null) {
            throw new NullPointerException("File path can't be null");
        }

        File directory = new File(String.valueOf(directoryPath));
        if (!directory.exists()) {
            log.info("Directory: " + directory + " not present, creating it");
            directory.mkdirs();
        }

        FileOutputStream fos = null;
        boolean success = true;
        try {
            fos = new FileOutputStream(String.format(filePath));
            fos.write(data);
            log.info("Data written on file: " + filePath);
        }
        catch (IOException e) {
            log.warn("Error while writing file to disk.", e);
            success = false;
        }
        finally {
            if (fos != null) {
                try {
                    fos.flush();
                    fos.close();
                    success = true;
                }
                catch (IOException e) {
                    log.warn("Error while closing file.", e);
                    success = false;
                }
            }
        }

        return success;
    }

    private static final Logger log = Logger.getLogger(Utils.class);

}