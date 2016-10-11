/*************************************************************************/
/*									 */
/*	Get case descriptions from data file				 */
/*	--------------------------------------				 */
/*									 */
/*************************************************************************/

#include<stdlib.h>
#include<string.h>
#if defined (UNIX)
    #include<strings.h>
#endif
#include "defns.h"
#include "types.h"
#include "extern.h"

#define Inc 2048

extern int i;

// functions
CError * GetDataFromString(const char * data);
Description GetDescriptionFromString(const char* data, CError * * errOcc);

/*************************************************************************/
/*									 */
/*  Read raw case descriptions from string.		 */
/*									 */
/*  On completion, cases are stored in array Item in the form		 */
/*  of Descriptions (i.e. arrays of attribute values), and		 */
/*  MaxItem is set to the number of data items.				 */
/*									 */
/*************************************************************************/

CError * GetDataFromString(const char * data)
{
	void resetI(void);
	Description GetDescriptionFromString(const char * data, CError * * errOcc);
	CError * errOcc;
    ItemNo i = 0, ItemSpace = 0;
    do {
		MaxItem = i;
		// Make sure there is room for another item
		if(i >= ItemSpace) {
	   		if(ItemSpace) {
				ItemSpace += Inc;
				Item = (Description *)realloc(Item, ItemSpace*sizeof(Description));
	    	}
	    	else {
				Item = (Description *)malloc((ItemSpace=Inc)*sizeof(Description));
	    	}
		}
		Item[i] = GetDescriptionFromString(data, &errOcc);
		if(errOcc != NULL) {
			return errOcc;
		}
		//if(Item[i] == Nil) printf("dentro GetDataFromString: Item[%d] ha valore nullo\n", i);
		/*else {
			printf("Item[%d] (riga %d) :\n", i, i);
			int xax;
			ForEach(xax, 0, MaxAtt) {
			if ( SpecialStatus[xax] == IGNORE )
			printf("for attribute %d IGNORE, Description = :%d:\n", xax, Item[i][xax]._discr_val);
			else {
				if ( MaxAttVal[xax] || SpecialStatus[xax] == DISCRETE ) 
				printf("for attribute %d DISCRETE, Description = :%d:\n", xax, Item[i][xax]._discr_val);
				else 
				printf("for attribute %d CONTINUOUS, Description = :%f:\n", xax, Item[i][xax]._cont_val);
				}
			}
			printf("classe di Item[%d] = %d\n", i, Item[i][MaxAtt+1]._discr_val); 
		}*/
    } while ( Item[i] != Nil && ++i );
    MaxItem = i - 1;
    return NULL;
}

/*************************************************************************/
/*									 */
/*  Read a raw case description from string data.				 */
/*									 */
/*  For each attribute, read the attribute value from the string.		 */
/*  If it is a discrete valued attribute, find the associated no.	 */
/*  of this attribute value (if the value is unknown this is 0).	 */
/*									 */
/*  Returns the Description of the case (i.e. the array of		 */
/*  attribute values).							 */
/*									 */
/*************************************************************************/

Description GetDescriptionFromString(const char * data, CError * * errOcc)
{
	Boolean ReadValue(const char * phrase, const char * value, Attribute position);
	String CopyString(String x);
	int Which(String Val, String List[], short First, short Last);
	double strtod();
    Attribute Att;
    char name[500];
    char * endname;
    int Dv;
    float Cv;
    Description Dvec;
    if(ReadValue(data, name, 0)) {
		Dvec = (Description) calloc(MaxAtt+2, sizeof(AttValue));
        ForEach(Att, 0, MaxAtt) {
	    	if(SpecialStatus[Att] == IGNORE) {
				// Skip this value
				DVal(Dvec, Att) = 0;
	    	}
	    	else if(MaxAttVal[Att] || SpecialStatus[Att] == DISCRETE) {
				// Discrete value
	        	if(!( strcmp(name, "?"))) {
		    		Dv = 0;
				}
	        	else {
		    		Dv = Which(name, AttValName[Att], 1, MaxAttVal[Att]);
		    		if(! Dv) {
						if(SpecialStatus[Att] == DISCRETE) {
			    			// Add value to list
			    			Dv = ++MaxAttVal[Att];
			   				if(Dv > (int) AttValName[Att][0]) {
			    				(*errOcc) = (CError *) malloc(sizeof(CError));
								(*errOcc)->errorCode = 6;
								(*errOcc)->errorMessage = (char *)calloc(strlen("error: too many values for attribute  (max number of value is )")+strlen(AttName[Att])+strlen(AttValName[Att][0])+3, sizeof(char));
								strcat((*errOcc)->errorMessage, "error: too many values for attribute ");
								strcat((*errOcc)->errorMessage, AttName[Att]);
								strcat((*errOcc)->errorMessage, " (max number of value is ");
								strcat((*errOcc)->errorMessage, AttValName[Att][0]);
								strcat((*errOcc)->errorMessage, " ). ");
								return NULL;
			    			}
			    			AttValName[Att][Dv] = CopyString(name);
						}
						else {
							(*errOcc) = (CError *) malloc(sizeof(CError));
							(*errOcc)->errorCode = 7;
							(*errOcc)->errorMessage = (char *)calloc(strlen("error: value  for attribute  is illegal")+strlen(AttName[Att])+strlen(name)+3, sizeof(char));
							strcat((*errOcc)->errorMessage, "error: value ");
							strcat((*errOcc)->errorMessage, name);
							strcat((*errOcc)->errorMessage, " for attribute ");
							strcat((*errOcc)->errorMessage, AttName[Att]);
							strcat((*errOcc)->errorMessage, " is illegal. ");
			    			return NULL;
						}
		    		}
	        	}
	        	DVal(Dvec, Att) = Dv;
	    	}
	    	else {
				// Continuous value
	        	if(!( strcmp(name, "?"))) {
		    		Cv = Unknown;
				}
	        	else {
		    		Cv = strtod(name, &endname);
		    		if(endname == name || *endname != '\0') {
		    			(*errOcc) = (CError *) malloc(sizeof(CError));
						(*errOcc)->errorCode = 7;
						(*errOcc)->errorMessage = (char *)calloc(strlen("error: value  for attribute  is illegal")+strlen(AttName[Att])+strlen(name)+3, sizeof(char));
						strcat((*errOcc)->errorMessage, "error: value ");
						strcat((*errOcc)->errorMessage, name);
						strcat((*errOcc)->errorMessage, " for attribute ");
						strcat((*errOcc)->errorMessage, AttName[Att]);
						strcat((*errOcc)->errorMessage, " is illegal. ");
			    		return NULL;
		    		}
				}
				CVal(Dvec, Att) = Cv;
	    	}
	    	ReadValue(data, name, 0);
        }
        if((Dv = Which(name, ClassName, 0, MaxClass)) < 0) {
        	(*errOcc) = (CError *) malloc(sizeof(CError));
			(*errOcc)->errorCode = 8;
			(*errOcc)->errorMessage = (char *)calloc(strlen("error: class name  is illegal")+strlen(name)+3, sizeof(char));
			strcat((*errOcc)->errorMessage, "error: class name ");
			strcat((*errOcc)->errorMessage, name);
			strcat((*errOcc)->errorMessage, " is illegal. ");
			Dv = 0;
			return NULL;
        }
		Class(Dvec) = Dv;
		(*errOcc) = NULL;
		return Dvec;
    }
    else {
    	(*errOcc) = NULL;
		return Nil;
    }
}
