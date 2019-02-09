package netlogger.model;

import javafx.collections.ModifiableObservableListBase;

import java.util.Collection;
import java.util.LinkedList;

public class ObservableLimitedList<T> extends ModifiableObservableListBase<T>
{

    public ObservableLimitedList (int maxSize) {
        _maxSize = maxSize;
        _list = new LinkedList<>();
    }

    public void changeSize (int newSize) {
        _maxSize = newSize;
    }

    @Override
    public boolean add (T element) {
        boolean result = super.add(element);
        if (size() > _maxSize) {
            remove(0);
        }
        return result;
    }

    @Override
    public boolean addAll (Collection<? extends T> c) {
        super.beginChange();
        try {
            boolean res = super.addAll(c);
            if (size() > _maxSize) {
                removeRange(0, size() - _maxSize);
            }

            return res;
        } finally {
            endChange();
        }
    }

    @Override
    public boolean addAll (int index, Collection<? extends T> c) {
        super.beginChange();
        try {
            boolean res = super.addAll(index, c);
            if (size() > _maxSize) {
                removeRange(0, size() - _maxSize);
            }
            return res;
        } finally {
            endChange();
        }
    }

    @Override
    public T get (int index) {
        return _list.get(index);
    }

    @Override
    public int size () {
        return _list.size();
    }

    @Override
    protected void doAdd (int index, T element) {
        _list.add(index, element);
    }

    @Override
    protected T doSet (int index, T element) {
        return _list.set(index, element);
    }

    @Override
    protected T doRemove (int index) {
        return _list.remove(index);
    }

    private LinkedList<T> _list;
    private int _maxSize;
}
