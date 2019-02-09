package netlogger.model.settings;

public class IntegerSetting extends Setting<Integer>
{
    public IntegerSetting () {
        _integerOriginator = new Originator<>();
        _integerCaretaker = new Caretaker<>();
    }

    @Override
    public void saveCurrentState (Integer state) {
        _integerOriginator.set(state);
        _integerCaretaker.setCurrentState(_integerOriginator.saveToMemento());
    }

    @Override
    public void saveOriginalState (Integer state) {
        _integerOriginator.set(state);
        _integerCaretaker.setOriginalState(_integerOriginator.saveToMemento());
    }

    @Override
    public Integer getCurrentState () {
        _integerOriginator.restoreFromMemento(_integerCaretaker.getCurrentState());
        return _integerOriginator.getState();
    }

    @Override
    public Integer getOriginalState () {
        _integerOriginator.restoreFromMemento(_integerCaretaker.getOriginalState());
        return _integerOriginator.getState();
    }

    private Originator<Integer> _integerOriginator;
    private Caretaker<Originator.Memento<Integer>> _integerCaretaker;
}
