package us.ihmc.aci.dspro2.util;

/**
 * Created by gbeni on 4/4/2018.
 */
public enum KnownClientId {

    DSProShell ((short) 0x00),
    DSProGUI ((short) 0x01),
    Atak  ((short) 0x02),
    SoiBridge ((short) 0x03),
    InfoManager ((short) 0x04),
    Mist ((short) 0x05),
    VirtualSensor ((short) 0x06),
    Manatim ((short) 0x07),
    NetCacher ((short) 0x08),
    Reset((short) 0x09),
    CtrlSoiBridge ((short) 0x10),

    KilSwitch ((short) 0x19);

    final short _clientId;

    KnownClientId(short clientId) {
        _clientId = clientId;
    }

    public short toShort() {
        return _clientId;
    }
}
