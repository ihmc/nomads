package netlogger.util;


import java.io.*;
import java.util.Properties;

public class ConfigFileManager
{
    public ConfigFileManager () {
        _properties = new Properties();
    }

    public void init (String configFilePath) {
        _configFile = configFilePath;
        loadFile();
    }


    public int getIntegerValue (String propertyName) {
        String val = _properties.getProperty(propertyName);
        int intVal;

        try {
            intVal = Integer.valueOf(val);
        } catch (NumberFormatException e) {
            intVal = 0;
        }

        return intVal;
    }

    public boolean getBooleanValue (String propertyName) {
        String val = _properties.getProperty(propertyName);
        boolean boolVal;

        try {
            boolVal = Boolean.valueOf(val);
        } catch (NumberFormatException e) {
            boolVal = true;
        }

        return boolVal;
    }

    public String getStringValue (String propertyName) {
        return _properties.getProperty(propertyName);
    }

    public double getDoubleValue (String propertyName) {
        String val = _properties.getProperty(propertyName);
        double doubleVal;

        try {
            doubleVal = Double.valueOf(val);
        } catch (NumberFormatException e) {
            doubleVal = 0;
        }

        return doubleVal;
    }


    public int getIntegerValue (String propertyName, int defaultValue) {
        String val = _properties.getProperty(propertyName, String.valueOf(defaultValue));
        int intVal;

        try {
            intVal = Integer.valueOf(val);
        } catch (NumberFormatException e) {
            intVal = defaultValue;
        }

        return intVal;
    }

    public boolean getBooleanValue (String propertyName, boolean defaultValue) {
        String val = _properties.getProperty(propertyName, String.valueOf(defaultValue));
        boolean boolVal;

        try {
            boolVal = Boolean.valueOf(val);
        } catch (NumberFormatException e) {
            boolVal = defaultValue;
        }

        return boolVal;
    }

    public String getStringValue (String propertyName, String defaultValue) {
        return _properties.getProperty(propertyName, defaultValue);
    }

    public double getDoubleValue (String propertyName, double defaultValue) {
        String val = _properties.getProperty(propertyName, String.valueOf(defaultValue));
        double doubleVal;

        try {
            doubleVal = Double.valueOf(val);
        } catch (NumberFormatException e) {
            doubleVal = defaultValue;
        }

        return doubleVal;
    }

    public void updateConfig (String propertyName, Object propertyValue) {
        if (_configFile != null) {
            try (OutputStream output = new FileOutputStream(_configFile)) {
                _properties.setProperty(propertyName, propertyValue.toString());
                _properties.store(output, null);

            } catch (IOException e) {
                e.printStackTrace();
            }

            loadFile();
        }
    }

    private void loadFile () {
        try (InputStream input = new FileInputStream(_configFile)) {
            _properties.load(input);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private Properties _properties;
    private String _configFile;
}
