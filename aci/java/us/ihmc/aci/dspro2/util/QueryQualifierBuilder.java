package us.ihmc.aci.dspro2.util;

import java.util.Collection;
import java.util.LinkedList;
import us.ihmc.aci.util.dspro.MetadataElement;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class QueryQualifierBuilder
{
    private Collection<MetadataElement> _groupBy = new LinkedList<MetadataElement>();
    private Collection<MetadataElement> _orderBy = new LinkedList<MetadataElement>();
    private int _limit = 1;

    public QueryQualifierBuilder()
    {
    }

    public QueryQualifierBuilder setLimit (int limit)
    {
        _limit = limit;
        return this;
    }

    public QueryQualifierBuilder setGroupBy (Collection<MetadataElement> groupBy)
    {
        _groupBy = groupBy;
        return this;
    }

    public QueryQualifierBuilder setOrdering (Collection<MetadataElement> orderBy)
    {
        _orderBy = orderBy;
        return this;
    }

    public String getQualifier ()
    {   
        if (_groupBy.isEmpty()) {
            return null;
        }

        StringBuilder sb = new StringBuilder ("GROUP BY =");
        boolean bIsFirst = true;
        for (MetadataElement el : _groupBy) {
            if (bIsFirst) {
                bIsFirst = false;
            }
            else {
                sb.append (',');
            }
            sb.append (el);
        }
        sb.append ('\n');

        if (!_orderBy.isEmpty()) {
            sb.append ("ORDER BY =");
            bIsFirst = true;
            for (MetadataElement el : _groupBy) {
                if (bIsFirst) {
                    bIsFirst = false;
                }
                else {
                    sb.append (',');
                }
                sb.append (el);
            }
            sb.append ('\n');
        }

        sb.append ("LIMIT =").append(_limit);

        return sb.toString();
    }
}
