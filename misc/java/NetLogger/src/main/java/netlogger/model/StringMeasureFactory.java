package netlogger.model;

import com.google.protobuf.util.Timestamps;
import netlogger.util.DateUtil;
import measure.proto.Measure;

import java.text.ParseException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Create a new StringMeasure based on the measure
 */
public class StringMeasureFactory
{
    public static StringMeasure createStringMeasure (Measure measure, String natsTopic) {
        String date = "";
        try {
            date = DateUtil.parseRFC3339Date(Timestamps.toString(measure.getTimestamp()));
        } catch (ParseException e) {
            e.printStackTrace();
        }

        StringBuilder stringsStringBuilder = new StringBuilder();
        StringBuilder integersStringBuilder = new StringBuilder();
        StringBuilder doublesStringBuilder = new StringBuilder();
        List<String> measureKeys = new ArrayList<>();

        Map<String, String> stringsMap = measure.getStringsMap();
        Map<String, Long> integersMap = measure.getIntegersMap();
        Map<String, Double> doublesMap = measure.getDoublesMap();

        String subjectKey = "Subject";
        measureKeys.add(subjectKey);

        for (String key : stringsMap.keySet()) {
            String initialCapsKey = key.substring(0, 1).toUpperCase() + key.substring(1);
            stringsStringBuilder.append(initialCapsKey).append(": ").append(stringsMap.get(key)).append(" ");

            measureKeys.add(initialCapsKey);
        }

        for (String key : integersMap.keySet()) {
            String initialCapsKey = key.substring(0, 1).toUpperCase() + key.substring(1);
            integersStringBuilder.append(initialCapsKey).append(": ").append(integersMap.get(key)).append(" ");

            measureKeys.add(initialCapsKey);
        }

        for (String key : doublesMap.keySet()) {
            String initialCapsKey = key.substring(0, 1).toUpperCase() + key.substring(1);
            doublesStringBuilder.append(initialCapsKey).append(": ").append(doublesMap.get(key)).append(" ");

            measureKeys.add(initialCapsKey);
        }

        return createStringMeasure(date, natsTopic,
                subjectKey + ": " + measure.getSubject().toString(),
                stringsStringBuilder.toString(), integersStringBuilder.toString(),
                doublesStringBuilder.toString(),
                measureKeys);
    }

    public static StringMeasure createStringMeasure (String timestamp, String natsTopic, String subject,
                                                     String stringsString, String intsString, String doublesString,
                                                     List<String> keys) {
        StringMeasure newStringMeasure = createStringMeasure();
        newStringMeasure.setTimestamp(timestamp);
        newStringMeasure.setNatsTopic(natsTopic);

        newStringMeasure.setString(subject + " " + stringsString + intsString + doublesString);
        newStringMeasure.setKeyList(keys);

        return newStringMeasure;
    }

    public static StringMeasure createStringMeasure () {
        return new StringMeasure();
    }
}
