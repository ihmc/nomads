package netlogger.model.settings;

public class Originator<T>
{
    public void set (T state) {
        _state = state;
    }

    public Memento<T> saveToMemento () {
        return new Memento<>(_state);
    }

    public void restoreFromMemento (Memento<T> memento) {
        _state = memento.getSavedState();
    }

    public T getState () {
        return _state;
    }


    public static final class Memento<T>
    {
        public Memento (T stateToSave) {
            _state = stateToSave;
        }

        private T getSavedState () {
            return _state;
        }

        private final T _state;
    }

    private T _state;
}
