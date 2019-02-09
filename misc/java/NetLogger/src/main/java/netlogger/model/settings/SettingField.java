package netlogger.model.settings;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;


@Target(ElementType.FIELD)
@Retention(RetentionPolicy.RUNTIME)
public @interface SettingField
{
    /**
     * Text that will be displayed in the settings window
     *
     * @return
     */
    String settingText ();

    /**
     * The key of this setting in the config file
     *
     * @return
     */
    String configText ();

    /**
     * The default value if the key isn't found in the config file
     *
     * @return
     */
    String defaultValue ();

}

