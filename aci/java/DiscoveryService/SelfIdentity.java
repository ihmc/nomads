package DiscoveryService;

import mil.army.cpi.ba4.discoveryLib.AbstractIdentity;


public class SelfIdentity extends AbstractIdentity{
     int systemType;
     int platformType;
     int secClass;
     int ADCONURN;
     int OPCONURN;
     int ORGANICURN;
     char agency;
     int nationality;
     String symbolCode;

    public int getSystemType() {
        systemType = (int)(getSystemTypeNative() & 0xFF);
        return systemType;
    }

    private native byte getSystemTypeNative();

    public int getPlatformType() {
        platformType = (int)(getPlatformTypeNative() & 0xFF);
        return platformType;
    }

    private native byte getPlatformTypeNative();

    public int getSecClass() {
        secClass = (int)(getSecClassNative() & 0xFF);
        return secClass;
    }

    private native byte getSecClassNative();

    public int getADCONURN() {
        ADCONURN = getADCONURNNative();
        return ADCONURN;
    }

    private native int getADCONURNNative();

    public int getOPCONURN() {
        OPCONURN = getOPCONURNNative();
        return OPCONURN;
    }

    private native int getOPCONURNNative();

    public int getORGANICURN() {
        ORGANICURN = getORGANICURNNative();
        return ORGANICURN;
    }

    private native int getORGANICURNNative();

    public char getAgency() {
        agency = (char)getAgencyNative();
        return agency;
    }

    private native byte getAgencyNative();

    public int getNationality() {
        nationality = getNationalityNative();
        return nationality;
    }

    private native byte getNationalityNative();

    public String getSymbolCode() {
        symbolCode = getSymbolCodeNative();
        return symbolCode;
    }

    private native String getSymbolCodeNative();
    
    public void setSystemType(int systemType) {
        this.systemType = systemType;
    }

    public void setPlatformType(int platformType) {
        this.platformType = platformType;
    }

    public void setSecClass(int secClass) {
        this.secClass = secClass;
    }

    public void setADCONURN(int ADCONURN) {
        this.ADCONURN = ADCONURN;
    }

    public void setOPCONURN(int OPCONURN) {
        this.OPCONURN = OPCONURN;
    }

    public void setORGANICURN(int ORGANICURN) {
        this.ORGANICURN = ORGANICURN;
    }

    public void setAgency(char agency) {
        this.agency = agency;
    }

    public void setNationality(byte nationality) {
        this.nationality = nationality;
    }

    public void setSymbolCode(String symbolCode) {
        this.symbolCode = symbolCode;
    }

    @Override
    public String toString(){
        String text = "SystemType: " + getSystemType();
        text += "\nPlatformType: " + getPlatformType();
        text += "\nSecurityClass: " + getSecClass();
        text += "\nADCONURN: " + getADCONURN();
        text += "\nOPCONURN: " + getOPCONURN();
        text += "\nORGANICURN: " + getORGANICURN();
        text += "\ntest";
        text += "\nAgency: " + Character.toString((char)getAgency());
        text += "\nNationality: " + getNationality();
        text += "\nSymbolCode: " + getSymbolCode();

        return text;
    }

    private long _selfIdentity;
}
