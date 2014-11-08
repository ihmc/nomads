/*
 * MsgLinkedList.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

package us.ihmc.util;

/**
 *
 * <p>Title: MsgLinkedList.java</p>
 * <p>Description: This Class provides specific support for MsgQueue using Java
 * language release jdk 1.1  However, the MsgLinkList can used as a generic
 * LinkedList.  Java 2 provides LinkedLists.</p>
 * <p>Copyright: Copyright (c) 2 June 2003</p>
 * <p>Company: IHMC</p>
 * @Christopher J. Eagle < ceagle@ai.uwf.edu >
 * @version 1.0
 */
public class MsgLinkedList
{
    public class ListElement
    {
        public ListElement(Object obj)
        {
            this.obj = obj;
        }

        public Object obj;
        private ListElement next = null;
    }

    /**
     * If list is empty, then create first element, set head/tail "pointing"
     * to first and only element.  Else put object at the end of the list.
     *
     * @param o -- Object to be added to list
     * @return boolean
     */
    public boolean add(Object o)
    {
        if (isEmpty()) {
            head = new ListElement(o);
            tail = head;
        }
        else {
            tail.next = new ListElement(o);
            tail = tail.next;
        }
        count++;
        return true;
    }

    /**
     * Returns null if list is empty, else returns Object from part of list
     * requested.
     *
     * @param which -- int to indicate which object to remove from list.
     * @return Object
     */
    public Object remove(int which)
    {
        Object obj = null;

        if (isEmpty()) {
            return obj;
        }

        if (which == 0) {
            obj = head.obj;
            if (head == tail) {
                // list had one item, will now be empty.
                head = null;
                tail = null;
            }
            else {
                head = head.next;
            }
        }
        else {
            ListElement hold, tmp;

            if (which >= count) {
                return obj;
            }

            tmp = head;
            for (int j = 1; j < which; j++) {
                tmp = tmp.next;
            }

            hold = tmp.next;
            obj = hold.obj;
            hold = hold.next;

            tmp.next = hold;
            if (which == (count-1)) {
                tail = hold;
            }

            hold = null;
            tmp = null;
        }
        count--;
        return obj;
    }

    /**
     * @return int -- number of elements in the list.
     */
    public int size()
    {
        return count;
    }

    /**
     * @return boolean -- true if list is empty (has no items).
     */
    private boolean isEmpty()
    {
        if (this.head == null) {
            return true;
        }
        else {
            return false;
        }
    }

    private ListElement head = null;
    private ListElement tail = null;
    private int count = 0;
}
