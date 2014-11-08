/**
 * The SequentialArithmetic class provides routines to handle sequential
 * arithmetics for transmission sequence numbers (TSN).
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

class SequentialArithmetic
{
    private SequentialArithmetic () {
        // no instances for this class
    }

    private static long normalize (long x) {
        assert -MaximumTSN < x && x < MaximumTSN;
        return (x < 0 ? x + MaximumTSN : x); 
    }

    /* modular addition of a and b */
    static long add (long a, long b) {
        return normalize ((a + b) % MaximumTSN);
    }
    
    /* modular subtraction of b from a */
    static long subtract (long a, long b) {
        return normalize ((a - b) % MaximumTSN);
    }
    
    /* modular comparison between a and b
     * this method returns true if a is greater than b */
    static boolean greaterThan (long a, long b) {
        return (a > b ? ((a - b) <= MaximumTSN / 2) : ((b - a) > MaximumTSN / 2));
    }
    
    /* modular comparison between a and b
     * this method returns true if a is less than b */
    static boolean lessThan (long a, long b) {
        return greaterThan (b, a);
    }
    
    /* modular comparison between a and b
     * this method returns true if a is greater than or equal to b */
    static boolean greaterThanOrEqual (long a, long b) {
        return (a == b) || greaterThan (a, b);
    }
    
    /* modular comparison between a and b
     * this method returns true if a is less than or equal to b */
    static boolean lessThanOrEqual (long a, long b) {
        return (a == b) || lessThan (a, b);
    }

    static long max (long a, long b) {
        return greaterThan (a, b) ? a : b;
    }
    
    /* maximum TSN is 2^32 */
    static long MaximumTSN = 4294967296L;
}
/*
 * vim: et ts=4 sw=4
 */

