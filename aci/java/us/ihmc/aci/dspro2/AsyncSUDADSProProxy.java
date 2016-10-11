/*
 * AsyncSUDADSProProxy.java
 *
 * This file is part of the IHMC ACI Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.dspro2;
import us.ihmc.aci.dspro2.legacy.LegacyDSProProxy;
import us.ihmc.aci.util.dspro.soi.TrackHandler;
import us.ihmc.comm.CommException;

/**
 *
 * @author gbenincasa
 */
public class AsyncSUDADSProProxy extends AsyncDSProProxy
{
    public AsyncSUDADSProProxy()
    {
        super (DEFAULT_POLLING_TIME);
    }

    public AsyncSUDADSProProxy (long pollingTime)
    {
        super (pollingTime);
    }

    public AsyncSUDADSProProxy (short applicationId)
    {
        super (applicationId);
    }

    public AsyncSUDADSProProxy (short applicationId, long pollingTime)
    {
        super (applicationId, pollingTime);
    }

    public AsyncSUDADSProProxy (short applicationId, String host, int iPort)
    {
    	super (applicationId, host, iPort);
    }

    public AsyncSUDADSProProxy (short applicationId, String host, int iPort, long pollingTime)
    {
    	super (applicationId, host, iPort, pollingTime);
    }

    @Override
    public int init()
    {
        _trackHandler = new TrackHandler (new LegacyDSProProxy (this));
        int rc = super.init();
        try {
            getNodeId();
        }
        catch (CommException ex) {
            LOG.warn (ex.getMessage());
        }
        new Thread (_trackHandler).start();
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
}
