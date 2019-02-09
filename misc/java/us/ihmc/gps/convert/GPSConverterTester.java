package us.ihmc.gps.convert;

import us.ihmc.gps.position.Coordinate;
import us.ihmc.gps.position.Degrees;
import us.ihmc.gps.position.GPSPosition;


public class GPSConverterTester 
{    
	//test driver //////////////////////////////////////////////////
	public static void main(String[] args)
	{			
		Degrees lat = new Degrees("N", 30, 24, 45.0);
		Degrees lon = new Degrees("W", 87, 12, 35.0);
		GPSPosition datum = new GPSPosition(lat,lon);
		
		//move west
		System.out.println("moving west");
		double seconds = 35.0;
		for(int i = 0; i < 20; i++){
			seconds += 0.01;			
			lat = new Degrees("N", 30, 24, 45.0);
			lon = new Degrees("W", 87, 12, seconds);
			GPSPosition location = new GPSPosition(lat,lon);
			Coordinate coord = GPSConverter.getXYFromGPS(datum, location);
			System.out.println("coord = " + coord.getX() + "\t" + coord.getY());	
		}
		
		//move east
		System.out.println("moving east");
		seconds = 35.0;
		for(int i = 0; i < 20; i++){
			seconds -= 0.01;			
			lat = new Degrees("N", 30, 24, 45.0);
			lon = new Degrees("W", 87, 12, seconds);
			GPSPosition location = new GPSPosition(lat,lon);
			Coordinate coord = GPSConverter.getXYFromGPS(datum, location);
			System.out.println("coord = " + coord.getX() + "\t" + coord.getY());	
		}
		
		//move north
		System.out.println("moving north");
		seconds = 45.0;
		for(int i = 0; i < 20; i++){
			seconds += 0.01;			
			lat = new Degrees("N", 30, 24, seconds);
			lon = new Degrees("W", 87, 12, 35.0);
			GPSPosition location = new GPSPosition(lat,lon);
			Coordinate coord = GPSConverter.getXYFromGPS(datum, location);
			System.out.println("coord = " + coord.getX() + "\t" + coord.getY());	
		}
		//move south
		System.out.println("moving south");
		seconds = 45.0;
		for(int i = 0; i < 20; i++){
			seconds -= 0.01;			
			lat = new Degrees("N", 30, 24, seconds);
			lon = new Degrees("W", 87, 12, 35.0);
			GPSPosition location = new GPSPosition(lat,lon);
			Coordinate coord = GPSConverter.getXYFromGPS(datum, location);
			System.out.println("coord = " + coord.getX() + "\t" + coord.getY());	
		}
	}
}
