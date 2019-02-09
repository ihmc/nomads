package us.ihmc.aci.util.dspro;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public enum XMLMetadataElement {

    Application_Metadata,
    Application_Metadata_Format,
    Application_ID,
    Message_ID,
    
    Refers_To,
    
    Referred_Data_Object_Id,
    Referred_Data_Instance_Id,

    External_Referred_Cached_Data_URL,

    ComputedVOI,

    Prev_Msg_ID,
    Node_Type,
    Data_Content,
    Classification,
    Data_Format,
    Left_Upper_Latitude,
    Right_Lower_Latitude,
    Left_Upper_Longitude,
    Right_Lower_Longitude,
    Description,
    Pedigree,
    Importance,
    Location,
    Receiver_Time_Stamp,
    Source,
    Source_Reliability,
    Source_Time_Stamp,
    Expiration_Time,
    Relevant_Missions,
    Target_ID,
    Target_Role,
    Target_Team,
    Track_ID,
    Track_Action,
    ABE_Equation, 
    ABE_IV, 
    ABE_Encrypted_Fields;
}

