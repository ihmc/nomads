/*************************************************************************/
/*								 	 */
/*	User interface for consulting trees and rulesets	 	 */
/*	------------------------------------------------	 	 */
/*								 	 */
/*************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "defns.h"
#include "types.h"
#include "extern.h"

typedef	struct ValRange	* RangeDescRec;
struct ValRange {
    Boolean	Known;
    Boolean Asked;
    float LowerBound;
    float UpperBound;
    float * Probability;
};

extern RangeDescRec	RangeDesc;

#define Fuzz 0.01

// functions declaration
CError * CheckValue(Attribute Att, Tree T, const char * phrase);
CError * ReadRange(Attribute Att, Tree T, const char * phrase);
CError * ReadContin(Attribute Att, Tree T, const char * phrase);
CError * ReadDiscr(Attribute Att, Tree T, const char * phrase);
void PrintRange(Attribute Att);
void SkipLine(char c);
void Clear(void);

/*************************************************************************/
/*									 */
/*	Ask for the value of attribute Att if necessary			 */
/*									 */
/*************************************************************************/

CError * CheckValue(Attribute Att, Tree T, const char * phrase)
{
    if(RangeDesc[Att].Asked) return NULL;
    Verbosity(1) {
        printf("%s", AttName[Att]);
        if(RangeDesc[Att].Known) {
            printf(" [ ");
            PrintRange(Att);
            printf(" ]");
        }
        printf(": ");
    }
    return ReadRange(Att, T, phrase);
}

/*************************************************************************/
/*									 */
/*	Print the range of values for attribute Att			 */
/*									 */
/*************************************************************************/

void PrintRange(Attribute Att)
{
    DiscrValue dv;
    Boolean First = true;
    float p;
    if(MaxAttVal[Att]) {    // discrete attribute
        ForEach(dv, 1, MaxAttVal[Att]) {
            if((p = RangeDesc[Att].Probability[dv]) > Fuzz) {
                if(! First) {
                    printf(", ");
                }
                First = false;
                printf("%s", AttValName[Att][dv]);
                if(p < 1-Fuzz) {
                    printf(": %.2f", p);
                }
            }
        }
    }
    else {    // continuous attribute
        printf("%g", RangeDesc[Att].LowerBound);
        if(RangeDesc[Att].UpperBound > RangeDesc[Att].LowerBound + Fuzz) {
            printf(" - %g", RangeDesc[Att].UpperBound);
        }
    }
}

extern char	Delimiter;   // defined in getnames.c
#define	SkipSpace	while((c = getchar()) == ' ' || c == '\t')

/*************************************************************************/
/*									 */
/*	Read a range of values for attribute Att or <cr>		 */
/*									 */
/*************************************************************************/

CError * ReadRange(Attribute Att, Tree T, const char * phrase)
{
    void resetI(void);
    Boolean ReadValue(const char * phrase, String value, Attribute position);
    char c;
    int x = 0;
    CError * errOcc;
    char name[500];
    RangeDesc[Att].Asked = true;
    while((c = phrase[x]) == ' ' || c == '\t') x ++;
    if(c == '\0') {
        errOcc = (CError *) malloc(sizeof(CError));
        errOcc->errorCode = 1;
        errOcc->errorMessage = (char *) calloc(strlen("error: the passed string phrase is empty. ")+1, sizeof(char));
        strcat(errOcc->errorMessage, "error: the passed string phrase is empty. ");
        return errOcc;
    }
    resetI();
    ReadValue(phrase, name, 1);
    if(!strcmp(name, "UNKNOWN")) {
        if(Delimiter != '\0') {
            errOcc = (CError *) malloc(sizeof(CError));
            errOcc->errorCode = 7;
            errOcc->errorMessage = (char *) calloc(strlen("error: illegal value for attribute . ")+strlen(AttName[Att])+1, sizeof(char));
            strcat(errOcc->errorMessage, "error: illegal value for attribute ");
            strcat(errOcc->errorMessage, AttName[Att]);
            strcat(errOcc->errorMessage, ". ");
            return errOcc;
        }
        else RangeDesc[Att].Known = false;
        return NULL;
    }
    RangeDesc[Att].Known = true;
    if(MaxAttVal[Att]) {
        return errOcc = ReadDiscr(Att, T, phrase);
    }
    else {
        return errOcc = ReadContin(Att, T, phrase);
    }
}

/*************************************************************************/
/*									 */
/*	Read a discrete attribute value or range			 */
/*									 */
/*************************************************************************/

CError * ReadDiscr(Attribute Att, Tree T, const char * phrase)
{
    int Which(String Val, String List[], short First, short Last);
    void resetI(void);
    Boolean ReadValue(const char * phrase, String value, Attribute position);
    char Name[500];
    DiscrValue dv, PNo;
    float P, PSum;
    CError * errOcc;
    ForEach(dv, 1, MaxAttVal[Att]) {
        RangeDesc[Att].Probability[dv] = 0.0;
    }
    do {
        resetI();
        ReadValue(phrase, Name, 1);
        Verbosity(1) printf(":%s:\n", Name);   // name readed by ReadValue
        dv = Which(Name, AttValName[Att], 1, MaxAttVal[Att]);
        if(!dv) {
            errOcc = (CError *) malloc(sizeof(CError));
            errOcc->errorCode = 7;
            errOcc->errorMessage = (char *) calloc(strlen("error: value name  for attribute  is illegal. ")+strlen(Name)+strlen(AttName[Att])+1, sizeof(char));
            strcat(errOcc->errorMessage, "error: value name ");
            strcat(errOcc->errorMessage, Name);
            strcat(errOcc->errorMessage, " for attribute ");
            strcat(errOcc->errorMessage, AttName[Att]);
            strcat(errOcc->errorMessage, " is illegal. ");
            return errOcc;
        }
        if(Delimiter == ':') {
            ReadValue(phrase, Name, 0);
            sscanf(Name, "%f", &P);	// get probability
        }
        else {
            P = 1.0;		//  only one attribute value
        }
        RangeDesc[Att].Probability[dv] = P;
    }
    while(Delimiter == ',');
    // Check that sum of probabilities is not > 1
    PNo = MaxAttVal[Att];
    PSum = 1.0;
    ForEach(dv, 1, MaxAttVal[Att]) {
        if(RangeDesc[Att].Probability[dv] > Fuzz) {
            PSum -= RangeDesc[Att].Probability[dv];
            PNo--;
        }
    }
    if(PSum < 0 || ! PNo && PSum > Fuzz) {
        errOcc = (CError *) malloc(sizeof(CError));
        errOcc->errorCode = 10;
        errOcc->errorMessage = (char *) calloc(strlen("error: probability values must sum to 1. ")+1, sizeof(char));
        strcat(errOcc->errorMessage, "error: probability values must sum to 1. ");
        return errOcc;
    }
    //  Distribute the remaining probability equally among the unspecified attribute values
    PSum /= PNo;
    ForEach(dv, 1, MaxAttVal[Att]) {
        if(RangeDesc[Att].Probability[dv] < Fuzz) {
                RangeDesc[Att].Probability[dv] = PSum;
        }
    }
    return NULL;
}

/*************************************************************************/
/*									 */
/*	Read a continuous attribute value or range			 */
/*									 */
/*************************************************************************/

CError * ReadContin(Attribute Att, Tree T, const char * phrase)
{
    Boolean ReadValue(const char * phrase, String value, Attribute position);
    void resetI(void);
    char Name[500];
    int ret;
    CError * errOcc;
    resetI();
    ReadValue(phrase, Name, 1);
    ret = sscanf(Name, "%f", &RangeDesc[Att].LowerBound);
    if(ret != 1) {
        errOcc = (CError *) malloc(sizeof(CError));
        errOcc->errorCode = 7;
        errOcc->errorMessage = (char *) calloc(strlen("error:  is an illegal value for attribute  . ")+strlen(AttName[Att])+strlen(Name)+1, sizeof(char));
        strcat(errOcc->errorMessage, "error: ");
        strcat(errOcc->errorMessage, Name);
        strcat(errOcc->errorMessage, " is an illegal value for attribute ");
        strcat(errOcc->errorMessage, AttName[Att]);
        strcat(errOcc->errorMessage, ". ");
        return errOcc;
    }
    Verbosity(1) printf("%f\n", RangeDesc[Att].LowerBound);
    if(Delimiter == ',') {
    	ReadValue(phrase, Name, 0);
    	ret = sscanf(Name, "%f", &RangeDesc[Att].UpperBound);
    	if(ret != 1) {
            errOcc = (CError *) malloc(sizeof(CError));
            errOcc->errorCode = 7;
            errOcc->errorMessage = (char *) calloc(strlen("error:  is an illegal upper bound value for attribute  . ")+strlen(AttName[Att])+strlen(Name)+1, sizeof(char));
            strcat(errOcc->errorMessage, "error: ");
            strcat(errOcc->errorMessage, Name);
            strcat(errOcc->errorMessage, " is an illegal upper bound value for attribute ");
            strcat(errOcc->errorMessage, AttName[Att]);
            strcat(errOcc->errorMessage, ". ");
            return errOcc;
    	}
    	Verbosity(1) printf("%f\n", RangeDesc[Att].UpperBound);
    }
    else {
        RangeDesc[Att].UpperBound = RangeDesc[Att].LowerBound;
    }
    return NULL;
}

/*************************************************************************/
/*									 */
/*	Skip to the end of the line of input				 */
/*									 */
/*************************************************************************/

void SkipLine(char c)
{
    while(c != '\n') c = getchar();
}

/*************************************************************************/
/*									 */
/*		Clear the range description				 */
/*									 */
/*************************************************************************/

void Clear(void)
{
    Attribute Att;
    ForEach(Att, 0, MaxAtt) {
        RangeDesc[Att].Known = false;
    }
}
