package us.ihmc.aci.netSupervisor.inferenceModule;

import us.ihmc.aci.ddam.NetworkEvent;

import java.util.ArrayList;
import java.util.List;

public class ClaspEvent {

    public ClaspEvent ()
    {
        _eventName = "default";
        _eventValue = "default";
        _duration = Long.MAX_VALUE;
        _eventFilters = new ArrayList<>();
    }

    public ClaspEvent (NetworkEvent netEvent)
    {
        _eventName = "default";
        _eventValue = "default";
        _duration = Long.MAX_VALUE;
        setClaspEventByNetworkEvent(netEvent);
    }

    public long getDuration ()
    {
        return _duration;
    }

    public void setDuration (long duration)
    {
        _duration = duration;
        System.out.println("Event duration: " + _duration);
    }

    public String getEventName() {
        return _eventName;
    }

    public void setEventName(String eventName) {
        _eventName = eventName;
    }

    public String getEventValue() {
        return _eventValue;
    }

    public void setEventValue(String eventValue) {
        if (eventValue != null && !eventValue.equals("")) {
            _eventValue = eventValue;
        }
        else {
            _eventValue = "default";
        }
    }

    public boolean copyFrom (ClaspEvent eventSource)
    {
        _eventName = eventSource.getEventName();
        _eventValue = eventSource.getEventValue();
        _duration = eventSource.getDuration();

        return true;
    }

    public boolean setClaspEventByNetworkEvent (NetworkEvent netEvent)
    {

        if ((netEvent.getDescription()).equals("newLinkType")) {
            _eventName = "select" + netEvent.getValue() + "LinkType";
            _eventValue = "default";

        }

        if ((netEvent.getDescription()).equals("CommonEvent")) {
            _eventName = netEvent.getTarget();
            _eventValue = netEvent.getValue();
        }

        if ((netEvent.getDescription()).equals("decrease") || (netEvent.getDescription()).equals("change") ||
                netEvent.getDescription().equals("increase") ) {
            _eventName = netEvent.getDescription() + stringToTitleCase(netEvent.getTarget()) + "Priority";
            _eventValue = netEvent.getValue();
        }

        if (_eventValue.equals("")) {
            _eventValue = "default";
        }

        _duration = netEvent.getDuration().getSeconds();

        return true;
    }

    private String stringToTitleCase (String inputString)
    {
        String outputString = "";

        if (inputString != null) {
            if (inputString.length() > 0) {
                outputString = inputString.substring(0, 1).toUpperCase() + inputString.substring(1);
            }
        }

        return outputString;
    }
    private String _eventName;
    private String _eventValue;
    private long _duration;
    private List<String> _eventFilters;

}
