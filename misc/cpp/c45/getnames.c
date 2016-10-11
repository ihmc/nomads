/*************************************************************************/
/*									 */
/*	Get names of classes, attributes and attribute values		 */
/*	-----------------------------------------------------		 */
/*									 */
/*************************************************************************/

#include "defns.h"
#include "types.h"
#include "extern.h"
#include<stdlib.h>
#include<string.h>
#if defined (UNIX)
    #include<strings.h>
#endif
#include<stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#define  Space(s)	(s == ' ' || s == '\n' || s == '\t')
#define  SkipComment	while ( ( c = getc(f) ) != '\n' )

char Delimiter;
int i;

// functions declaration
Boolean ReadValue(const char * phrase, String value, Attribute position);
CError * GetNamesFromString(const char * names);
void resetI(void);
void incrementI(void);
int Which(String Val, String List[], short First, short Last);
String CopyString(String x);

/*************************************************************************/
/*									 */
/*  Read a name from string phrase into string s, setting Delimiter.		 */
/*									 */
/*  - Embedded periods are permitted, but periods followed by space	 */
/*    characters act as delimiters.					 */
/*  - Embedded spaces are permitted, but multiple spaces are replaced	 */
/*    by a single space.						 */
/*  - Any character can be escaped by '\'.				 */
/*  - The remainder of a line following '|' is ignored.			 */
/*									 */
/*************************************************************************/

Boolean ReadValue(const char * phrase, String value, Attribute position)
{
    register char * Sp = value;
    register int c;
    int x;
    //  Skip to first non-space character
    while((c = phrase[i] ) == '|' || Space(c)) {
		if(c == '|') {
    		i ++;
    		while(( c = phrase[i] ) != '\n' );
   		}
   		i ++;
    }
    //  Return false if no names to read
    //if ( phrase == NULL )
    //{
	//	Delimiter = EOF;
	//	return false;
    //}
    //  Read in characters up to the next delimiter
    x = 0;
    c = phrase[i];
    if(c == '.') return false;  // read a point with nothing before, this is an error in the sintax
    while(x < position) {
    	while(c != ',' && c != '\0') {
    		i ++;
    		c = phrase[i];
    	}
    	if(c == ',') {
    		x ++;
    		i ++;
    		c = phrase[i];
    	}
    	else {
    		Delimiter = '\0';
    		return false;
    	}
    }
    while( c != ':' && c != ',' && c != '\n' && c != '|' && c != '\0') {
    	i ++;
		if(c == '.') {
			if((phrase[i] ) == '|' || Space(phrase[i]) || (phrase[i] == '\0')) break;
		    *Sp++ = '.';
		}
		if(c == '\\') {
			i ++;
	    	c = phrase[i];
		}
		*Sp++ = c;
		if(c == ' ') {
	    	while((c = phrase[i]) == ' ') i ++;
		}
		else {
	  		c = phrase[i];
		}
    }
    if(c == '\0') {
    	*Sp++ = '\0';
    	Delimiter = c;
    	return true;
    }
    if(c == '|') {
    	i ++;
    	while(( c = phrase[i] ) != '\n' );
    }
    Delimiter = c;
    //  Strip trailing spaces
    while ( Space(*(Sp-1)) ) Sp--;
    *Sp++ = '\0';
    i ++;
    return true;
}

/*************************************************************************/
/*									 */
/*  Read the names of classes, attributes and legal attribute values from string.	 */
/*  On completion, these names are stored in:				 */
/*	ClassName	-  class names					 */
/*	AttName		-  attribute names				 */
/*	AttValName	-  attribute value names			 */
/*  with:								 */
/*	MaxAttVal	-  number of values for each attribute		 */
/*									 */
/*  Other global variables set are:					 */
/*	MaxAtt		-  maximum attribute number			 */
/*	MaxClass	-  maximum class number				 */
/*	MaxDiscrVal	-  maximum discrete values for any attribute	 */
/*									 */
/*  Note:  until the number of attributes is known, the name		 */
/*	   information is assembled in local arrays			 */
/*									 */
/*************************************************************************/

CError * GetNamesFromString(const char * names)
{
    char Buffer[400];
    Boolean returnValue;     // check the return value of ReadValue
    CError * errOcc;		 // error message
    DiscrValue v;
    int AttCeiling=10;       // predict 10 different attributes
    int ClassCeiling = 2;    // the default number of classes is 2
    int ValCeiling;          // predict number of value for each attribute
    //  Get class names from names file
    ClassName = (String *) calloc(ClassCeiling, sizeof(String));
    MaxClass = -1;
    resetI();    // initialize the counter for ReadValue()
    Verbosity(1) printf("\nreading classes from string names\n");
    if(names == NULL) {
    	errOcc = (CError *) malloc(sizeof(CError));
    	errOcc->errorCode = 1;
    	errOcc->errorMessage = "error: the passed string parameter is a NULL string. \0";
    	return errOcc;
    }
    do {
		if(++MaxClass >= ClassCeiling) {
	    	ClassCeiling ++;  // add space for another class
	    	ClassName = (String *) realloc(ClassName, ClassCeiling*sizeof(String));
		}
		returnValue = ReadValue(names, Buffer, 0);
		if(!returnValue) {
			errOcc = (CError *) malloc(sizeof(CError));
			errOcc->errorCode = 2;
			errOcc->errorMessage ="error: unable to read the name of one of the classes. \0";
			return errOcc;
		}
		ClassName[MaxClass] = CopyString(Buffer);
		Verbosity(1) printf("class %d: :%s:\n", MaxClass, ClassName[MaxClass]);
    }
    while(Delimiter == ',');
    //  Get attribute and attribute value names from names
    AttName = (String *) calloc(AttCeiling, sizeof(String));  // attributes names, 10
    MaxAttVal = (DiscrValue *) calloc(AttCeiling, sizeof(DiscrValue)); //number of values for each attribute
    AttValName = (String **) calloc(AttCeiling, sizeof(String *));  // 1 value name for each attribute
    SpecialStatus = (char *) calloc(AttCeiling, sizeof(char));
    MaxAtt = -1;
    Verbosity(1) printf("\nreading attributes and values from string names\n");
    while(ReadValue(names, Buffer, 0)) {
		if ( Delimiter != ':' ) {
			errOcc = (CError *) malloc(sizeof(CError));
			errOcc->errorCode = 3;
			errOcc->errorMessage = (char *) calloc(strlen("error: colon expected after attribute name ")+strlen(Buffer)+3, sizeof(char));
			strcat(errOcc->errorMessage, "error: colon expected after attribute name ");
			strcat(errOcc->errorMessage, Buffer);
			strcat(errOcc->errorMessage, ". ");
			return errOcc;
		}
		if(++MaxAtt >= AttCeiling) {    // correct sintax: "attributeName: value1, value2."
	   		AttCeiling += 5;          // add space for other 5 attributes
	    	AttName = (String *) realloc(AttName, AttCeiling*sizeof(String));
	    	MaxAttVal = (DiscrValue *) realloc(MaxAttVal, AttCeiling*sizeof(DiscrValue));
	    	AttValName = (String **) realloc(AttValName, AttCeiling*sizeof(String *));
	    	SpecialStatus = (char *) realloc(SpecialStatus, AttCeiling);
		}
		AttName[MaxAtt] = CopyString(Buffer);
		Verbosity(1) printf("attribute %d: :%s:\n", MaxAtt, AttName[MaxAtt]);
		SpecialStatus[MaxAtt] = Nil;
		MaxAttVal[MaxAtt] = 0;  // initial value
		ValCeiling = 1;        // allocate space for 1 value for this attribute (AttName[MaxAtt])
		AttValName[MaxAtt] = (String *) calloc(ValCeiling, sizeof(String));
		do { // reading the values for this attribute
	    	if(!(ReadValue(names, Buffer, 0))) {
				errOcc = (CError *) malloc(sizeof(CError));
				errOcc->errorCode = 4;
				errOcc->errorMessage = (char *)calloc(strlen("error: unable to read values of attribute ")+strlen(Buffer)+3, sizeof(char));
				strcat(errOcc->errorMessage, "error: unable to read values of attribute ");
				strcat(errOcc->errorMessage, Buffer);
				strcat(errOcc->errorMessage, ". ");
	    		return errOcc;
	    	}
	    	if(++MaxAttVal[MaxAtt] >= ValCeiling) {
				ValCeiling += 5;  // add space for other 5 values
				AttValName[MaxAtt] = (String *) realloc(AttValName[MaxAtt], ValCeiling*sizeof(String));
	    	}
	    	AttValName[MaxAtt][MaxAttVal[MaxAtt]] = CopyString(Buffer);
	    	Verbosity(1) printf("\tvalue %d: :%s:\n", MaxAttVal[MaxAtt], AttValName[MaxAtt][MaxAttVal[MaxAtt]]);
		}
		while(Delimiter == ',');
		Verbosity(1) printf("\n");
		if(MaxAttVal[MaxAtt] == 1) {
	    	//  Check for special treatment
	    	if(! strcmp(Buffer, "CONTINUOUS")) {}  // no special status in this case
	    	else if(! memcmp(Buffer, "DISCRETE", 8)) {
				SpecialStatus[MaxAtt] = DISCRETE;
				//  Read max values, reserve space and check MaxDiscrVal
				v = atoi(&Buffer[8]);  // take the number after "discrete"
				if(v < 2) {
					errOcc = (CError *) malloc(sizeof(CError));
					errOcc->errorCode = 5;
					errOcc->errorMessage = (char *)calloc(strlen("error: illegal number of discrete values (< 2) for attribute ")+strlen(AttName[MaxAtt])+3, sizeof(char));
					strcat(errOcc->errorMessage, "error: illegal number of discrete values (< 2) for attribute ");
					strcat(errOcc->errorMessage, AttName[MaxAtt]);
					strcat(errOcc->errorMessage, ". ");
		    		return errOcc;
				}
				AttValName[MaxAtt] = (String *) realloc(AttValName[MaxAtt], (v+2)*sizeof(String));
				AttValName[MaxAtt][0] = (char *) v;
				if(v > MaxDiscrVal) MaxDiscrVal = v;
	   		}
	    	else if(! strcmp(Buffer, "IGNORE")) {
				SpecialStatus[MaxAtt] = IGNORE;
	    	}
	    	else {
				errOcc = (CError *) malloc(sizeof(CError));
				errOcc->errorCode = 5;
				errOcc->errorMessage = (char *)calloc(strlen("error: illegal number of discrete values (< 2) for attribute ")+strlen(AttName[MaxAtt])+3, sizeof(char));
				strcat(errOcc->errorMessage, "error: illegal number of discrete values (< 2) for attribute ");
				strcat(errOcc->errorMessage, AttName[MaxAtt]);
				strcat(errOcc->errorMessage, ". ");
		    	return errOcc;
	    	}
	    	MaxAttVal[MaxAtt] = 0;
		}
		else if(MaxAttVal[MaxAtt] > MaxDiscrVal) MaxDiscrVal = MaxAttVal[MaxAtt];
    }
    /*printf("dentro GetNamesFromString in fondo, valori letti:\n");
    printf("MaxClass = :%d:\n", MaxClass);
    ForEach(xc, 0, MaxClass) {
    	printf("ClassName[%d] = :%s:\n", xc, ClassName[xc]);
    }
    printf("MaxAtt = :%d:\n", MaxAtt);
     ForEach(xc, 0, MaxAtt) {
    	int xb;
    	printf("AttName[%d] = :%s:\n", xc, AttName[xc]);
    	if(SpecialStatus[xc] == IGNORE) printf("SpecialStatus[%d] = IGNORE\n", xc);
    	else {
    		if(SpecialStatus[xc] == DISCRETE) printf("SpecialStatus[%d] = DISCRETE\n", xc);
    		else printf("SpecialStatus[%d] = CONTINUOUS\n", xc);
    	}
    	printf("MaxAttVal[%d] = :%d:\n", xc, MaxAttVal[xc]);
    	ForEach(xb, 0, MaxAttVal[xc]) {
    		printf("AttValName[%d] = :%s:\n", xb, AttValName[xc][xb]);
    	}
    }*/
    return NULL;
}

/*************************************************************************/
/*	Reset to zero the value of the counter i			 */
/*************************************************************************/

void resetI(void)
{
	i = 0;
}

/*************************************************************************/
/*	Increment the value of the counter i			 */
/*************************************************************************/

void incrementI(void)
{
	i ++;
}

/*************************************************************************/
/*									 */
/*	Locate value Val in List[First] to List[Last]			 */
/*									 */
/*************************************************************************/

int Which(String Val, String List[], short First, short Last)
{
    short n=First;
    while(n <= Last && strcmp(Val, List[n])) n++;
    return (n <= Last ? n : First-1);
}

/*************************************************************************/
/*									 */
/*	Allocate space then copy string into it				 */
/*									 */
/*************************************************************************/

String CopyString(String x)
{
    char *s;
    s = (char *) calloc(strlen(x)+1, sizeof(char));
    strcpy(s, x);
    return s;
}
