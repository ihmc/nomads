package netlogger.util.settings;

public class SettingsManager
{
    private SettingsManager () {
        _settingsBridge = new SettingsBridge();
    }

    public static SettingsManager getInstance () {
        if (_instance == null) {
            _instance = new SettingsManager();
        }

        return _instance;
    }

    public SettingsBridge getSettingsBridge () {
        return _settingsBridge;
    }

    private SettingsBridge _settingsBridge;
    private static SettingsManager _instance;
}
