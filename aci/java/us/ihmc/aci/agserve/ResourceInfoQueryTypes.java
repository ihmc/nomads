package us.ihmc.aci.agserve;

public class ResourceInfoQueryTypes
{
    public static int AvgBytecodes = 0x01;						//Average bytescode used by the specified service.
    public static int AvgParameterData = 0x02;					//Average bytes read by the specified service.
    public static int AvgReturnData = 0x03;						//Average bytes written by the specified service.
    public static int AvgInvocationRate = 0x04;					//Average bytescode rate of the specified service.
    public static int AvgCycleBytecodes = 0x05;					//Average bytescode used by the specified service in a life cycle.
    public static int AvgBytecodesSign = 0x33;					//Average bytescode used by the specified service running the method with the specified signature.
    public static int AvgParameterDataSign = 0x34;				//Average bytes read by the specified service running the method with the specified signature.
    public static int AvgReturnDataSign = 0x35;					//Average bytes written by the specified service running the method with the specified signature.
    public static int AvgInvocationRateSign = 0x36;				//Average bytescode rate of the specified service running the method with the specified signature.
    public static int AvgBytecodesClient = 0x65;				//Average bytescode used by the specified service and host.
    public static int AvgParameterDataClient = 0x66;			//Average bytes read by the specified service and host.
    public static int AvgReturnDataClient = 0x67;				//Average bytes written by the specified service and host.
    public static int AvgInvocationRateClient = 0x68;			//Average bytescode rate of the specified service and host 
    public static int AvgBytecodesClientSign = 0x97;			//Average bytescode used by the specified service and host running the method with the specified signature.
    public static int AvgParameterDataClientSign = 0x98;		//Average bytescode used by the specified service and host running the method with the specified signature.
    public static int AvgReturnDataClientSign = 0x99;			//Average bytescode used by the specified service and host running the method with the specified signature.
    public static int AvgInvocationRateClientSign = 0x9A;		//Average bytescode used by the specified service and host running the method with the specified signature.
}
