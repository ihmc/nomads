package netlogger.model.kibana;

import org.apache.http.protocol.HTTP;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

public class RequestTest
{
    public static void main (String[] args) throws Exception {
        String url = "http://128.49.235.115:5601" + Requests.getVisualizationsRequestString();

        URL obj = new URL(url);
        HttpURLConnection con = (HttpURLConnection) obj.openConnection();

        // optional default is GET
        con.setRequestMethod("GET");

        //add request header
        con.setRequestProperty("User-Agent", HTTP.USER_AGENT);

        int responseCode = con.getResponseCode();
        _logger.info("\nSending 'GET' request to URL : " + url);
        _logger.info("Response Code : " + responseCode);

        BufferedReader in = new BufferedReader(
                new InputStreamReader(con.getInputStream()));
        String inputLine;
        StringBuilder response = new StringBuilder();

        while ((inputLine = in.readLine()) != null) {
            response.append(inputLine);
        }
        in.close();


        _logger.info(response.toString());

    }

    private static final Logger _logger = LoggerFactory.getLogger(RequestTest.class);
}
