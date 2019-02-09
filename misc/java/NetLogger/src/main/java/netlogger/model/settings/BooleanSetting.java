package netlogger.model.settings;

public class BooleanSetting extends Setting<Boolean>
{
    public BooleanSetting () {
        _booleanOriginator = new Originator<>();
        _booleanCaretaker = new Caretaker<>();
    }

    @Override
    public void saveCurrentState (Boolean state) {
        _booleanOriginator.set(state);
        _booleanCaretaker.setCurrentState(_booleanOriginator.saveToMemento());
    }

    @Override
    public void saveOriginalState (Boolean state) {
        _booleanOriginator.set(state);
        _booleanCaretaker.setOriginalState(_booleanOriginator.saveToMemento());
    }

    @Override
    public Boolean getCurrentState () {
        _booleanOriginator.restoreFromMemento(_booleanCaretaker.getCurrentState());
        return _booleanOriginator.getState();
    }

    @Override
    public Boolean getOriginalState () {
        _booleanOriginator.restoreFromMemento(_booleanCaretaker.getOriginalState());
        return _booleanOriginator.getState();
    }

    private Originator<Boolean> _booleanOriginator;
    private Caretaker<Originator.Memento<Boolean>> _booleanCaretaker;
}
