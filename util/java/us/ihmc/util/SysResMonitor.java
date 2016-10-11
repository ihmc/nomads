/*
 * SysResMonitor.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

public class SysResMonitor
{
    public SysResMonitor()
    {
        checkLibrary();
    }
    
    /** Obtain a list of network devices available on the given
     * system. Some systems may benefit by having a variety of active
     * external interfaces - for eg., 802.11{a,b,g}, Wired Ethernet
     * at 10/100/1000, Bluetooth, IrDA, etc. 
     * 
     * We'll attempt to return a string that will indicate in
     * some detail what the device is, as opposed to just a
     * device identifier like 'eth0'.
     */

    public String[] enumerateNetworkInterfaces()
    {
        return getNativeNetworkInterfaceList();
    }  
    
    /** Get the speed of the link and return it in bps(bits per
     * second), as we expect we'll have a wide range of types
     * of connections. Handle the case where device is not indicated.
     * We'll attempt to default to whichever wireless device is
     * available. 
     */
    public int getNetworkLinkSpeed()
    {
        return getNetworkLinkSpeed (defaultInterface);
    }    
    
    public int getNetworkLinkSpeed (String networkInterface)
    {
        return getNativeNetworkLinkSpeed (networkInterface);
    }   
    
    /** Get some indication of the quality of the wireless link,
     * assuming one exists. If -1 is returned, unable to get it
     * for whatever reason.
     */
    public int getWirelessLinkQuality()
    {
        return getWirelessLinkQuality (defaultInterface);
    }    
    
    public int getWirelessLinkQuality (String networkInterface)
    {
        return getNativeWirelessLinkQuality (networkInterface);
    }    
    
    /** Get the ssid of the active WiFi wireless adapter.
     */
    public int getWirelessSSID()
    {
        return getWirelessSSID (defaultInterface);
    }    
    
    public int getWirelessSSID (String networkInterface)
    {
        return getNativeWirelessSSID (networkInterface);
    }    
        
    
    /** Get the channel of the active WiFi wireless adapter.
     * Currently, 802.11b is assumed, thus ISM band, which has
     * channels 1-11.
     */
    public int getWirelessChannel()
    {
        return getWirelessChannel (defaultInterface);
    }    
    
    public int getWirelessChannel (String networkInterface)
    {
        return getNativeWirelessChannel (networkInterface);
    }    
    
    /** Set the channel of the active WiFi wireless adapter.
     * Currently, 802.11b is assumed, thus ISM band, which has
     * channels 1-11.
     */
    public boolean setWirelessChannel(int channel)
    {
        return setWirelessChannel (defaultInterface, channel);
    }    
    
    public boolean setWirelessChannel (String networkInterface, int channel)
    {
        return setNativeWirelessChannel (networkInterface, channel);
    }    
    
    /** Get the raw byte count on the incoming stream on the network 
     * interface. This will be returned in bps. Like the CPU utilization,
     * nice running averages may not be available, and will need to be
     * calculated by the caller.
     */
    public int getInboundNetworkUtilization()
    {
        return getInboundNetworkUtilization (defaultInterface);
    }

    public int getInboundNetworkUtilization (String networkInterface)
    {
        return getNativeInboundNetworkUtilization (networkInterface);
    }

    /** Get the raw byte count on the outbound stream on the network 
     * interface. This will be returned in bps. Like the CPU utilization,
     * nice running averages may not be available, and will need to be
     * calculated by the caller.
     */
    public int getOutboundNetworkUtilization()
    {
        return getOutboundNetworkUtilization (defaultInterface);
    }

    public int getOutboundNetworkUtilization (String networkInterface)
    {
        return getNativeOutboundNetworkUtilization (networkInterface);
    }

    /** Get a count of the number of currently established TCP
     * connections. 
     */
    public int getEstablishedTCPConnections()
    {
        return getEstablishedTCPConnections (defaultInterface);
    }
    
    public int getEstablishedTCPConnections (String networkInterface)
    {
        return getNativeEstablishedTCPConnections (networkInterface);
    }

    /** Get the utilization of the processor as a percentage. 
     * This may take five seconds or so to obtain as most systems
     * do not keep a running counter that could give nice last
     * 5/15/30 minute averages. It will be up to the calling method
     * to calculate such averages over longer time periods.
     */
    public int getOverallCPUUtilization()
    {
        return getNativeOverallCPUUtilization();
    }
    
     /** Get the type of CPU that this particular host has -
      * this may be returned as a string that contains more
      * than just 'Pentium' or 'ARM', but some info that might 
      * indicate other pertinent info that distiguishes processor
      * family, and relative power, such that the general level
      * of this host can be discerned by the caller.
     */   
    public int getCPUType()
    {
        return getNativeCPUType();
    }
    
    /** Get the MHz rating of this hosts processor. 
     */
    public int getCPUSpeedRating()
    {
        return getNativeCPUSpeedRating();
    }
    
    /** Get the system architecture - such as 'Pocket/PC' or 
     * 'PC104' or 'PC' so that overall utility of host can
     * be determined. 
     */
    public int getSystemArchitecture()
    {
        return getNativeSystemArchitecture();
    }
    
    public String getOSName()
    {
        return getNativeOSName();
    }
    
    public String getOSVersion()
    {
        return getNativeOSVersion();
    }
    
    /** Get the memory utilzation as a percentage.
     */
    public int getMemoryUtilization()
    {
        return  getNativeMemoryUtilization();
    }
    
    /** Get the memory size as an absolute value in MB(megabytes).
     */
    public int getMemorySize()
    {
        return  getNativeMemorySize();
    }
    
    /** Determine power source that host is currently operating 
     * on. If batteries, then take this into account in calculating 
     * usefullness of host with respect to anticipated duration
     * of stream life. POWER_AC == 1 and POWER_BATTERY == 2.
     */
    public int getPowerSource()
    {
        return  getNativePowerSource();
    }
    
    /** Determine how much of the batteries life is remaining
     * as a percentage.
     */
    public int getBatteryPowerRemaining()
    {
        return  getNativeBatteryPowerRemaining();
    }
    
    /** Determine how much of the batteries life is remaining
     * as a absolute value in seconds. A -1 is returned if this
     * cannot be ascertained.
     */
    public int getBatteryTimeRemaining()
    {
        return  getNativeBatteryTimeRemaining();
        
    }   ////////////////// VOID ///////////////////////
    private void checkLibrary()
    {
        if (!_sysResLoaded) {
            Runtime rt = Runtime.getRuntime();
            rt.loadLibrary ("SysResMonitor");
            _sysResLoaded = true;
        }
        
    }

    
    private SysResMonitor _resMon = null; 
    private boolean _sysResLoaded = false;
    private String defaultInterface = "wireless";
    
    private native String[] getNativeNetworkInterfaceList();
    private native int getNativeNetworkLinkSpeed (String networkInterface);
    private native int getNativeWirelessLinkQuality (String networkInterface);
    private native int getNativeWirelessSSID (String networkInterface);
    private native int getNativeWirelessChannel (String networkInterface);
    private native boolean setNativeWirelessChannel (String networkInterface, int channel);
    private native int getNativeEstablishedTCPConnections (String networkInterface);
    private native int getNativeInboundNetworkUtilization (String networkInterface);
    private native int getNativeOutboundNetworkUtilization (String networkInterface);
    private native int getNativeCPUType();
    private native int getNativeCPUSpeedRating();
    private native int getNativeOverallCPUUtilization();
    private native int getNativeSystemArchitecture();
    private native String getNativeOSName();
    private native String getNativeOSVersion();
    private native int getNativeMemoryUtilization();
    private native int getNativeMemorySize();
    private native int getNativePowerSource();
    private native int getNativeBatteryPowerRemaining();
    private native int getNativeBatteryTimeRemaining();
}
