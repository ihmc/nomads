package us.ihmc.mockets;

import java.io.IOException;

class NotEnoughSizeException extends IOException
{
    NotEnoughSizeException (int availableSize, int neededSize) {
        _availableSize = availableSize;
        _neededSize = neededSize;
    }

    int getAvailableSize() {
        return _availableSize;
    }

    int getNeededSize() {
        return _neededSize;
    }

    private int _availableSize;
    private int _neededSize;
}
/*
 * vim: et ts=4 sw=4
 */

