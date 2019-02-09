package netlogger.model.settings;

import java.util.List;

public class ListSetting extends Setting<List<String>>
{

    public ListSetting () {
        _listOriginator = new Originator<>();
        _listCaretaker = new Caretaker<>();
    }


    @Override
    public void saveCurrentState (List<String> state) {
        _listOriginator.set(state);
        _listCaretaker.setCurrentState(_listOriginator.saveToMemento());
    }

    @Override
    public void saveOriginalState (List<String> state) {
        _listOriginator.set(state);
        _listCaretaker.setOriginalState(_listOriginator.saveToMemento());
    }

    @Override
    public List<String> getCurrentState () {
        _listOriginator.restoreFromMemento(_listCaretaker.getCurrentState());
        return _listOriginator.getState();
    }

    @Override
    public List<String> getOriginalState () {
        _listOriginator.restoreFromMemento(_listCaretaker.getOriginalState());
        return _listOriginator.getState();
    }

    private Originator<List<String>> _listOriginator;
    private Caretaker<Originator.Memento<List<String>>> _listCaretaker;
}
