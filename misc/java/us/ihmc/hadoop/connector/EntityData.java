package us.ihmc.hadoop.connector;

import org.apache.accumulo.core.data.Mutation;
import org.apache.accumulo.core.data.Value;
import org.apache.accumulo.core.security.ColumnVisibility;
import org.apache.hadoop.io.Text;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.PatternSyntaxException;

/**
 * EntityData.java
 * <p/>
 * Class <code>EntityData</code> represents the data format read/written from/to the accumulo data store.
 */
public class EntityData
{
    public enum Family
    {
        name,
        appears_in,
        type,
        context,
        confidence
    }

    private String entityId;
    private String name;
    private String appearsIn;
    private List<EntityType> types;
    private String context;

    public EntityData ()
    {
        types = new ArrayList<EntityType>();
    }

    public EntityData (final String entityId, final String name, final String appearsIn, final String context)
    {
        this.entityId = entityId;
        this.name = name;
        this.appearsIn = appearsIn;
        types = new ArrayList<EntityType>();
        this.context = context;
    }

    public boolean isComplete ()
    {
        return (entityId != null &&
                name != null &&
                appearsIn != null &&
                context != null &&
                types.size() >= 1);
    }

    public List<String> toStringList ()
    {
        ArrayList<String> entityList = new ArrayList<String>();
        entityList.add(entityId);
        entityList.add(name);
        entityList.add(appearsIn);
        if (types.size() != 0) {   //TODO add also other types
            entityList.add(types.get(0).type);
            entityList.add(types.get(0).getNormalizedConfidence());
        }
        entityList.add(context);

        return entityList;
    }

    public static EntityData fromStringList (List<String> entityList) throws IndexOutOfBoundsException,
            NumberFormatException, PatternSyntaxException
    {
        EntityData e = new EntityData();
        e.setId(entityList.get(1));
        e.setName(entityList.get(2));
        e.setAppearsIn(entityList.get(3));
        e.getTypes().add(new EntityType(entityList.get(4), Double.valueOf(entityList.get(5).split("%")[0]) / 100)); //HACK,
        // we know that confidence is 1
        e.setContext(entityList.get(6));
        return e;
    }


    public Mutation toUpdateMutation ()
    {
        //Add only type and confidence to the Mutation for now
        if (getTypes().size() != 1) {
            System.out.println("ERROR: Found more than 1 type, undetermined entity, unable to mutate");
            return null;
        }

        Text type = new Text(getTypes().get(0).type);
        Value confidence = new Value(String.valueOf(getTypes().get(0).confidence).getBytes());
        long timestamp = System.currentTimeMillis();

        Mutation mutation = new Mutation(new Text(entityId));
        mutation.put(new Text(Family.type.toString()), type, new ColumnVisibility(), timestamp, confidence);

        return mutation;
    }

    public Mutation toDeleteMutation()
    {
        //Add only type and confidence to the Mutation for now
        if (getTypes().size() != 1) {
            System.out.println("ERROR: Found more than 1 type, undetermined entity, unable to mutate");
            return null;
        }

        Mutation mutation = new Mutation(new Text(entityId));
        mutation.putDelete(new Text(Family.name.toString()), new Text(name));
        mutation.putDelete(new Text(Family.appears_in.toString()), new Text(appearsIn));
        mutation.putDelete(new Text(Family.type.toString()), new Text((getTypes().get(0).type)));
        mutation.putDelete(new Text(Family.context.toString()), new Text(context));

        return mutation;
    }

    @Override
    public String toString ()
    {
        StringBuilder sb = new StringBuilder();
        sb.append("EntityId: ").append(entityId != null ? entityId : "null");
        sb.append(" Name: ").append(name != null ? name : "null");
        sb.append(" AppearsIn: ").append(appearsIn != null ? appearsIn : "null");
        int i = 1;
        for (EntityType type : types) {
            sb.append(" Type #").append(i).append(": ").append(type.type != null ? type.type : "null").append(" " +
                    "Confidence: ").append(type.getNormalizedConfidence()).append("\n");
            i++;
        }

        sb.append(" Context: ").append(context != null ? context : "null");

        return sb.toString();
    }

    public String getId ()
    {
        return entityId;
    }

    public void setId (String id)
    {
        this.entityId = id;
    }

    public String getName ()
    {
        return name;
    }

    public void setName (String name)
    {
        this.name = name;
    }

    public String getAppearsIn ()
    {
        return appearsIn;
    }

    public void setAppearsIn (String appearsIn)
    {
        this.appearsIn = appearsIn;
    }

    public List<EntityType> getTypes ()
    {
        return types;
    }

    public void setContext (String context)
    {
        this.context = context;
    }
}
