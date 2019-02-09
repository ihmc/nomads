package mil.army.cpi.ba4.utils;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

import mil.army.cpi.ba4.discoveryLib.AbstractDiscoveredSystem;

public class Utils {

    private static final String TAG = Config.class.getSimpleName();
    public static final String DEFAULT_CONFIG_FILEPATH = "/sdcard/default.conf";
    public static final String FILE_OUTPUT_FILEPATH = "/sdcard/config_output.conf";

    public Config loadConfig(String filePath) {
        String json = readFromFile(filePath);
        return gsonToObj(json);
    }

    public boolean fileExists(String filePath) {
        File f = new File(filePath);
        if(f.exists() && !f.isDirectory()) {
            return true;
        }
        else {
            return false;
        }
    }

    public void writeConfig (String filePath, Config config) {
        write2File(filePath, gsonToJson(config));
    }

    private String readFromFile(String filePath) {
        String content = "";

        try {
            File file = new File(filePath);
            Scanner scanner = new Scanner(file);

            while (scanner.useDelimiter("\\Z").hasNext()) {
                content += scanner.useDelimiter("\\Z").next();
            }

        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }

        return content;
    }

    private Config gsonToObj(String json) {
        Gson gson = new Gson();
        Config config = gson.fromJson(json, Config.class);

        System.out.println("CONFIG DATA: " + config.getNodeName() + " " + config.getURN());
        return config;
    }

    public void createConfigFromPeerData(AbstractDiscoveredSystem discoveredSystemData) {
        write2File(FILE_OUTPUT_FILEPATH, gsonToJson(convertPeerData2Config(discoveredSystemData)));
    }

    private Config convertPeerData2Config(AbstractDiscoveredSystem discoveredSystemData) {
        Config config = new Config();
        return config;
    }

    private String gsonToJson(Config config) {
        Gson gson = new GsonBuilder().setPrettyPrinting().create();
        String json = gson.toJson(config);
        return  json;
    }

    private void write2File(String filePath, String testString) {
        try {
            FileWriter file = new FileWriter(filePath);
            file.write(testString);
            file.flush();
            file.close();

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public Config createDummyConfig() {
        Config config = new Config();

        config.setIdentity(69, 181, 1, 'A', 1216616, 1216617, 1216617, 248, "SFGPEWR");

        config.addService("", 0, 65530);
        config.addService("", 19, 65529);
        config.addService("", 13, 65528);
        config.addService("", 71, 65527);
        config.addService("", 46, 65526);

        config.addNetwork(0, 0, 1, "148.25.76.122");
        config.addNetwork(12, 0, 10, "113.25.66.52");

        config.setNodeName("CO_NW_1-MOCK");
        config.setURN(1048106);

        List<String> groups = new ArrayList<>();
        groups.add("CO");
        groups.add("BN");
        groups.add("BDE");
        config.setGroups(groups);

        return config;
    }
}