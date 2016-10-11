/*
 * SUDADSProProxy.java
 *
 * This file is part of the IHMC ACI Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.dspro2;

import us.ihmc.aci.dspro2.legacy.LegacyDSProProxy;
import us.ihmc.aci.dspro2.util.LoggerInterface;
import us.ihmc.aci.dspro2.util.LoggerWrapper;
import us.ihmc.aci.util.dspro.soi.TrackHandler;
import us.ihmc.comm.CommException;


/**
 * DSProProxy version that cancel the received Soi Native Tracks from DSPro
 * and DS data caches and pushes generated Tracks at every setActualPosition().
 *
 * @author Giacomo Benincasa (gbenincasa@ihmc.us), Enrico Casini (ecasini@ihmc.us)
 */
public class SUDADSProProxy extends DSProProxy
{
    public SUDADSProProxy()
    {
        super();
    }

    public SUDADSProProxy (short applicationId)
    {
        super(applicationId);
    }

    public SUDADSProProxy (short applicationId, String host, int iPort)
    {
        super(applicationId, host, iPort);
    }

    @Override
    public int init()
    {
        _trackHandler = new TrackHandler (new LegacyDSProProxy (this));
        int rc = super.init();
        if (rc >= 0) {
            try {
                getNodeId();
            }
            catch (CommException ex) {
                LOG.warn (ex.getMessage());
            }
            new Thread (_trackHandler).start();
        }
        return rc;
    }

    @Override
    public synchronized boolean setCurrentPosition (float fLatitude, float fLongitude, float fAltitude,
                                                   String location, String note) throws CommException
    {
        _trackHandler.setActualPosition (getNodeId(), fLatitude, fLongitude, fAltitude, location, note);
        return super.setCurrentPosition (fLatitude, fLongitude, fAltitude, location, note);
    }

    private TrackHandler _trackHandler;
    private final static LoggerInterface LOG = LoggerWrapper.getLogger (SUDADSProProxy.class);
}
