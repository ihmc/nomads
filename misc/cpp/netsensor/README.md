# Netsensor

Netsensor is a network monitoring library written in C++.


## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.
See deployment for notes on how to deploy the project on a live system.

### Prerequisites

to compile the project it is necessary:
- Protobuf 
- Libpcap
Windows
- VS2015
- Winpcap: https://www.winpcap.org/install/default.htm (tested with 4.1.3 on windows 10)
Linux

OWRT

### Installing

Compiling 

Executing 

## Configurations
NetSensor can be launched with different options:

        -h                      - Shows possible flags
        -cp                     - Compress reports
        -et                     - Produce external topology reports
        -trtt                   - Enable TCP RTT detection
        -i [interfaceName1 interfaceName2 ...]  - Specify interfaces to listen to (example - eth0 eth1, .. )
        -conf [configPath]      - Specify a path for the config file [UNTESTED]
        -nr [ipAddress]         - Specify a recipient for NetSensor reports
        -rm [configPath]        - Set REPLAY MODE and specify a path for the REPLAY MODE config file [UNTESTED]
        -period [msNumber]      - Set delivery period in ms
        -faddr [ipAddress]      - Force sensor Addr, this is used to override the sensor information in the reports
        -fnmsk [ipNetmask]      - Force sensor netmask, this is used to override the sensor information in the reports
        -fi [IName1:IAddr1:INetMsk1 IName2:IAddr2:INetMsk2 ...] - Force interface addr and netmasks, , this is used to override the interface information in the reports



## Running the tests

Explain how to run the automated tests for this system

### Break down into end to end tests

Explain what these tests test and why

```
Give an example
```

## Built With

## Contributing

## Versioning

## Authors

Mattia Mantovani (mmantovani@ihmc.us)
Robberto Fronteddu (rfronteddu@ihmc.us)

## License

This project is licensed under...

## Acknowledgments

* Hat tip to anyone who's code was used
* Inspiration
* etc