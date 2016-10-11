/*
 * ConfigFileHandler.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Created on March 19, 2003, 3:03 PM
 */

package us.ihmc.util;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Date;
import java.util.Enumeration;
import java.util.Properties;

/**
 * This class reads a configuration file and contructs a Properties object
 * All commments in the config file are kept, even if the 
 * config file is saved (using the provided save() method implementation) 
 * after modifying the values for any key.
 * New values can be added to this object. These new values are saved to the 
 * end of the file (w/o no comments)
 * The comment char is '#'. An extra '!' can be used after the first comment char 
 * to replace the line with the current date and time when the file is saved.
 * For instance, '#! here date and time' at the first line will be replaced by 
 * #! Wed Mar 19 18:42:00 CST 2003
 * 
 *
 * @author Adrian Granados  < agranados@ai.uwf.edu >
 */
public class ConfigFileHandler extends Properties {        
    
    private static final long serialVersionUID = 8767433280818171976L;

    public ConfigFileHandler()
    {
        super();
    }
    
    /** 
     * Creates a new instance of ConfigFileHandler with defaults values
     *
     * @param defaults the defaults properties
     */
    public ConfigFileHandler (Properties defaults)
    {
        super();
        putAll(defaults);
    }
    
    /** 
     * Creates a new instance of ConfigFile and load a configuration file 
     * If the file does not exist, it creates a new one config file
     * 
     * @param file - The filename of the configuration file
     */
    public ConfigFileHandler (String file) 
    {    
        super();        
        load(file);        
    }
    
    /**
     * Load a file
     * @param filename the file
     */
    public synchronized void load(String file)        
    {
        _file = file;
        File f = new File(_file);
        if (f.exists()){
            try {
                load(new FileInputStream(f));                
            }
                catch (IOException io){
                System.out.println("Error reading the file: " + _file);
            }
        }
        else {
            System.out.println("File not found: " + _file);
            try {
                f.createNewFile();
            }
            catch (IOException io){
                System.out.println("Error creating config file: " + _file);
            }
        }
    }
    
    public synchronized void save()
        throws IOException
    {
        save(_file);
    }
    
    /**
     * Writes this configuration object (key and value pairs) to the file from it was loaded.
     * All comments in the file are kept and the new pairs are saved to the end
     * of the file. 
     * If the file has a #!, the current date is added to the file, for instance
     * #! Wed Mar 19 18:42:00 CST 2003
     *
     * @throw IOException if an error occurs saving the file
     */
    @SuppressWarnings("rawtypes")
    public synchronized void save(String file)
        throws IOException
    {                
        if (file == null){
            return;
        }
        
        Properties properties = (Properties) this.clone();
        String line, key, value;  
        StringBuffer sbuffer = new StringBuffer();   
        String newline = System.getProperty("line.separator");
        
        File f = new File(file);
        if (f.exists()){        

            BufferedReader br = new BufferedReader(new FileReader(file));       
                       
            StringBuffer fixedLine = new StringBuffer();
            while ((line = br.readLine()) != null){
                line = line.trim();
                if (line.length() > 0){
                    if (line.startsWith("#!")){
                        sbuffer.append("#! ");
                        sbuffer.append((new Date(System.currentTimeMillis())).toString());
                        sbuffer.append(newline);
                    }
                    else if (line.startsWith("#") || line.startsWith("!")){
                        sbuffer.append(line + newline);
                    }
                    else if (line.endsWith("\\")){
                        fixedLine.append((line.substring(0, line.length())).trim());
                    }
                    else {
                        fixedLine.append(line);
                        key = readKey(fixedLine.toString());                        
                        if ((value = (String) properties.remove(key)) == null){
                            value = readValue(fixedLine.toString());
                        }
                        writeProperty(key, value, sbuffer);
                        sbuffer.append(newline + newline);
                        fixedLine.setLength(0);
                    }
                }
            }

            br.close();
        }        
        else {
            f.createNewFile();
        }
        
        for (Enumeration e = properties.keys(); e.hasMoreElements();){
            key = (String) e.nextElement();
            value = properties.getProperty(key);
            writeProperty(key, value, sbuffer);
            sbuffer.append(newline + newline);
        }    

        BufferedWriter bw = new BufferedWriter(new FileWriter(file));
        bw.write(sbuffer.toString());
        bw.close();              
    }
    
    /**
     * Parse a key from a string line
     *
     * @param line the String
     * @return the key 
     */
    private String readKey(String line)
    {        
        
        StringBuffer sbuffer = new StringBuffer();
        int i = 0;                
        while (i < line.length() && line.charAt(i) != ':' && line.charAt(i) != '='){
            if (line.charAt(i) == '\\'){
                sbuffer.append(line.charAt(i));
                i++;
                if (i < line.length()){
                    sbuffer.append(line.charAt(i));
                }                
            }
            else { 
                sbuffer.append(line.charAt(i));                
            }
            i++;
        }                
                        
        return unEscapeLine(sbuffer.toString().trim());                
          
        /*
        int indexOfEqual = line.indexOf('=');
        int indexOfColon = line.indexOf(':');
        int i = indexOfEqual > indexOfColon ? indexOfEqual : indexOfColon;
        i = i > 0 ? i : 0;
        return unEscapeLine(line.substring(0, i).trim());
        */
    }
    
    /**
     * Parse a value from String line
     *
     * @param line the String
     * @return the value
     */
    private String readValue(String line)
    {        
        /*
        StringBuffer sbuffer = new StringBuffer();        
        int i = 0;
        while (i < line.length() && line.charAt(i) != ':' && line.charAt(i) != '='){
            if (line.charAt(i) == '\\'){
                i++;
            }
            i++;
        }
        */        
        
        int indexOfEqual = line.indexOf('=');
        int indexOfColon = line.indexOf(':');
        int i = indexOfEqual > indexOfColon ? indexOfEqual : indexOfColon;
        i = i > 0 ? i : -1;
        return unEscapeLine(line.substring(i+1).trim());
    }        
    
    /**
     * Write a property to a StringBuffer
     * The property is written: key + "=" + value
     * The string key and value are escaped.
     */
    private void writeProperty(String key, String value, StringBuffer sbuffer)
    {                
        sbuffer.append(escapeLine(key) + "=" + escapeLine(value));        
    }
    
    /**
     * Unescape a line 
     *
     * @param line the String
     * @return the unescaped string
     */
    private String unEscapeLine(String line)
    {    
        StringBuffer sbuffer = new StringBuffer();
        for (int i = 0; i < line.length(); i++){
            if (line.charAt(i) == '\\'){
                i++;
                if (i < line.length()){
                    switch (line.charAt(i)){
                        case 't':
                            sbuffer.append("\t");
                            break;
                        case 'n':
                            sbuffer.append("\n");
                            break;
                        case 'r':
                            sbuffer.append("\r");
                            break;
                        case '\\':
                            sbuffer.append("\\");
                            break;
                        case ' ':
                        case '"':
                        case '\'':
                        case ':':
                        case '=':
                            sbuffer.append(line.charAt(i));
                            break;
                        default:
                            sbuffer.append('\\');
                            sbuffer.append(line.charAt(i));                    
                    }
                }
            }
            else {
                sbuffer.append(line.charAt(i));
            }
        }
        
        return sbuffer.toString().trim();
    }
    
    /**
     * Escape a string line
     *
     * @param line the String
     * @return the escaped string
     */
    private String escapeLine(String line)
    {
        StringBuffer sbuffer = new StringBuffer();
        for (int i = 0; i < line.length(); i++){
            switch (line.charAt(i)){
                case '\\':
                    sbuffer.append("\\" + "\\");    
                    break;
                case '\t':
                    sbuffer.append("\\" + "t");
                    break;
                case '\n':
                    sbuffer.append("\\" + "n");
                    break;    
                case '\r':
                    sbuffer.append("\\" + "r");                        
                    break;
                case '#':
                case '!':
                case '=':
                case ':':
                    sbuffer.append('\\');
                    sbuffer.append(line.charAt(i));
                    break;            
                default:
                    sbuffer.append(line.charAt(i));
            }
        }
        
        return sbuffer.toString().trim();
    }
    
    private String _file = null;    
    
    @SuppressWarnings("rawtypes")
    public static void main(String args[])
    {            
        if (args[0].equalsIgnoreCase("c")){
            ConfigFileHandler cf = new ConfigFileHandler("test.conf");

            for (Enumeration e = cf.keys(); e.hasMoreElements();){
                String key = e.nextElement().toString();
                System.out.println(key + "=" + cf.getProperty(key));
            }

            try {
                cf.save("test.conf");
            }
            catch (Exception xcp){
                xcp.printStackTrace();
            }
        }
        else {
            try {
                Properties p = new Properties();
                p.load(new FileInputStream("test.conf"));

                Enumeration e = p.keys();
                while (e.hasMoreElements()){
                    String key = e.nextElement().toString();
                    System.out.println(key + "=" + p.getProperty(key));
                }

                p.store(new java.io.FileOutputStream("test.conf"), "");
            }
            catch (Exception xcp){
                xcp.printStackTrace();
            }
        }
    }
     
}
