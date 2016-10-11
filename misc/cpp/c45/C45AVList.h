/*
 * C45AVList.h
 *
 * This file is part of the IHMC C4.5 Decision Tree Library.
 * Copyright (c) 2008-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 * Created on November 23, 2011, 12:00 PM
 */

#ifndef INCL_C45_AVLIST_H
#define INCL_C45_AVLIST_H

#include "AVList.h"

namespace NOMADSUtil
{
    class String;
    class AVList;
}

namespace IHMC_C45
{
    class C45AVList : public NOMADSUtil::AVList
    {
        public:
            // keywords
            static const NOMADSUtil::String _CLASS;
            static const NOMADSUtil::String _UNKNOWN;
            static const NOMADSUtil::String _CONTINUOUS;
            static const NOMADSUtil::String _IGNORE;
            static const NOMADSUtil::String _DISCRETE;
            static const NOMADSUtil::String _DEFAULT_CLASS;

            explicit C45AVList (unsigned int uiInitialSize = 0U);
            virtual ~C45AVList (void);

            C45AVList * concatListsInNewOne (C45AVList *pFirst, C45AVList *pSecond);
            C45AVList * copyList (void);		// Create a copy of the object.
    };
}

#endif // INCL_C45_AVLIST_H

/************************************* How to use C45AVList class ********************************************/
/*
 * This class is passed as a parameter in some methods in C45DecisionTree, C45Rules and DataGenerator classes,
 * and represent the input for the c4.5 decision tree algorithm and for the data gnerator.
 * 
 * c4.5 input types:
 * 
 *  1) a list of all the possible attributes and their values, used by "configureTree" function
 *  2) a dataset composed as a list of examples, use to construct a tree or test it
 *  3) a record, used to consult a tree or a rule-set
 * 
 * data generator input types:
 * 
 *  1) a list of all the possible attributes and their values, just like c4.5
 *  4) a list of rules, each of them composed by conditions
 *  5) a list of ranges for continuous attributes
 *
 * Each type of input can be represented by the class C45AVList, but is important to know that each type has its
 * own syntax, that must be followed or the input won't be accepted.
 */
/************************************* 1 - Attributes names list syntax *****************************************/
/*
 * To work properly, the first step is configure the decision tree algorithm with a list of all the attributes
 * and their values that will be used in the tree. Here are the rules to follow while building this type of input:
 * 
 *  + The number of different attributes has no limit and can be set as needed, but there must be one and only
 *    one attribute tagged as "class" (using _CLASS constant specified in C45AVList) that represent the attribute
 *    whose values will be predicted by the c4.5 algorithm. These values usually are in number of 2, but they could
 * 	  be a different number. All the attributes and the "class" must be together in the same instance of C45AVList.
 *    The order is not important, so the "class" could be added at the begin, at the end or in the middle. Suppose
 *    that we have created an instance of C45AVList called "treeAttributes", and that we want that our tree will
 *    predict as output the values "play" and "don't play". In order to do this, the correct way is this:
 *
 *						treeAttributes->addPair(C45AVList::_CLASS, "play, don't play");
 *
 *    Note that the first parameter must be the constant value "treeAttributes->_CLASS" and the second parameter
 *    a string containing the concatenation of all the possible values for the "class", and each value must be
 *    separated by comma.
 *
 *  + Now let consider all the other attributes used by the algorithm to compute the prediction. The c4.5 algorithm
 *    accepts 4 different ways to represent an attribute:
 *
 *     - attributes with real values
 *     - attributes that are present in the input dataset but we don't want to use them in the tree
 *     - attributes with a known number of discrete values, but without any names for the values
 *     - attributes with discrete values, and each value has a name
 *
 *    Again, each type of attribute has is own syntax, and they are explained below.
 *
 *  + Attributes with real values:
 *    The algorithm will manage this type of attribute with float variables, regardless of the input type. It means
 *    that if your attribute has integer values, the algorithm will convert it into a float. To add a real values
 *    attribute, imagine that its name is "temperature", to the list of attributes, do in this way:
 *
 *						treeAttributes->addPair("temperature", C45AVList::_CONTINUOUS);
 *
 *    The attribute name must be the first parameter, followed by the constant value "_CONTINUOUS" declared in C45AVList.
 *
 *  + Attributes to be ignored:
 *    Use this type of attribute if you want the tree doesn't take in consideration this attribute. In this
 *    case, the algorithm will simply skip this attribute when it appear in the dataset. For example, if you want
 *    that your tree won't consider the attribute called "humidity", do in this way:
 * 
 * 						treeAttributes->addPair("humidity", C45AVList::_IGNORE);
 * 
 *  + Attributes with a discrete number of values:
 *    Use this type of attribute only when you don't know in advance the names of the values; if you know them, it is
 *    better to use the type "discrete with known values" because it permits the algorithm to check the input values
 *    in the dataset. Imagine that you have an attribute called "wind" and you know that it can assume 8 different
 *    values but you don't known the names of these values, here is the correct syntax to do that:
 * 
 * 						NOMADSUtil::String value = C45AVList::_DISCRETE; 
 *						value += "8";
 * 						treeAttributes->addPair("wind", value);
 * 
 *    Note that:
 * 
 *             			treeAttributes->addPair("wind", 8);
 * 
 *    has a different meaning, it represent an attribute "wind" that has a single value called "8". That attribute
 *    won't be accepted by the algorithm, because the minimum possible number of values for an attribute is 2. Is
 *    also important to note that the same instruction has a different meaning when you are creating a dataset instead
 *    of a list of attributes (see below for detailed information about the dataset). 
 * 
 *  + Attributes with known discrete values:
 *    Use this type of attribute when you have an attribute that can assume a finite number of values, and these
 *    values have a name (a name could also be "1", "2", etc). For example if we have an attribute called "outlook"
 *    that can assume any of these values: "rain", "sunny", "overcast", "cloudy", this is the correct way to add
 *    this attribute to the AVList instance:
 * 
 * 						treeAttributes->addPair("outlook", "rain, sunny, overcast, cloudy");
 * 
 *    The attribute name must be the first parameter, and the second parameter must be a concatenation of the values
 *    names, separated by comma.
 */ 
/***************************************** 2 - Dataset syntax *******************************************************/
/*
 * A dataset is a list of examples, and each example is a list of values that represent a particular event in the
 * past. For istance, an example could be:
 * 
 * 								temperature  =  85
 * 								humidity     =  64
 * 								wind         =  strong
 * 								outlook      =  rain
 * 								_CLASS       =  don't play
 * 
 * This could represent what happened in a day X in the past. To work properly, the c4.5 algorithm needs a training
 * dataset to be able to construct a tree on it. To create a dataset, simply allocate an other instance of class
 * C45AVList, for example called "dataset", and add to it all the examples using the method "addPair". With that method
 * you can add just one couple "attribute/value" (for example "outlook", "rain") at a time. The order is not important
 * while adding the attribute of an example, but is important to add the examples one by one. For instance, if you
 * have 2 examples, begin adding all the attributes for example 1, and start to add the attributes of example 2 only
 * when you have already added all the attributes of example 1.
 * 
 *  + Add a real value:
 *    Use this way to add the values of attributes previously declared as "_CONTINUOUS". The first parameter must be
 *    the name of the attribute, the second parameter could be an integer or a float, but the inserted value is always
 *    converted to a float.
 * 
 * 								dataset->addPair("temperature", 83);   (int)
 * 								dataset->addPair("humidity", 65.74);   (float)
 * 
 *    Note that you can add any type of value for an attribute that was declared "_IGNORE", but the algorithm will
 *    ignore that value.
 * 
 *  + Add a discrete value:
 *    In this way you could add string values for attributes previously declared "discrete with known values" and
 *    "discrete with unknown values".
 * 
 * 								dataset->addPair("wind", "strong");
 * 
 *  + Add an unknown value:
 *    If in an example, you don't know the value of an attribute for any reason, you must add this attribute anyway
 *    because every example must contains all the declared attributes. If, for instance, the value of the attribute
 *    "wind" is unknown, you have to specify it with the proper constant "_UNKNOWN" available in the class C45AVList
 *    in this way:
 * 
 * 								dataset->addPair("wind", C45AVList::_UNKNOWN);
 * 
 *  + Add a "class" value:
 *    Each example must contains a "class" value. The "class" value must be known and specified. So is not possible
 *    to add an example without a known "class", and this makes sense because the goal of examples is to learn
 *    the right "class" value from a past event with some specified attributes values, so an example with unknown
 *    "class" is completely useless. This is the way to specify the "class" value:
 * 
 * 								dataset->addPair(C45AVList::_CLASS, "don't play");
 */
/***************************************** 3 - Record syntax **********************************************************/
/*
 * When you want to use a created decision tree to make a prediction, you need a type of input called record.
 * The record is very similar to an example, but a record doesn't contain the "class" value of course, this value
 * will be predict by the tree or the rule-set. For instance, a record could be:
 * 
 * 								temperature  =  56.1
 * 								humidity     =  80
 * 								wind         =  absent
 * 								outlook      =  sunny
 * 
 * To create a record, just allocate a new instance of C45AVList class, for example called "record" and add couples
 * "attribute/value" with the method "addPair". There is a specific syntax for the records that allow to query the
 * tree/rule set in different ways.
 * 
 *  + Add an unknown value:
 *    In this case the syntax is equal to the unknowns value in an example
 * 
 * 								record->addPair("wind", C45AVList::_UNKNOWN);
 * 
 *  + Add a single value:
 *    When you want to specify a simple value, you can do it in this way:
 * 
 * 								record->addPair("humidity", 80);        (integer, continuous)
 * 								record->addPair("temperature", 56.1);   (float, continuous)
 * 								record->addPair("outlook", "sunny");    (string, discrete)
 * 
 *    Note that if you are adding a value for an attribute that was declared "discrete", this value must be a string,
 *    if you are adding a value for a "continuous" attribute, this value could be an integer or a float, bur remember
 *    that the algorithm convert all the values to float.
 * 
 *  + Add a range of values:
 *    You can specify a range of values instead of a single value only for continuous attributes. For the algorithm
 *    it means that the value of the attribute is somewhere in the given range. For example, if you want to specify
 *    that the attribute "temperature" is in the range [60 - 65], you have to convert the range in a string format
 *    as: "60, 65" where there must be exactly 2 numeric values (the algorithm will convert them to float) separated
 *    by a comma; and then use this instruction:
 * 
 * 								record->addPair("temperature", "60, 65");
 * 
 *  + Add a set of "value/probability":
 *    This type of input can be used only for discrete attributes, and allow to specify a list of all or part of the
 *    possible values for an attribute and a probability for each value. If the sum of the probabilities is less than 1
 *    and you have listed just a part of the values, the remaining probability is distributed equally among the
 *    unspecified values of the attribute. If the sum of all the listed probabilities is greater than 1, the record
 *    won't be accepted by the algorithm. The list of "value/probability" must be in a string format and respect this
 *    syntax: "value1:prob1, value2:prob2, value3:prob3" the couples "value/probability" must be separated by comma
 *    and there must be a colon between the value and its probability. Here is an example:
 * 
 * 								record->addPair("outlook", "sunny:0.15, rain:0.3");
 */
/********************************************* 4 - Rules syntax ****************************************************/
/*
 * The DataGenerator class uses input rules to generate data in a more realistic way. The DataGenerator class generates
 * random values for each attribute, but the "class" value is generated following the given rules. The input for
 * DataGenerator is an array of C45AVList instances. Each C45AVList instance in the array represents a single rule.
 * In the array there must be one rule that represents the default class and a variable number of rules to represent
 * relations between some attributes and one of the possible class values. If one of the rules matches the attributes
 * values generated, the class value indicated in the rule is selected. If no one of the rules matches, the default
 * class value is chosen. The following example illustrate how to compose the input for DataGenerator into a C45AVList
 * array called "rules":
 * 
 *  + Default class: The order used to insert the rules in the array is not important, so the default class could be
 *    added at any point. The rule that contains the default class must contain just that and not other conditions.
 *    This is the syntax to specify the default class:
 * 
 * 								rules[N]->addPair(C45AVList::_DEFAULT_CLASS, "play");
 * 
 *  + Specify a single rule: A rule is composed by a set of conditions and a specified "class" value. Each condition
 *    is defined with a single call of "addPair()" method. A condition specifies an attribute name (first parameter in
 *    addPair() ) and a value name (second parameter). This is an example of condition for a "_DISCRETE" attribute:
 * 
 * 								rules[N]->addPair("outlook", "sunny");
 * 
 *    It is also possible to specify logic operators if the attribute is defined as "_CONTINUOUS". The operator must
 *    be included in the second parameter after the value name. Possible operators are: <, >, <=, >=, != . "=" is the
 *    default operator (used in all the conditions for discrete attributes) and is omitted in the conditions. These are
 *    some examples of conditions on continuous attributes:
 * 
 * 								rules[N]->addPair("temperature", 70);	(temperature = 70)
 * 								rules[N]->addPair("humidity", "50,>="); (humidity >= 50)
 * 
 *    If there are more than one condition on the same attribute, the DataGenerator do an OR between the conditions.
 *    For example:
 * 
 * 								rules[N]->addPair("outlook", "sunny");
 * 								rules[N]->addPair("outlook", "rain");	(outlook = sunny OR rain)
 * 
 *    A rule is satisfied if all the conditions are satisfied:
 * 
 * 			RULE SATISFIED = (conditions on attribute 1) AND ... AND (conditions on attribute N)
 * 
 *    If a rule is satisfied the associated class value is chosen. To specify a class value for a rule the syntax is
 *    this:
 * 
 * 								rules[N]->addPair(C45AVList::_CLASS, "don't play");
 * 
 * The follow is an example of a complete set of 4 rules:
 * 
 * 								rules[0]->addPair("temperature", "70, >");
 * 								rules[0]->addPair("humidity", "80, <=");
 * 								rules[0]->addPair("outlook", "sunny");
 * 								rules[0]->addPair(C45AVList::_CLASS, "play");
 * 
 * 								rules[1]->addPair("outlook", "overcast");
 * 								rules[1]->addPair("outlook", "rain");
 * 								rules[1]->addPair("humidity", "85, >=");
 * 								rules[1]->addPair(C45AVList::_CLASS, "don't play");
 * 
 * 								rules[2]->addPair("temperature", "50, <");
 * 								rules[2]->addPair("temperature", "100, >");
 * 								rules[2]->addPair("temperature", 62);
 * 								rules[2]->addPair(C45AVList::_CLASS, "don't play");
 *
 * 								rules[3]->addPair(C45AVList::_DEFAULT_CLASS, "play"); 
 * 
 * Meaning of the rules:
 * 
 * 		rule 0 : CLASS = "play" if (temperature > 70) AND (humidity <= 80) AND (outlook = "sunny")
 * 		rule 1 : CLASS = "don't play" if ((outlook = "overcast") OR (outlook = "rain")) AND (humidity >= 85)
 * 		rule 2 : CLASS = "don't play" if (temperature < 50) OR (temperature > 100) OR (temperature = 62)
 * 		rule 3 : CLASS = "play" if (! rule 0) AND (! rule 1) AND (! rule 2)
 */
/********************************************* 5 - Ranges syntax ***************************************************/
/*
 * Ranges are used as input by DataGenerator class. All the ranges must be specified inside the same C45AVList instance.
 * It's possible to specify a range for each attribute declared as "_CONTINUOUS". They are not mandatory but, when they
 * are specified, the data generator generate continuous values inside the specified ranges, obtaining more realistic
 * values. For example, to specify a range for the attribute "temperature" the first step is to create an instance of
 * C45AVList, here called "ranges" and specify the range in a string format in this way:
 * 
 * 								ranges->addPair("temperature", "-40, 120");
 */
