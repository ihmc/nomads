package netlogger;

import main.DatabrokerAdapter;
import netlogger.model.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;
import java.util.function.Consumer;

/**
 * Generates test strings to be shown in the list view.
 * The test receiver essentially bypasses a few classes since those classes depend on receiving measures.
 * @author Blake Ordway (bordway@ihmc.us) on 8/24/2018
 */
public class TestReceiver extends DatabrokerAdapter
{
    @Override
    public void run(){
        while (!_terminationRequested){
                _handler.accept(createNewStyledString());
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    private StyledString createNewStyledString(){
        StringMeasure stringMeasure = StringMeasureFactory.createStringMeasure();

        stringMeasure.setString("This is a test string. Current count is now: " + _count);
        _count++;

        return new StyledString(stringMeasure);
    }


    public void setConsumer(Consumer<StyledString> handler){
        _handler = handler;
    }


    private Consumer<StyledString> _handler;


    @Override
    public void connect () {

    }

    @Override
    public void setInfo (String s, String s1, List<String> list) {

    }

    private int _count = 0;

    private static final Logger _logger = LoggerFactory.getLogger(TestReceiver.class);
}
