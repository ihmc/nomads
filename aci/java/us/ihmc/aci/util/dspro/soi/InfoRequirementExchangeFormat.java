package us.ihmc.aci.util.dspro.soi;

/**
 * InfoRequirementExchangeFormat.java
 * <p/>
 * Class containing fields parsed from <code>InformationObject</code> instances of information requirement data type.
 *
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class InfoRequirementExchangeFormat extends ARLExchangeFormat
{
    public static final String INFO_REQ_ID = "information.requirement.id";
    public static final String GROUP_ID = "group.id";
    public static final String STATUS = "status";
    public static final String PRIORITY = "priority";
    public static final String STANDING = "standing";
    public static final String CATEGORY = "category";
    public static final String SUBCATEGORY = "subcategory";

    public static final String APPROVAL_STATE = "approval.state";
    public static final String APPROVAL_TOPIC = "approval.topic";
    public static final String APPROVAL_METHOD = "approval.method";

    public static final String NAMED_BOUNDS_COUNT = "named.bounds.count";
    public static final String NAMED_BOUNDS_NAME = "named.bounds.name.number.";
    public static final String NAMED_BOUNDS_DISTANCE = "named.bounds.distance.number.";
    public static final String NAMED_BOUNDS_POINT = "named.bounds.point.number.";
    public static final String NAMED_BOUNDS_POINT_COUNT = ".count";
    public static final String NAMED_BOUNDS_POINT_COORD_GRID = ".coord.grid.number.";
    public static final String NAMED_BOUNDS_TYPE = "named.bounds.type.number.";

    public static final String PEDIGREE_RULE = "pedigree.rule";
    public static final String PEDIGREE_CLASS = "pedigree.class";
    public static final String PEDIGREE_REFERENCE_GRID = "pedigree.reference.grid";
    public static final String PEDIGREE_CATEGORY_COUNT = "pedigree.category.count";
    public static final String PEDIGREE_CATEGORY_ID = "pedigree.category.id.number";
    public static final String PEDIGREE_SOURCE_TYPE = "pedigree.source.type";
    public static final String PEDIGREE_SOURCE_NAME = "pedigree.source.name";
    public static final String PEDIGREE_TASK_COUNT = "pedigree.task.count";
    public static final String PEDIGREE_TASK_ID = "pedigree.task.id.number";
    public static final String PEDIGREE_TASK_NAME = "pedigree.task.name.number";

    public static final String INTENT_OBJECTIVE_COUNT = "intent.objective.count";
    public static final String INTENT_OBJECTIVE_TYPE = "intent.object.type.number.";

    public static final String CONSTRAINTS_FRESHNESS_UNITS = "constraints.freshness.units";
    public static final String CONSTRAINTS_FRESHNESS_QUANTITY = "constraints.freshness.quantity";
    public static final String CONSTRAINTS_COLLECTION_BEGIN_TIME = "constraints.collection.begin.time";
    public static final String CONSTRAINTS_COLLECTION_END_TIME = "constraints.collection.end.time";
    public static final String CONSTRAINTS_DELIVERY_BEGIN_TIME = "constraints.delivery.begin.time";
    public static final String CONSTRAINTS_DELIVERY_END_TIME = "constraints.delivery.end.time";

    public static final String THRESHOLD_UNCONDITIONAL_ALERT_CAPTION = "threshold.unconditional.missionAlert.caption.number.";
    public static final String THRESHOLD_UNCONDITIONAL_ALERT_MESSAGE = "threshold.unconditional.missionAlert.message.number.";

    public static final String THRESHOLD_ALERT_COUNT = "threshold.missionAlert.count";
    public static final String THRESHOLD_ALERT_CAPTION = "threshold.missionAlert.caption.number.";
    public static final String THRESHOLD_ALERT_MESSAGE = "threshold.missionAlert.message.number.";
    public static final String THRESHOLD_ALERT_MEASUREMENT = "threshold.missionAlert.measurement.number.";
    public static final String THRESHOLD_ALERT_MEASUREMENT_MESSAGE = "threshold.missionAlert.measurement.message.number.";
    public static final String THRESHOLD_ALERT_MEASUREMENT_CAPTION = "threshold.missionAlert.measurement.caption.number.";
    public static final String THRESHOLD_ALERT_QUANTITY = "threshold.missionAlert.quantity.number.";
    public static final String THRESHOLD_ALERT_VARIABLE = "threshold.missionAlert.variable.number.";
    public static final String THRESHOLD_ALERT_TYPE_BOUNDS_REF = "threshold.missionAlert.bounds.ref";
    public static final String THRESHOLD_ALERT_TYPE_COMPAR_OPERATOR = "threshold.missionAlert.compar.operator";
    public static final String THRESHOLD_ALERT_TYPE_DATA = "threshold.missionAlert.compar.data";
    public static final String THRESHOLD_ALERT_TYPE_VALUE = "threshold.missionAlert.compar.value";

    public static final String THRESHOLD_RELIABILITY_INFO_LEVEL = "threshold.reliability.info.level";
    public static final String THRESHOLD_RELIABILITY_SOURCE_LEVEL = "threshold.reliability.source.level";

    public static final String INFONEED_QUERY_BODY = "infoneed.query.body";
    public static final String INFONEED_QUERY_VARIABLES_INPUT_COUNT = "infoneed.query.variables.input.count";
    public static final String INFONEED_QUERY_VARIABLES_INPUT_VALUE = "infoneed.query.variables.input.value.number.";
    public static final String INFONEED_QUERY_VARIABLES_OUTPUT_COUNT = "infoneed.query.variables.output.count";
    public static final String INFONEED_QUERY_VARIABLES_OUTPUT_NAME = "infoneed.query.variables.output.name.number.";
    public static final String INFONEED_QUERY_VARIABLES_OUTPUT_TYPE = "infoneed.query.variables.output.type.number.";
    public static final String INFONEED_QUERY_VARIABLES_OUTPUT_CLASS = "infoneed.query.variables.output.class.number.";
    public static final String INFONEED_QUERY_VARIABLES_OUTPUT_CONCEPT_NAME = "infoneed.query.variables.output.concept.name.number.";
    public static final String INFONEED_QUERY_VARIABLES_OUTPUT_PREDICATE_NAME = "infoneed.query.variables.output.predicate.name.number.";

//    public static final String DSPRO_GHUB_DESCRIPTION = "InformationRequirement";   // Moved to DSProMimeType
//    public static final String DSPRO_GHUB_MIME_TYPE = "x-dspro/x-soi-ghub-information-requirement";
//    public static final String DSPRO_INFO_REQUIREMENT_MIME_TYPE = "x-dspro/x-soi-information-requirement";  // Moved to DSProMimeType
}
