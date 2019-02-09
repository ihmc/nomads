
/**
 * Represents a point in 3D space, with an age. The age increases
 * over time. This class is meant to hold sonar readings, and other
 * sensor readings.
 */
public class PointWithAge
{
    /**
     * Create a new PointWithAge, setting the position
     * to (0,0,0), and the age to 0.
     */
    public PointWithAge()
    {
        x = y = z = 0;
        startTime = System.currentTimeMillis();
    }

    /**
     * Create a new PointWithAge, with given position,
     * and age of 0.
     * @param newX The X.
     * @param newY The Y.
     * @param newZ The Z.
     */
    public PointWithAge(double newX, double newY, double newZ)
    {
        x = newX;
        y = newY;
        z = newZ;
        startTime = System.currentTimeMillis();
    }

    /**
     * Create a new PointWithAge, with given position and age.
     * @param newX The X.
     * @param newY The Y.
     * @param newZ The Z.
     * @param age The age.
     */
    public PointWithAge(double newX, double newY, double newZ, long age)
    {
        x = newX;
        y = newY;
        z = newZ;
        startTime = System.currentTimeMillis() - age;
    }

    /**
     * Get the age of this position.
     * @return The age.
     */
    public long getAge()
    {
        return System.currentTimeMillis() - startTime;
    }

    /**
     * Get the X value of this position.
     * @return The X value.
     */
    public double getX()
    {
        return x;
    }

    /**
     * Get the Y value of this position.
     * @return The Y value.
     */
    public double getY()
    {
        return y;
    }

    /**
     * Get the Z value of this position.
     * @return The Z value.
     */
    public double getZ()
    {
        return z;
    }

    /**
     * Set the X value.
     * @param newX The new X value.
     */
    public void setX(double newX)
    {
        x = newX;
    }

    /**
     * Set the Y value.
     * @param newY The new Y value.
     */
    public void setY(double newY)
    {
        y = newY;
    }

    /**
     * Set the Z value.
     * @param newZ The new Z value.
     */
    public void setZ(double newZ)
    {
        z = newZ;
    }

    /**
     * Resets the age to 0 at the time this method is called.
     */
    public void resetAge()
    {
        startTime = System.currentTimeMillis();
    }

    /**
     * Return a string with the x position, y position, z position, and age
     * separated by commas.
     * @return This position and age as a string.
     */
    public String toString()
    {
        return Double.toString(getX()) + ", " + Double.toString(getY()) + ", " +
                               Double.toString(getZ()) + ", " +
                               Long.toString(getAge());
    }

    private long startTime;
    private double x, y, z;
}


