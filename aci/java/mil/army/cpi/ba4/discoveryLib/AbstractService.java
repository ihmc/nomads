package mil.army.cpi.ba4.discoveryLib;


public abstract class AbstractService{

    public String UUID;
    public String serviceName;
    public int serviceType;
    public int port;


    public AbstractService(){

    }
    
    public AbstractService(AbstractService service) {
        this.UUID = service.UUID;
        this.serviceName = service.serviceName;
        this.serviceType = service.serviceType;
        this.port = service.port;
    }

    public enum ServiceType {

        Data_Access_Services,
        Data_Service_Broker_Services,
        Data_Discovery_Services,
        Data_Translatio_Services,
        Data_Indexing_Services,
        Configuration_Services,
        Data_Security_Services,
        Metadata_Services,
        Data_Notification_Services,
        Data_Validation_Services,
        Data_Query_Services,
        Data_Storage_Services,
        External_Interopability_Services,
        File_Services,
        Data_Analytics_Services,
        Data_Enrichment_Services,
        Data_Synchronization_Services,
        Data_Ingest_Services,
        Network_Services,
        Identity_Services,
        Directory_Services,
        Authorization_Svcs,
        Mail_Services,
        Knowledge_Management_Portal_Services,
        Chat_Services,
        Monitoring_Services,
        Management_Services,
        Voice_Collaboration_Services,
        Video_Collaboration_Services,
        COTS_DB_Mgmt_Services,
        PKI_Certificate_Services,
        Web_Presentation_Framework_Services,
        Web_Application_Hosting_Services,
        Web_Service_Hosting_Services,
        Geospatial_Client_Services,
        Geospatial_Data_Storage_Services,
        Geospatial_Data_Management_Services,
        Simulation_Driver_Data_Services,
        Engineering_Software_Services,
        Airspace_Management_Software_Services,
        Aviation_Maneuver_Planning,
        Electronic_Warfare_Services,
        Army_Fires_C2_Services,
        Joint_Fires_Coodination_Services,
        Fires_Weather_Services,
        Fires_Sim_Services,
        Air_Defense_Services,
        Force_Protection_Services,
        CBRNE_Services,
        Tactical_Sustainment_Services,
        Enterprise_Sustainment_Services,
        GEOINT_Services,
        SIGINT_Services,
        HUMINT_Services,
        Weather_Services,
        MASINT_Services,
        All_Source_Analysis_Services,
        Information_Collection_Management_Services,
        Network_Managmement_Services,
        Network_Planning_Services,
        IT_Management_Services,
        Security_Data_Log_Mgmt_Services,
        Cyber_SA_Services,
        Cybersecurity_Monitoring_Services,
        Crypto_Key_Mgmt_Services,
        Host_Based_Security_Services,
        Defensive_Cyber_Operations_Services,
        Voice_services,
        Application_Interface_Services,
        Tactical_Email_Services,
        PLI_data_services,
        Collaboration_Services,
        Print_Share_Services,
        Language_Services,
        Information_Control_Services,
        Deployed_Tactical,
        Network_Security,
        Admin_Services,
        Information_Administration_Services,
        COOP_Services,
        Information_Content_Services,
        Information_Retrieval_Services,
        Mediation_Services,
        Service_Discovery,
        Content_Discovery,
        ISA_Controller_Service
    }
}
