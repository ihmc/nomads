#ifndef C45TYPES_H_
#define C45TYPES_H_

/*********************************************************************************************/
//		Type definitions for C4.5
/*********************************************************************************************/

typedef char Boolean, * String, * Set;
typedef int ItemNo;         // data item number
typedef float ItemCount;    // count of (partial) items
typedef short ClassNo;	    // class number, 0..MaxClass
typedef short DiscrValue;   // discrete attribute value (0 = ?)
typedef short Attribute;    // attribute number, 0..MaxAtt
typedef short RuleNo;	    // rule number

typedef union _attribute_value {    // used to keep items
    DiscrValue _discr_val;
    float _cont_val;
} AttValue, * Description;

#define CVal(Case,Attribute)   Case[Attribute]._cont_val
#define DVal(Case,Attribute)   Case[Attribute]._discr_val
#define Class(Case)            Case[MaxAtt+1]._discr_val

#define Unknown  -999   // unknown value for continuous attribute
#define NodeTypeLeaf 0  // node types:	leaf
#define BrDiscr	1	// node types:	branch
#define ThreshContin 2	// threshold cut
#define BrSubset 3	// subset test
#define IGNORE 1	// special attribute status: do not use
#define DISCRETE 2	// ditto: collect values as data read

/********************************* tree options struct ****************************************/

// this struct contains all the options for processTree

typedef struct {
    int initialWindow;
    int	maxWindow;
    int	increment;
    float cycleErrors;
    float cycleWindow;
    Boolean probThresh;
    Boolean gainCriterion;
    float pruneConfidence;
    Boolean verbosity;
    Boolean subsetting;
    int	 minObjects;
} treeOptions;

/******************************** rule set options struct ************************************/

// this struct contains all the options for processRules

typedef struct {
    float pruneConfidence;
    float redundancy;
    float fisherThresh;
    Boolean isFisherInUse;
    Boolean	annealing;
    Boolean	verbosity;
} ruleOptions;

/******************************** tree classes, attributes and values names ******************/

// the struct is used to configure the tree and the rule set

typedef struct {
    short MaxClass;         // number of different classes
    char * * ClassName;     // array with lenght MaxClass that contains the names of the classes
    short MaxAtt;	    // number of different attributes
    char * * AttName;	    // array with lenght MaxAtt that contains the names of the attributes
    short * MaxAttVal;	    // array with lenght MaxAtt that contains the number of different values for each attribute
    char * * * AttValName;  // 2D array with lenght MaxAtt*MaxAttVal[noAtt] that contains values names for each attribute
    char * SpecialStatus;   // tag for every attribute that indicates when an attribute is discrete
                            // without specified names for the values, or indicates the status "IGNORE"
    short MaxDiscrVal;      // maximum discrete values for any attribute
} Configure;

/******************************** errors struct **********************************************/

// this struct contains a code and a message that explain the occurred errors when needed

typedef struct {
    short errorCode;        // a code associated with the error
    char * errorMessage;    // a message that explains the error occurred
} CError;                   // errorCode = 1 means: the passed parameter is NULL
                            // errorCode = 2 means: unable to read classes from string names
                            // errorCode = 3 means: unable to read attributes from string names
                            // errorCode = 4 means: unable to read values from string names
                            // errorCode = 5 means: illegal number of discrete values in string names
                            // errorCode = 6 means: illegal number of values in string data
                            // errorCode = 7 means: illegal value in string data or phrase
                            // errorCode = 8 means: illegal class name in string data
                            // errorCode = 9 means: there isn't any input tree
                            // errorCode = 10 means: illegal probability values in string phrase

/********************************* results from processTree ***********************************/

typedef struct _tree_record * Tree;
typedef struct _tree_record {
    short NodeType;             // 0=leaf 1=branch 2=cut 3=subset
    ClassNo Leaf;               // most frequent class at this node
    ItemCount Items;            // no of items at this node
    ItemCount * ClassDist;      // class distribution of items
    ItemCount Errors;           // no of errors at this node
    Attribute Tested;           // attribute referenced in test
    short Forks;                // number of branches at this node
    float Cut;                  // threshold for continuous attribute
    float Lower;                // lower limit of soft threshold
    float Upper;                // upper limit ditto
    Set * Subset;               // subsets of discrete values
    Tree * Branch;              // Branch[x] = (sub)tree for outcome x
} TreeRec;

typedef struct {
    int treeSize;           // size of the tree: number of nodes in the tree
    int noErrors;           // number of misclassified items in the test
    int noItems;            // total number of analyzed items in the test
    float percErrors;       // % of errors on the total number of items analyzed
    float estimate;         // estimate the % of errors (only for pruned trees)
    int * confusionMatrix;  // an array of int[noClasses * noClasses] that represent the confusion matrix for this tree
    int noClasses;          // number of different classes
} testRes;

typedef struct _result_ {
    Tree tree;		// a single complete tree
    short isPruned;		// if = 1 means that this tree is pruned, if = 0 the tree is unpruned
    testRes	* testResults;	// test results pointer
} TRes;

typedef struct _result_ * treeResults;
typedef struct {
    treeResults * trees;    // an array of treeResults structs
    int nTrees;             // give the no of trees. The function processTree returns more than 2 trees when the trials
                            // option is set. The pruned tree is always the last one. It could be set to a different no.
                            // of trees when a *processTreeResults variable is used as input for processRules function
                            // (default = 1). Pass to consultTree processTreeResults->trees[x], where trees[x] is the
                            // tree you want to consult.
     CError	* codeErrors;   // reports errors occurred
} processTreeResults;

/********************************* results from processRules ************************************/

typedef struct TestRec * Test;
struct TestRec {
    short NodeType;     // test type (see tree nodes)
    Attribute Tested;   // attribute tested
    short Forks;        // possible branches
    float Cut;          // threshold (if relevant)
    Set * Subset;       // subset (if relevant)
};

typedef struct CondRec * Condition;
struct CondRec {
    Test CondTest;      // test part of condition
    short TestValue;    // specified outcome of test
};

typedef struct ProdRuleRec PR;
struct ProdRuleRec {
    short Size;         // number of conditions
    Condition * Lhs;    // conditions themselves
    ClassNo     Rhs;    // class given by rule
    float Error;        // estimated error rate
    float Bits;         // bits to encode rule
    ItemNo Used;        // times rule used
    ItemNo Incorrect;   // times rule incorrect
    int better;         // number of cases where the rule predict the right value
    int worse;          // number of cases where the rule predict the a wrong class
                        // Advantage = better - worse
    float errorRate;    // percent of errors
};

typedef struct {
    int	noItems;                // total number of analyzed items in the test
    int	noErrors;               // number of misclassified items in the test
    float percErrors;           // % of errors on the total number of items analyzed
    int * confusionMatrix;	// an array of int[noClasses][noClasses] that represent the confusion matrix for this ruleset
    int	noClasses;              // number of different classes
} testRes2;

typedef struct RuleSetRec RuleSet;
struct RuleSetRec {
   PR * SRule;              // rules
   RuleNo SNRules;          // number of rules
   RuleNo * SRuleIndex;	    // ranking of rules
   ClassNo SDefaultClass;   // default class for this ruleset
   short isComposite;	    // if = 1 means that this is a composite ruleset
   char * DefaultClassName; // default class name
   testRes2 * testResults;  // results from a test
};

typedef struct {
    RuleSet * set;          // an array of RuleSet structs
    int nSet;               // number of RuleSet present in the array. If nSet is > 1 it means that there are nSet-1
                            // RuleSet constructed from nSet-1 trees, plus in processRulesResults[nSet-1] there's the
                            // final RuleSet resulted from the sum of all RuleSets
    CError * codeErrors;    // reports errors occurred
} processRulesResults;

/****************************** results from consultTree *************************************/

typedef struct {
    char * className;       // resulted class
    float guessProb;        // best guess probability
    float lowProb;          // the probability that the class is correct is in the intervall:
    float upperProb;        // [lowProbability - upperProbability]
    int nClasses;           // give the no of classes that the item might belong to, usually is 1
    CError * codeErrors;    // reports errors occurred
} consultTreeResults;

/****************************** results from consultRules *************************************/

typedef struct {
    char * className;       // resulted class
    float probability;      // probability that the resulted class is correct
    Boolean isDefault;      // = true if the resulted class is the default class
    CError * codeErrors;    // reports errors occurred
} consultRulesResults;

#endif /*TYPES_H_*/
