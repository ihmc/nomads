package us.ihmc.cue.imsbridgesubscriber.websocket;

public class Endpoint
{
    public Endpoint(String val){
        _endpoint = val;
    }

    @Override
    public String toString() {
        return "/" + _endpoint;
    }

    private String _endpoint;
}
