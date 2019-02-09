package netlogger.model.settings;

public class Caretaker<T>
{
    public Caretaker () {
    }

    public void setCurrentState (T state) {
        _currentState = state;
    }

    public void setOriginalState (T state) {
        _originalState = state;
    }

    public T getOriginalState () {
        return _originalState;
    }

    public T getCurrentState () {
        return _currentState;
    }


    private T _originalState;
    private T _currentState;
}
