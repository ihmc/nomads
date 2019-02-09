package us.ihmc.aci.nodemon.data.process;

import com.google.gson.annotations.SerializedName;
import com.google.protobuf.GeneratedMessage;
import org.apache.commons.lang3.builder.EqualsBuilder;
import org.apache.commons.lang3.builder.HashCodeBuilder;

/**
 * PROCESS.java
 * <p>
 * Class <code>PROCESS</code>
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Process {

    public enum ProcessContentType {

        DISSERVICE,
        MOCKETS,
        NETPROXY,
        SNMP
    }

    public Process() {
        id = "";
        type = null;
        content = null;

    }

    public Process(String id, ProcessContentType type, ProcessContent content) {
        this.id = id;
        this.type = type;
        this.content = content;
    }

    @Override
    public int hashCode() {
        return new HashCodeBuilder(17, 31). //two randomly chosen prime numbers
                append(id).
                append(type).
                append(content).
                toHashCode();
    }

    @Override
    public boolean equals(final Object o) {
        if (this == o) return true;

        if (o == null || getClass() != o.getClass()) return false;

        Process b = (Process) o;
        return new EqualsBuilder().
                append(id, b.id).
                append(type, b.type).
                append(content, b.content).
                isEquals();
    }

    /**
     * Returns a <code>String</code> as the id representation of this <code>PROCESS</code>.
     *
     * @return a <code>String</code> as the id representation of this <code>PROCESS</code>.
     */
    public String getId() {
        return id;
    }

    /**
     * Returns the <code>ProcessContentType</code> of this <code>PROCESS</code.
     *
     * @return the <code>ProcessContentType</code> of this <code>PROCESS</code.
     */
    public ProcessContentType getType() {
        return type;
    }

    /**
     * Returns the </code><code>PROCESS_CONTENT</code> of this <code>PROCESS</code>.
     *
     * @return the </code><code>PROCESS_CONTENT</code> of this <code>PROCESS</code>.
     */
    public ProcessContent getContent() {
        return content;
    }

    @SerializedName("id")
    private final String id;
    @SerializedName("type")
    private final ProcessContentType type;
    @SerializedName("content")
    private final ProcessContent content;

}
