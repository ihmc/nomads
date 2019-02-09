package netlogger.controller.settings;

import netlogger.util.ConfigInjector;

import java.util.HashMap;

public interface SettingsController extends ConfigInjector
{
    void saveToConfig ();

    void revertAll ();

    void initializeFromConfigFile ();

    void initializeListeners ();

    HashMap<String, Object> getSettings ();
}
