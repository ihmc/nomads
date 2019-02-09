package netlogger.util.settings;

import netlogger.model.settings.SettingField;
import netlogger.model.settings.Updatable;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SettingsBridge
{
    public SettingsBridge () {
        _classWithSettingsTextMap = new HashMap<>();
        _objectForClass = new HashMap<>();
        _settingsList = new ArrayList<>();
    }

    public void updateAsync (final HashMap<String, Object> settings) {
        new Thread(() -> {
            for (Class<?> cl : _classWithSettingsTextMap.keySet()) {

                List<String> settingStrings = _classWithSettingsTextMap.get(cl);

                if (settingStrings == null) {
                    _logger.error("Class {} with @SettingUser annotation has no @SettingField fields", cl.getName());
                    continue;
                }

                // Put the classes settings into a hashmap for it to use
                HashMap<String, Object> settingsForClass = new HashMap<>();
                for (String settingString : settingStrings) {
                    settingsForClass.put(settingString, settings.get(settingString));
                }

                Updatable updatable = _objectForClass.get(cl);
                updatable.update(settingsForClass);
            }
        }, "Config updater thread").start();
    }

    public void addUpdatableClass (Updatable updatable) {
        _objectForClass.put(updatable.getClass(), updatable);
        registerSettingsForClass(updatable.getClass());
    }

    /**
     * Removes an updatable class from the table of updatable classes. This should be called when an object is destroyed.
     * For instance if NetLogger is used as a plugin to netviewer, this is a very important function to call.
     * @param updatable Class with the {@link Updatable} interface
     */
    public void removeUpdatableClass (Updatable updatable) {
        _objectForClass.remove(updatable.getClass());
        _classWithSettingsTextMap.remove(updatable.getClass());
    }

    public List<SettingField> getSettingsList () {
        return _settingsList;
    }

    private void registerSettingsForClass (Class<?> cl) {
        Field[] fields = cl.getDeclaredFields();
        for (Field field : fields) {
            SettingField annotation = field.getAnnotation(SettingField.class);
            if (annotation == null) {
                continue;
            }

            _settingsList.add(annotation);

            String settingText = annotation.settingText();
            List<String> settingStrings = _classWithSettingsTextMap.computeIfAbsent(cl, k -> new ArrayList<>());
            settingStrings.add(settingText);
        }
    }


    private Map<Class, List<String>> _classWithSettingsTextMap;
    private Map<Class, Updatable> _objectForClass;
    private List<SettingField> _settingsList;
    private static final Logger _logger = LoggerFactory.getLogger(SettingsBridge.class);
}
