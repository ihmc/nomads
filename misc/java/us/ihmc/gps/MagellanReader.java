package us.ihmc.gps;

/**
 * MagellanReader.java
 * <p> 
 * Description: The MagellanReader class encapsulates serial comm input in 
 * NMEA0183-Format from a Magellan GPS device (Sports Trak models: Map). 
 * </p>
 * Created on Aug 31, 2004
 * @author Christopher J. Eagle <ceagle@ihmc.us>
 * @version $Revision$ 
 * $Date$
 * <p>Copyright (c) Institute for Human and Machine Cognition (www.ihmc.us)</p>
 */

import us.ihmc.gps.convert.GPSConverter;
import us.ihmc.gps.position.Coordinate;
import us.ihmc.gps.position.Degrees;
import us.ihmc.gps.position.GPSPose;
import us.ihmc.gps.position.GPSPosition;


public class MagellanReader extends Thread
{
    public MagellanReader() throws Exception
    {
        this ( _defaultCommPort );
    }
    
    public MagellanReader(int commPort) throws Exception
	{
		try{
			_gpsHelper = new MagellanHelper (commPort);
			 if(_gpsHelper == null){
				 System.out.println("*** ERROR starting GPS ***");
			 }
		}
		catch(Exception e){
			e.printStackTrace();
			throw e;
		}
    }
    
    public double getAltitude()
    {
        return _altitude;
    }
    
    public GPSPosition getDatum()
    {
		return GPSConverter.getDatum();
    }

    public GPSPosition getGPSPosition()
    {
        return _gpsPosition;
    }
	    
    public GPSPose getGPSPose()
    {
        return _gpsPose;
    }

	public double getCourse()
    {
        return _course;
    }

	public int getReadInterval()
    {
        return _readInterval;
    }

	public Coordinate getXYPosition()
    {
		Coordinate coord = GPSConverter.PosConverter(_gpsPosition.getLatitude(),
           _gpsPosition.getLongitude());

		return coord; 
    }

    public double getX()
    {
		if(_currentCoordinate != null){
			return _currentCoordinate.getX();
		}
		return 0;
    }

	public double getY()
    {
		if(_currentCoordinate != null){
			return _currentCoordinate.getY();
		}
		return 0;
    }

    public void setReadInterval(int interval)
    {
        _readInterval = interval;
    }

    public void run() 
    {
        String sLatitude = null;
        String sLongitude = null;
		String sCourse = null;
        String height = null;
        int commaIndex;
        int counter = 0;

        try {       
            while (_running) 
            {
                counter++;
                
                if (counter >= 8){
                    try{
                        Thread.sleep(1);// Give up the processor.
                    }
                    catch(Exception e){
                        e.printStackTrace();
                    }
                    counter = 0;
                }

                String line = _gpsHelper._bufferedReader.readLine();
                
                //check if GCA
                if((line.startsWith("$GPGGA"))){
                    height = _gpsHelper.getAltitude(line);
                    if (height != null) {                
                        _altitude = new Double(height).doubleValue();
                        continue;
                    }
                }                
				
				//check if RMC
				if(!(line.startsWith("$GPRMC"))){
					continue;
				}
				
				//find the comma
                commaIndex = line.indexOf(',');
                if (commaIndex == -1) {
                    continue;
                }
									
				//use helper to parse the data appropriately
                line = line.substring(commaIndex + 1, line.length());
                String field = _gpsHelper.findGpsField(line, MagellanHelper.RMC);

                if (field == null) {
                    continue;
                }

                sLatitude = _gpsHelper.getLatitude(field);  //must be in this order!
                sLongitude = _gpsHelper.getLongitude(field);
                if (sLatitude == null || sLongitude == null) {
                    continue;
                }
                					
				sCourse = _gpsHelper.getCourse(line);                
				double course = new Double(sCourse).doubleValue();														

				//receive Degrees from helper and set Lat/Long to degrees
                _latitude = (Degrees) _gpsHelper.parseGpsField(sLatitude);					
                _longitude = (Degrees) _gpsHelper.parseGpsField(sLongitude);					
				_gpsPosition = new GPSPosition(_latitude, _longitude);					

				//check for course stability
				if (course != 0.0){
					double diff2 = Math.pow((course - _lastcourse), 2);
					double diff = Math.sqrt(diff2);
					System.out.println();
					if(diff < 2){
						_validCounter++;
					}
					else{
						_validCounter = 0;
					}
					_lastcourse = course;
				}
				else{
					_lastcourse = 0.0;
					_validCounter = 0;
				}
				
				if(_validCounter > 2){
					_course = course;
					_validCounter = 0;
				}
				else{
					_course = 0.0;
				}
				
				_gpsPose = new GPSPose(_latitude, _longitude);
				_gpsPose.setCourse(_course);
                
            } //end of while loop
        }
        catch (Exception e) {
            System.out.println("Exception on GPSReader outer try{}");
            e.printStackTrace();
        }
    }


    public void stopRunning() 
    {
        _running = false;
    }     
    
    
    //test driver 
	public static void main(String[] args)
	{		
		//get commPort
		int commPort = 1;
		for(int i = 0; i < args.length; i++){
			if(args[i].equalsIgnoreCase("-gpsCommPort")){
				commPort = new Integer(args[i+1]).intValue();
			}			
		}

		//set read interval
		int readInterval = 500;
		System.out.println("Starting GPS reader\n\ton comm port " + commPort 
            + "\n\twith a " + readInterval + "ms read interval");
                           	
        MagellanReader gps = null;
		try{
			gps = new MagellanReader (commPort);
            gps.setReadInterval(readInterval);
		}
		catch(Exception e){
			e.printStackTrace();
			System.exit(0);
		}
		
		gps.start();
        
		while(true){
			System.out.println("(lat/long) " + gps.getGPSPosition() + 
			    "\t\tcourse = " + gps.getCourse());
			
			//sleep
			try{
				Thread.sleep(gps.getReadInterval());
			}
			catch(Exception e) {}		
		}		
	}


    //Class Variables
    private Coordinate _currentCoordinate = null;
    private Degrees _latitude;
    private Degrees _longitude;
    private double _altitude = 0.0;  // in meters.
    private double _course = 0.0;
    private double _lastcourse = 0.0;
    private int _validCounter = 0;
	private GPSPosition _gpsPosition;
	private GPSPose _gpsPose;
    private MagellanHelper _gpsHelper;
    private int _readInterval = 500;
    private boolean _running = true;
    private boolean _initStatus = false;

    private static int _defaultCommPort = 1;
}
