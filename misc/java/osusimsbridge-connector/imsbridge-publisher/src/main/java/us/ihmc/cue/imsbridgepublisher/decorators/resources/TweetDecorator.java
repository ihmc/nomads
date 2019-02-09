package us.ihmc.cue.imsbridgepublisher.decorators.resources;

import mil.dod.th.core.types.MapEntry;
import us.ihmc.cue.imsbridgepublisher.decorators.LFDecorator;
import us.ihmc.linguafranca.MimeType;
import us.ihmc.linguafranca.MutabilityType;
import us.ihmc.linguafranca.Property;
import us.ihmc.linguafranca.Resource;

import java.io.UnsupportedEncodingException;
import java.util.Base64;
import java.util.Collections;
import java.util.List;

public class TweetDecorator extends AbstractResourceDecorator
{
    @Override
    public void generateObject () {
        if (_currentObservation.isSetReserved()) {
            List<MapEntry> entries = _currentObservation.getReserved();
            boolean isTweet = false;
            for (MapEntry mapEntry : entries) {
                if(mapEntry.getKey().equals("tweet_id")){
                    isTweet = true;
                    break;
                }
            }
            if (isTweet) {
                for (MapEntry entry : entries) {
                    if (entry.getKey().equals("rich_text")){
                        String val = entry.getValue().toString();
                        String encodedTweet;
                        try {
                            encodedTweet = Base64.getEncoder().withoutPadding().encodeToString(val.getBytes("utf-8"));
                            Property iconType = new Property("icon", "icon to show on ATAK", MutabilityType
                                    .ReadOnly.toString(), "tweet");

                            _object = new Resource(MimeType.raw.value(), null, null,
                                    Collections.singletonList(iconType), encodedTweet);
                            break;
                        } catch (UnsupportedEncodingException e) {
                            e.printStackTrace();
                        }
                    }
                }
            }
        }
    }
}
