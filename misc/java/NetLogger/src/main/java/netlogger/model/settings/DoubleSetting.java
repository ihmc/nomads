package netlogger.model.settings;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * @author Blake Ordway (bordway@ihmc.us) on 8/15/2018
 */
public class DoubleSetting extends Setting<Double>
{
    public DoubleSetting () {
        _doubleOriginator = new Originator<>();
        _doubleCaretaker = new Caretaker<>();
    }

    @Override
    public void saveCurrentState (Double state) {
        _doubleOriginator.set(state);
        _doubleCaretaker.setCurrentState(_doubleOriginator.saveToMemento());
    }

    @Override
    public void saveOriginalState (Double state) {
        _doubleOriginator.set(state);
        _doubleCaretaker.setOriginalState(_doubleOriginator.saveToMemento());
    }

    @Override
    public Double getCurrentState () {
        _doubleOriginator.restoreFromMemento(_doubleCaretaker.getCurrentState());
        return _doubleOriginator.getState();
    }

    @Override
    public Double getOriginalState () {
        _doubleOriginator.restoreFromMemento(_doubleCaretaker.getOriginalState());
        return _doubleOriginator.getState();
    }

    private Originator<Double> _doubleOriginator;
    private Caretaker<Originator.Memento<Double>> _doubleCaretaker;

    private static final Logger _logger = LoggerFactory.getLogger(DoubleSetting.class);
}
