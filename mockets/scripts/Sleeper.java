public class Sleeper
{

    public static void main (String args[])
    {
	if (args.length < 1) {
	    System.out.println("Usage: java Sleeper <milliseconds>");
	    System.exit(1);
	}
	try
	{
	    Thread.sleep(Long.parseLong(args[0]));
	}
	catch (InterruptedException e)
	{System.out.println("Error: "+e.toString());}
    }
}
