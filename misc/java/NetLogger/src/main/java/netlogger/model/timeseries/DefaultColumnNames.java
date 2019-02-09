package netlogger.model.timeseries;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class DefaultColumnNames
{
    public static String SOURCE_APPLICATION_STRING = "source_application";
    public static String SOURCE_MODULE_STRING = "source_module";
    public static String DEST_APPLICATION_STRING = "dest_application";
    public static String DEST_MODULE_STRING = "dest_module";
    public static String PREV_DATA_ID_STRING = "prev_data_id";
    public static String CURR_DATA_ID_STRING = "curr_data_id";
    public static String CHECKSUM_STRING = "checksum";
    public static String DATA_TYPE_STRING = "type";
    public static String SOURCE_CLIENT_ID_STRING = "source_client_id";
    public static String DEST_CLIENT_ID_STRING = "dest_client_id";

    private static final Logger _logger = LoggerFactory.getLogger(DefaultColumnNames.class);
}
