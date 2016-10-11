package us.ihmc.gst.oa;

import java.util.Date;
import java.util.List;
import us.ihmc.gst.util.DateUtils;
import us.ihmc.gst.util.ElementComparator;
import us.ihmc.gst.util.GeoCoordinates;

/**
 * <?xml version="1.0" encoding="UTF-8"?>
 * <ltsn:EntityIdentMessage xmlns:ism="urn:us:gov:ic:ism:v2"
 *                          xmlns:ltsn="http://ltsn.onr.navy.mil/messaging/" 
 *                          xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 *                          xsi:schemaLocation="http://ltsn.onr.navy.mil/messaging/ MessageTypes.xsd ">
 * <Security ism:FGIsourceOpen="NMTOKENS" ism:FGIsourceProtected="NMTOKENS"
 *           ism:SARIdentifier="NMTOKENS" ism:SCIcontrols="NMTOKENS"
 *           ism:classification="U" ism:classificationReason=""
 *           ism:classifiedBy="" ism:dateOfExemptedSource="2001-01-01"
 *           ism:declassDate="2001-01-01" ism:declassEvent=""
 *           ism:declassException="NMTOKENS" ism:declassManualReview="true"
 *           ism:derivativelyClassifiedBy="" ism:derivedFrom=""
 *           ism:disseminationControls="NMTOKENS"
 *           ism:nonICmarkings="NMTOKENS" ism:ownerProducer="NMTOKENS"
 *           ism:releasableTo="NMTOKENS" ism:typeOfExemptedSource="NMTOKENS"/>
 * <MessageID>0</MessageID>
 * <CommDeviceID>CommDeviceID</CommDeviceID>
 * <SensorID>SensorID</SensorID>
 * <Time>2001-12-31T12:00:00</Time>
 * <Location>
 *  <Latitude>0.0</Latitude>
 *  <Longitude>0.0</Longitude>
 *  <AltitudeInMeters>0.0</AltitudeInMeters>
 *  <AltitudeUncertaintyInMeters>0.0</AltitudeUncertaintyInMeters>
 *  <LocationUncertainty>
 *    <SemiAxis1InMeters>0.0</SemiAxis1InMeters>
 *    <SemiAxis2InMeters>0.0</SemiAxis2InMeters>
 *    <Axis1Orientation>0.0</Axis1Orientation>
 *  </LocationUncertainty>
 * </Location>
 * <EntityFeatures>
    <EntityName>EntityName</EntityName>
    <AssociationID>0</AssociationID>
    <AssociationIDConfidence>0.0</AssociationIDConfidence>
    <MatchConfidence>0.0</MatchConfidence>
    <VideoSensorInfo>
      <SensorAltitude>0.0</SensorAltitude>
      <SensorAltitudeUnits>Meters</SensorAltitudeUnits>
      <SensorElevation>0.0</SensorElevation>
      <SensorAzimuth>0.0</SensorAzimuth>
      <SensorFOV>0.0</SensorFOV>
      <Temperature>0</Temperature>
      <ProcessorLoad>0</ProcessorLoad>
      <Functional>0</Functional>
      <Wavelength>0</Wavelength>
    </VideoSensorInfo>
    <AudioSensorMicArrayInfo>
      <ElementXPos>0.0</ElementXPos>
      <ElementYPos>0.0</ElementYPos>
      <SamplingRate>0</SamplingRate>
      <Bits>0</Bits>
      <ArrayOrientation>0.0</ArrayOrientation>
    </AudioSensorMicArrayInfo>
    <VideoDetectionInfo>
      <DistanceToCamera>0</DistanceToCamera>
      <BodyHeadPoseToCamera>0</BodyHeadPoseToCamera>
    </VideoDetectionInfo>
    <AudioSensorInfoDOA>
      <AngularRes>0.0</AngularRes>
      <NumberSource>0</NumberSource>
      <RelativeAngleOfArrival>0.0</RelativeAngleOfArrival>
    </AudioSensorInfoDOA>
    <AudioDetectInfo>
      <Timestamp>2001-12-31T12:00:00</Timestamp>
      <PoseToMicrophone>0</PoseToMicrophone>
      <VoiceStreamChannelID>0</VoiceStreamChannelID>
      <VoiceBlockID>0</VoiceBlockID>
    </AudioDetectInfo>
    <AudioInfoMfccSegs>
      <HighestFrequency>0</HighestFrequency>
      <Timestamp>0</Timestamp>
      <FFTLength>0</FFTLength>
      <FrameRate>0</FrameRate>
      <SegThreshold>0</SegThreshold>
      <MFCCVectorSize>0</MFCCVectorSize>
    </AudioInfoMfccSegs>
    <VoiceTranscript>
      <VoiceSegID>0</VoiceSegID>
      <TranscriptText>TranscriptText</TranscriptText>
    </VoiceTranscript>
    <AudioVideoAssociation>
      <ClipID>0</ClipID>
      <AudioClipID>0</AudioClipID>
      <VideoClipID>0</VideoClipID>
      <SubjectID>0</SubjectID>
      <AudioID>0</AudioID>
      <Confidence>0.0</Confidence>
    </AudioVideoAssociation>
    <FacialRecognitionInfo>
      <SubjectNumber>0</SubjectNumber>
      <FacePrintUniqueID>0</FacePrintUniqueID>
      <LeftEyePositionX>0</LeftEyePositionX>
      <LeftEyePositionY>0</LeftEyePositionY>
      <RightEyePositionX>0</RightEyePositionX>
      <RightEyePositionY>0</RightEyePositionY>
      <IDMatcher>IDMatcher</IDMatcher>
      <MatchConfidence>0.0</MatchConfidence>
      <FaceProbeImage>FaceProbeImage</FaceProbeImage>
      <FaceGalleryImage>FaceGalleryImage</FaceGalleryImage>
    </FacialRecognitionInfo>
    <LinkedIdent>
      <AudioUniqueEntityID>0</AudioUniqueEntityID>
      <FaceUniqueEntityID>0</FaceUniqueEntityID>
      <VehicleFingerprint>0</VehicleFingerprint>
    </LinkedIdent>
    <GaitRecognitionString>
      <SilhouetteImage>SilhouetteImage</SilhouetteImage>
      <GaitSubjectNumber>GaitSubjectNumber</GaitSubjectNumber>
      <GaitConfidence>0.0</GaitConfidence>
    </GaitRecognitionString>
    <Biometrics>
      <InternalTrackID>0</InternalTrackID>
      <HeightClassification>HeightClassification</HeightClassification>
      <Height>0</Height>
      <HeightConfidence>0.0</HeightConfidence>
      <Width>0</Width>
      <Depth>0</Depth>
      <AgeClassification>AgeClassification</AgeClassification>
      <Age>0</Age>
      <AgeConfidence>0.0</AgeConfidence>
      <WeightClassification>fat</WeightClassification>
      <Weight>0</Weight>
      <WeightConfidence>0.0</WeightConfidence>
      <Gender>Male</Gender>
      <GenderConfidence>0.0</GenderConfidence>
      <Ethnicity>Ethnicity</Ethnicity>
      <EthnicityConfidence>0.0</EthnicityConfidence>
      <FacialHair>FacialHair</FacialHair>
      <FacialHairConfidence>0.0</FacialHairConfidence>
      <HairColor>HairColor</HairColor>
      <HairColorConfidence>0.0</HairColorConfidence>
      <ClothesColor>ClothesColor</ClothesColor>
      <SkinColor>SkinColor</SkinColor>
      <SkinColorConfidence>0.0</SkinColorConfidence>
      <EyeColor>EyeColor</EyeColor>
      <EyeColorConfidence>0.0</EyeColorConfidence>
      <ShirtColor>ShirtColor</ShirtColor>
      <ShirtColorConfidence>0.0</ShirtColorConfidence>
      <PantsColor>PantsColor</PantsColor>
      <PantsColorConfidence>0.0</PantsColorConfidence>
      <PersonColorspace>PersonColorspace</PersonColorspace>
      <PersonColorspaceConfidence>0.0</PersonColorspaceConfidence>
    </Biometrics>
    <TTL>
      <TagID>0</TagID>
    </TTL>
    <AudioPrint>
      <SubjectNumber>0</SubjectNumber>
      <AudioUniqueID>0</AudioUniqueID>
      <AudioMatchConfidence>0.0</AudioMatchConfidence>
      <AudioFingerPrint>0</AudioFingerPrint>
    </AudioPrint>
    <ThroughTheWall>
      <ClosingSpeed>0.0</ClosingSpeed>
      <DetectionConfidence>0.0</DetectionConfidence>
      <ImpSAR>
        <ScanDescription>ScanDescription</ScanDescription>
        <SensorLocation>
          <StartingPosition>
            <Latitude>0.0</Latitude>
            <Longitude>0.0</Longitude>
            <AltitudeInMeters>0.0</AltitudeInMeters>
            <AltitudeUncertaintyInMeters>0.0</AltitudeUncertaintyInMeters>
            <LocationUncertainty>
              <SemiAxis1InMeters>0.0</SemiAxis1InMeters>
              <SemiAxis2InMeters>0.0</SemiAxis2InMeters>
              <Axis1Orientation>0.0</Axis1Orientation>
            </LocationUncertainty>
          </StartingPosition>
          <EndingPosition>
            <Latitude>0.0</Latitude>
            <Longitude>0.0</Longitude>
            <AltitudeInMeters>0.0</AltitudeInMeters>
            <AltitudeUncertaintyInMeters>0.0</AltitudeUncertaintyInMeters>
            <LocationUncertainty>
              <SemiAxis1InMeters>0.0</SemiAxis1InMeters>
              <SemiAxis2InMeters>0.0</SemiAxis2InMeters>
              <Axis1Orientation>0.0</Axis1Orientation>
            </LocationUncertainty>
          </EndingPosition>
        </SensorLocation>
        <FileCount>0</FileCount>
        <ObjectCount>0</ObjectCount>
        <File>
          <Description>Description</Description>
          <URL>http://tempuri.org</URL>
          <Type>Type</Type>
          <Encoding>Encoding</Encoding>
          <Content>Content</Content>
        </File>
        <ObjectParameter>
          <Type>Type</Type>
        </ObjectParameter>
        <Description>Description</Description>
        <Content>Content</Content>
      </ImpSAR>
 *       <TUSSensorSAIC>
 *      <MinimumRangeScanned>0.0</MinimumRangeScanned>
 *      <MaximumRangeScanned>0.0</MaximumRangeScanned>
 *      <MinimumAzimuthAngle>0.0</MinimumAzimuthAngle>
 *      <MaximumAzimuthAngle>0.0</MaximumAzimuthAngle>
 *      <Orientation>0.0</Orientation>
 *      <ID>ID</ID>
 *      <HumanEntity>
 *        <MovingMotion>
 *          <Speed>0.0</Speed>
 *          <Direction>0.0</Direction>
 *        </MovingMotion>
 *        <StationaryMotion>
 *          <RespirationRate>0.0</RespirationRate>
 *          <HeartRate>0.0</HeartRate>
 *        </StationaryMotion>
 *        <ObjectEntity>
 *          <Length>0.0</Length>
 *          <Depth>0.0</Depth>
 *          <Orientation>0.0</Orientation>
 *        </ObjectEntity>
 *      </HumanEntity>
 *      <Note>Note</Note>
 *    </TUSSensorSAIC>
 *  </ThroughTheWall>
 *  <VehicleID>
 *    <VehicleUniqueID>VehicleUniqueID</VehicleUniqueID>
 *    <VehicleFingerprint>VehicleFingerprint</VehicleFingerprint>
 *    <Description>Description</Description>
 *  </VehicleID>
 *  <VehicleDismountAssoc>
 *    <AssociationID>0</AssociationID>
 *    <VehicleUniqueID>VehicleUniqueID</VehicleUniqueID>
 *    <TrackSource>TrackSource</TrackSource>
 *  </VehicleDismountAssoc>
 *  <CobhamHumanDetection>
 *    <HumanActivity>0</HumanActivity>
 *    <LevelOfHumanActivity>0</LevelOfHumanActivity>
 *    <StillImageryAvailable>0</StillImageryAvailable>
 *    <VideoAvailable>0</VideoAvailable>
 *    <BlueForceTrackAvailable>0</BlueForceTrackAvailable>
 *    <NumberTracks>0</NumberTracks>
 *  </CobhamHumanDetection>
 *  <GeoCulturalDroid>
 *    <AlertOrObservationID>AlertOrObservationID</AlertOrObservationID>
 *    <AlertOrObservation>AlertOrObservation</AlertOrObservation>
 *    <BackUpFacts>BackUpFacts</BackUpFacts>
 *     </GeoCulturalDroid>
 * </EntityFeatures>
 * <RelatedLink>http://tempuri.org</RelatedLink>
 * <CanonicalURI>token</CanonicalURI>
 * </ltsn:EntityIdentMessage>
 *                                                                                                                                                                                       236,1         Bot
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class EntityIdentMessage extends ClassifiedMessage
{
    public static final String MAGIC_STRING = "EntityIdentMessage";

    private Location _location;
    private Date _date;
    private String _messageId;
    private String _sensorId;
    private String _type;
    private String _features;

    private enum Element {
        Location,
        Latitude,
        Longitude,
        AltitudeInMeters,
        Time,
        SensorID,
        MessageID,
        Type,
        Features
    }

    public EntityIdentMessage()
    {
        super();
        _location = new Location();
        _messageId = "";
        _sensorId = "";
        _type = "";
        _features = "";
        _date = null;
    }

    @Override
    protected String getMagicString()
    {
        return MAGIC_STRING;
    }

    public String getMessageId()
    {
        return _messageId;
    }

    public String getSensorId()
    {
        return _sensorId;
    }

    public String getType()
    {
        return _type;
    }

    public String getFeatures()
    {
        return _features;
    }

    public Location getLocation()
    {
        return _location;
    }

    public Date getDate()
    {
        return _date;
    }

    @Override
    protected void readElement(List<String> elements, String val) {
        super.readElement (elements, val);

        if (ElementComparator.matches (elements, Element.Latitude.name(), Element.Location.name())) {
            // Latitude is in the ] -90.0, 90.0[ range
            Double coord = Double.parseDouble (val);
            if (GeoCoordinates.isValidLatitude (coord)) {
                _location._latitude = coord.floatValue();
            }
        }
        else if (ElementComparator.matches (elements, Element.Longitude.name(), Element.Location.name())) {
            // Longitude is in the ] -180.0, 180.0[ range
            Double coord = Double.parseDouble (val);
            if (GeoCoordinates.isValidLongitude (coord)) {
                _location._longitude = coord.floatValue();
            }
        }
        else if (ElementComparator.matches (elements, Element.AltitudeInMeters.name(), Element.Location.name())) {
            
        }
        else if (ElementComparator.matches (elements, Element.Time.name())) {
            _date = DateUtils.parseDate (val);
        }
        else if (ElementComparator.matches (elements, Element.SensorID.name())) {
            _sensorId = val;
        }
        else if (ElementComparator.matches (elements, Element.MessageID.name())) {
            _messageId = val;
        }
        else if (ElementComparator.matches (elements, Element.Type.name())) {
            _type = val;
        }
        else if (ElementComparator.matches (elements, Element.Features.name())) {
            _features = val;
        }
    }

    @Override
    protected void readProperty(List<String> elements, String property, String val)
    {
        super.readProperty(elements, property, val);
    }
}
