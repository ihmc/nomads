package netlogger.model.settings;

public class StringSetting extends Setting<String>
{
    public StringSetting () {
        _stringOriginator = new Originator<>();
        _stringCaretaker = new Caretaker<>();
    }

    @Override
    public void saveCurrentState (String state) {
        _stringOriginator.set(state);
        _stringCaretaker.setCurrentState(_stringOriginator.saveToMemento());
    }

    @Override
    public void saveOriginalState (String state) {
        _stringOriginator.set(state);
        _stringCaretaker.setOriginalState(_stringOriginator.saveToMemento());
    }

    @Override
    public String getCurrentState () {
        _stringOriginator.restoreFromMemento(_stringCaretaker.getCurrentState());
        return _stringOriginator.getState();
    }

    @Override
    public String getOriginalState () {
        _stringOriginator.restoreFromMemento(_stringCaretaker.getOriginalState());
        return _stringOriginator.getState();
    }

    private Originator<String> _stringOriginator;
    private Caretaker<Originator.Memento<String>> _stringCaretaker;
}
