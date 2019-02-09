package us.ihmc.cue.OSUStoIMSBridgePlugin;

import mil.dod.th.core.log.Logging;
import org.osgi.service.log.LogService;

import java.io.*;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;

/**
 * @author Rita Lenzi (rlenzi@ihmc.us) - 6/8/2017
 * @author Blake Ordway (bordway@ihmc.us) - 7/18/2018
 */
public class FederationPublisher
{
    public FederationPublisher () {
        _publisherData = new PublisherData();
    }

    /**
     * Sets the FCID and updates the pubURL
     * @param federationCIDString New client ID
     */
    public void setFederationCIDString (String federationCIDString) {
        Logging.log(LogService.LOG_INFO, "Setting publisher's client id to: " + federationCIDString);
        _publisherData.setFederationCIDString(federationCIDString);
        _publisherData.updatePubURL();
    }

    /**
     * Updates the host and port information for the publisher. The Federation CID is also set to null so that no messages
     * are published before the new Federation instance has a chance to give our subscriber a client ID
     * @param host
     * @param port
     */
    public void updatePubURL (String host, int port) {
        _publisherData.setHost(host);
        _publisherData.setPort(port);

        _publisherData.setFederationCIDString(null);
    }

    /**
     * Tries to publish a new json string. In the future this may be updated to hold onto the string and publish when the
     * federationCID is set.
     * @param json String to try to send
     */
    public void tryPublish (String json) {
        if (json == null || json.isEmpty()){
            Logging.log(LogService.LOG_ERROR, "Publisher::Cannot publish null or empty json string");
        }
        // If there's a federation client id, we can tryPublish this
        _publisherData.getFederationCIDString().ifPresent(fedCID -> {
            publish(json, _publisherData.getPubURL());
        });
    }

    /**
     * Publish the json to the URL
     * @param json JSON string to send
     * @param url URL to send JSON to
     */
    private void publish (String json, String url) {
        OutputStreamWriter out = null;
        BufferedReader in = null;
        InputStreamReader is = null;

        try {
            URLConnection connection = (new URL(url)).openConnection();
            connection.setDoOutput(true);
            connection.setRequestProperty("Content-Type", "application/json");
            connection.setConnectTimeout(5000);
            connection.setReadTimeout(5000);
            out = new OutputStreamWriter(connection.getOutputStream());
            out.write(json);
            Utils.safeClose(out);
            StringBuilder builder = new StringBuilder();
            is = new InputStreamReader(connection.getInputStream());
            in = new BufferedReader(is);

            String line;
            while ((line = in.readLine()) != null) {
                builder.append(line);
            }

            Utils.safeClose(in);
            Utils.safeClose(is);
            Logging.log(LogService.LOG_INFO,"Published observation to IMSBridge. Server response: " + builder.toString());
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            Utils.safeClose(out);
            Utils.safeClose(in);
            Utils.safeClose(is);
        }
    }

    private PublisherData _publisherData;
}
