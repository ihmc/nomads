package us.ihmc.media.imageDiff;

import java.awt.Dimension;
import java.util.Vector;
import javax.media.format.RGBFormat;
import javax.media.Buffer;

import us.ihmc.media.imageDiff.ImageCompareEngine;

public class LineEngine implements ImageCompareEngine
{
    public static int RED = 0;
    public static int GREEN = 1;
    public static int BLUE = 2;
   
    
    public Object computeChecksum (Object image, float granularity)
    {
        Buffer bufImg = (Buffer) image;
        RGBFormat format = (RGBFormat)bufImg.getFormat();
        Dimension size = format.getSize();
        byte[] imgData = (byte[])bufImg.getData(); 
        
        int height = size.height;
        int width = size.width;
        
        int[][] matrix = createColorMatrix (width, height, format.getLineStride(), imgData, _color);
        
        Vector vResult = new Vector();
        for (int y = 0; y < height; y++) {
            float widthAvg = 0;
            for (int x = 0; x < width; x++) {
                widthAvg = widthAvg + matrix[x][y];    
            }
            widthAvg = widthAvg / width;
            vResult.addElement (new Float (widthAvg));
        }
        
        return vResult;
        
    }
    
    public float compareChecksums (Object checkSumOne, Object checkSumTwo)
    {
        Vector one = (Vector)checkSumOne;
        Vector two = (Vector)checkSumTwo;
        float cmpValue = 0;
        for (int i = 0; i < one.size(); i++) {
            float f1 = ((Float)one.elementAt (i)).floatValue();  
            float f2 = ((Float)two.elementAt (i)).floatValue();
            float tmp = Math.abs (f1 - f2);
            if (tmp > cmpValue) {
                cmpValue = tmp;
            } 
        }
        
        return cmpValue;
    }
    
    public void setColor (int color)
    {
        if (color == RED || color == GREEN || color == BLUE) {
            _color = color;
        }
    }
    
    public int getColor()
    {
        return _color;
    }
    
    private int[][] createColorMatrix (int imgWidth, int imgHeight, int lineStride, byte[] imgData, int color) 
    {
        int[][] matrix = new int[imgWidth][imgHeight];
        for (int x = 0; x < imgWidth; x++) {
            for (int y = 0; y < imgHeight; y++) {
                matrix[x][y] = getColorValueForPixel (x, y, lineStride, imgData, color);    
            }
        }
        
        return matrix;
    }
    
    /**
     * Get the value of a color at a give x,y pixel position
     */
    private int getColorValueForPixel (int x, int y, int lineStride, byte[] imgData, int color) 
    {
        int pos = y * lineStride + (x * 3) + color;
        return imgData[pos];
    }
    
    public int _color = RED;
}