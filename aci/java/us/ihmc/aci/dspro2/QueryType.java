/*
 * QueryType.java
 *
 * This file is part of the IHMC ACI Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.dspro2;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class QueryType
{
    public static String SQL_ON_FEDERATION_DB = "dspro-application-query";
    public static String XPATH_ON_FEDERATION_DB = "dspro-application-xpath-query";
    public static String SQL_ON_APPLICATION = "dspro-application-query";
    public static String XPATH_ON_APPLICATION = "dspro-application-xpath-query";
    public static String XPATH_ON_DSPRO = "xpath-dspro-query";
    public static String SQL_ON_DSPRO_METADATA = "sql-on-metadata-dspro-query";
    public static String PROP_LIST_ON_DSPRO_APPLICATION_METADATA = "prop-list-on-application-metadata-dspro-query";
    public static String SQL_ON_DISSERVICE_METADATA = "disservice-query";
}
