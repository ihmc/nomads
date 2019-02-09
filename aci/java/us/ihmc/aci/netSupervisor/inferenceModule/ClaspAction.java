package us.ihmc.aci.netSupervisor.inferenceModule;

import static us.ihmc.aci.netSupervisor.inferenceModule.ClaspAction.ActionType.*;
import static us.ihmc.aci.netSupervisor.inferenceModule.ClaspAction.DirectionType.*;

public class ClaspAction
{

    public enum ActionType
    {
        UNKNOWN_TYPE,
        INCREASE,
        DECREASE,
        CHANGE_FIXED,
        CHANGE_PARAMETRIC
    }

    public enum DirectionType
    {
        NONE,
        INCOMING,
        OUTGOING,
        BIDIRECTIONAL
    }

    public ClaspAction()
    {

    }

    public String getActionName()
    {
        return _actionName;
    }

    public void setActionName(String actionName)
    {
        _actionName = actionName;
    }

    public int getTypeOfAction()
    {
        return _typeOfAction;
    }

    public void setTypeOfAction(int typeOfAction)
    {
        _typeOfAction = typeOfAction;
    }

    public int getGroupId()
    {
        return _groupId;
    }

    public int getDirection()
    {
        return _direction;
    }

    public int getPriorityValue()
    {
        return _priorityValue;
    }

    public void setGroupId(int groupId)
    {
        _groupId = groupId;
    }

    public void setDirection(int orientation)
    {
        _direction = orientation;
    }

    public void setPriorityValue(int priorityValue)
    {
        _priorityValue = priorityValue;
    }

    public long getDuration()
    {
        return _duration;
    }

    public void setDuration(long duration)
    {
        _duration = duration;
    }

    public void setDefaultForType (int typeOfAction, String actionDescription, int groupId)
    {
        _direction = 3;
        _groupId = groupId;
        _typeOfAction = typeOfAction;
        switch (typeOfAction) {
            case 1:
                _priorityValue = 1;
                _actionName = "increase" + stringToTitleCase(actionDescription) + "Priority";
                break;
            case 2:
                _priorityValue = 1;
                _actionName = "decrease" + stringToTitleCase(actionDescription) + "Priority";
                break;
            case 3:
                _priorityValue = 0;
                _actionName = "change" + stringToTitleCase(actionDescription) + "Priority";
                break;
            case 4:
                _priorityValue = 0;
                _actionName = "changeParametric" + stringToTitleCase(actionDescription) + "Priority";
                break;
        }
    }

    public ActionType getTypeOfActionEnum()
    {
        switch (_typeOfAction) {
            case 1:
                return INCREASE;
            case 2:
                return DECREASE;
            case 3:
                return CHANGE_FIXED;
            case 4:
                return CHANGE_PARAMETRIC;
        }
        return UNKNOWN_TYPE;
    }

    public DirectionType getDirectionEnum()
    {
        switch (_direction) {
            case 1:
                return INCOMING;
            case 2:
                return OUTGOING;
            case 3:
                return BIDIRECTIONAL;
        }
        return NONE;
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

    private int _groupId;
    private String _actionName;
    private int _typeOfAction;
    private int _direction;
    private int _priorityValue;
    private long _duration;
}
