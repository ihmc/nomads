package us.ihmc.aci.grpMgrOld.gui;

public class ListEntry implements Comparable
{
    public String name;
    public boolean local;
    public boolean remote;
    public boolean member;

    public String toString()
    {
        return name;
    }
    
    public int compareTo (Object o)
    {
        return name.compareToIgnoreCase (((ListEntry)o).name);
    }
}
