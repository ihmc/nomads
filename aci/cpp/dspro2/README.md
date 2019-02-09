# Proactive Dissemination Service (DSPro)

## Build

Either use the provided Visual Studio solution, or do `make` form the `linux` directory.

## Changelog

### Stable Branches

#### Master

Merged _dspro-nats-android_:
- Removed ifdef to remove nats-related code when buidling for android
- Modified android Makefile to link protobuf statically, and natswr dynamically

Merged ABE branch (_dspro-abe-proxy_):
- DSProProxy refactoring (based on DSProProxyBuilder),
- Added MetadataElements for ABE equation,

Merged JSON Metadata refactoring (_dspro-json-metadata_):
Changed DSPro API to return metadata as JSON instread of XML
- Changed APIs's parameter and method names (doGetMatchingMetaDataAsXML -> getMatchingMetadata)
- Changed DSProProxy's command: getMatchingMetaDataAsXML -> getMatchingMetadata
- DPProProxBuilder's "make" method renamed as "build"
- The MetadataElement enum elements have been renamed to similar camelCase names (instead of the Underscore_Separated_Ones)
- However, Data_Content was renamed into dataName, as it is a better name for that property
- TODO: Need to test ABEDSProProxy to parse json, instead of XML

Merged DSPro_port_to_VS2017 branch
- Completed VS solution port from VS2015 to VS2017;
- Fixed compilation/linking errors with release/x64 configurations;
- Removed obsolete DSPro JNI wrapper project from the VS solution;
- Renamed method parameters to reflect the change from XML to JSON;
- Added comments to the DSPro API methods;
- Changed NULL preprocessor defined macro to c++11 nullptr keyword;
- fixed code style and removed trailing whitespaces

#### v20181015

post-IM18.
Merged _dspro-reset_ branch:
- Added support for disruption tolerant "reset" message that synchronizes the session key.


### Development Branches
