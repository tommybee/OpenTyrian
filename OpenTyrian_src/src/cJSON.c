/*
  Copyright (c) 2009 Dave Gamble

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

// cJSON
// JSON parser in C.

#include "mingw_fixes.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include "cJSON.h"

static void *(*cJSON_malloc)(size_t sz) = malloc;
static void *(*cJSON_realloc)(void *ptr, size_t sz) = realloc;
static void (*cJSON_free)(void *ptr) = free;

static char* cJSON_strdup(const char* str)
{
      size_t len;
      char* copy;

      len = strlen(str) + 1;
      if (!(copy = (char*)cJSON_malloc(len))) return 0;
      memcpy(copy,str,len);
      return copy;
}

void cJSON_InitHooks(cJSON_Hooks* hooks)
{
    if (!hooks) { /* Reset hooks */
        cJSON_malloc = malloc;
        cJSON_realloc = realloc;
        cJSON_free = free;
        return;
    }

	cJSON_malloc = (hooks->malloc_fn)?hooks->malloc_fn:malloc;
	cJSON_realloc= (hooks->realloc_fn)?hooks->realloc_fn:realloc;
	cJSON_free	 = (hooks->free_fn)?hooks->free_fn:free;
}

// Internal constructor.
static cJSON *cJSON_New_Item()
{
	cJSON* node = (cJSON*)cJSON_malloc(sizeof(cJSON));
	if (node) memset(node,0,sizeof(cJSON));
	return node;
}

// Delete a cJSON structure.
void cJSON_Delete(cJSON *c)
{
	cJSON *next;
	while (c)
	{
		next=c->next;
		if (c->child) cJSON_Delete(c->child);
		if (c->valuestring) cJSON_free(c->valuestring);
		if (c->string) cJSON_free(c->string);
		cJSON_free(c);
		c=next;
	}
}

// Parse the input text to generate a number, and populate the result into item.
static const char *parse_number(cJSON *item,const char *num)
{
	double n=0,sign=1,scale=0;int subscale=0,signsubscale=1;

	// Could use sscanf for this?
	if (*num=='-') sign=-1,num++;	// Has sign?
	if (*num=='0') num++;			// is zero
	if (*num>='1' && *num<='9')	do	n=(n*10.0)+(*num++ -'0');	while (*num>='0' && *num<='9');	// Number?
	if (*num=='.') {num++;		do	n=(n*10.0)+(*num++ -'0'),scale--; while (*num>='0' && *num<='9');}	// Fractional part?
	if (*num=='e' || *num=='E')		// Exponent?
	{	num++;if (*num=='+') num++;	else if (*num=='-') signsubscale=-1,num++;		// With sign?
		while (*num>='0' && *num<='9') subscale=(subscale*10)+(*num++ - '0');	// Number?
	}

	n=sign*n*pow(10.0,(scale+subscale*signsubscale));	// number = +/- number.fraction * 10^+/- exponent
	
	item->valuedouble=n;
	item->valueint=(int)n;
	item->type=cJSON_Number;
	return num;
}

// Render the number nicely from the given item into a string.
static char *print_number(cJSON *item)
{
	char *str;
	double d=item->valuedouble;
	if (fabs(((double)item->valueint)-d)<=DBL_EPSILON)
	{
		str=(char*)cJSON_malloc(21);	// 2^64+1 can be represented in 21 chars.
		sprintf(str,"%d",item->valueint);
	}
	else
	{
		str=(char*)cJSON_malloc(64);	// This is a nice tradeoff.
		if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)	sprintf(str,"%e",d);
		else									sprintf(str,"%f",d);
	}
	return str;
}

// Parse the input text into an unescaped cstring, and populate item.
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const char *parse_string(cJSON *item,const char *str)
{
	const char *ptr=str+1;char *ptr2;char *out;int len=0;unsigned uc;
	if (*str!='\"') return 0;	// not a string!
	
	while (*ptr!='\"' && *ptr>31 && ++len) if (*ptr++ == '\\') ptr++;	// Skip escaped quotes.
	
	out=(char*)cJSON_malloc(len+1);	// This is how long we need for the string, roughly.
	if (!out) return 0;
	
	ptr=str+1;ptr2=out;
	while (*ptr!='\"' && *ptr>31)
	{
		if (*ptr!='\\') *ptr2++=*ptr++;
		else
		{
			ptr++;
			switch (*ptr)
			{
				case 'b': *ptr2++='\b';	break;
				case 'f': *ptr2++='\f';	break;
				case 'n': *ptr2++='\n';	break;
				case 'r': *ptr2++='\r';	break;
				case 't': *ptr2++='\t';	break;
				case 'u':	 // transcode utf16 to utf8. DOES NOT SUPPORT SURROGATE PAIRS CORRECTLY.
					sscanf(ptr+1,"%4x",&uc);	// get the unicode char.
					len=3;if (uc<0x80) len=1;else if (uc<0x800) len=2;ptr2+=len;
					
					switch (len) {
						case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 1: *--ptr2 =(uc | firstByteMark[len]);
					}
					ptr2+=len;ptr+=4;
					break;
				default:  *ptr2++=*ptr; break;
			}
			ptr++;
		}
	}
	*ptr2=0;
	if (*ptr=='\"') ptr++;
	item->valuestring=out;
	item->type=cJSON_String;
	return ptr;
}

// Render the cstring provided to an escaped version that can be printed.
static char *print_string_ptr(const char *str)
{
	const char *ptr;char *ptr2,*out;int len=0;
	
	ptr=str;while (*ptr && ++len) {if (*ptr<32 || *ptr=='\"' || *ptr=='\\') len++;ptr++;}
	
	out=(char*)cJSON_malloc(len+3);
	ptr2=out;ptr=str;
	*ptr2++='\"';
	while (*ptr)
	{
		if (*ptr>31 && *ptr!='\"' && *ptr!='\\') *ptr2++=*ptr++;
		else
		{
			*ptr2++='\\';
			switch (*ptr++)
			{
				case '\\':	*ptr2++='\\';	break;
				case '\"':	*ptr2++='\"';	break;
				case '\b':	*ptr2++='b';	break;
				case '\f':	*ptr2++='f';	break;
				case '\n':	*ptr2++='n';	break;
				case '\r':	*ptr2++='r';	break;
				case '\t':	*ptr2++='t';	break;
				default: ptr2--;	break;	// eviscerate with prejudice.
			}
		}
	}
	*ptr2++='\"';*ptr2++=0;
	return out;
}
// Invote print_string_ptr (which is useful) on an item.
static char *print_string(cJSON *item)	{return print_string_ptr(item->valuestring);}

// Predeclare these prototypes.
static const char *parse_value(cJSON *item,const char *value);
static char *print_value(cJSON *item,int depth);
static const char *parse_array(cJSON *item,const char *value);
static char *print_array(cJSON *item,int depth);
static const char *parse_object(cJSON *item,const char *value);
static char *print_object(cJSON *item,int depth);

// Utility to jump whitespace and cr/lf
static const char *skip(const char *in)
{
	if (in != NULL)
		while (*in != '\0' && *in <= 32)
			in++;
	return in;
}

// Parse an object - create a new root, and populate.
cJSON *cJSON_Parse(const char *value)
{
	cJSON *c=cJSON_New_Item();
	if (!c) return 0;       /* memory fail */

	if (!parse_value(c,skip(value))) {cJSON_Delete(c);return 0;}
	return c;
}

// Render a cJSON item/entity/structure to text.
char *cJSON_Print(cJSON *item)			{return print_value(item,0);}

// Parser core - when encountering text, process appropriately.
static const char *parse_value(cJSON *item,const char *value)
{
	if (!value)						return 0;	// Fail on null.
	if (!strncmp(value,"null",4))	{ item->type=cJSON_NULL;  return value+4; }
	if (!strncmp(value,"false",5))	{ item->type=cJSON_False; return value+5; }
	if (!strncmp(value,"true",4))	{ item->type=cJSON_True; item->valueint=1;	return value+4; }
	if (*value=='\"')				{ return parse_string(item,value); }
	if (*value=='-' || (*value>='0' && *value<='9'))	{ return parse_number(item,value); }
	if (*value=='[')				{ return parse_array(item,value); }
	if (*value=='{')				{ return parse_object(item,value); }

	return 0;	// failure.
}

// Render a value to text.
static char *print_value(cJSON *item,int depth)
{
	char *out=0;
	switch (item->type)
	{
		case cJSON_NULL:	out=cJSON_strdup("null");	break;
		case cJSON_False:	out=cJSON_strdup("false");break;
		case cJSON_True:	out=cJSON_strdup("true"); break;
		case cJSON_Number:	out=print_number(item);break;
		case cJSON_String:	out=print_string(item);break;
		case cJSON_Array:	out=print_array(item,depth);break;
		case cJSON_Object:	out=print_object(item,depth);break;
	}
	return out;
}

// Build an array from input text.
static const char *parse_array(cJSON *item,const char *value)
{
	cJSON *child;
	if (*value!='[')	return 0;	// not an array!

	item->type=cJSON_Array;
	value=skip(value+1);
	if (*value==']') return value+1;	// empty array.

	item->child=child=cJSON_New_Item();
	if (!item->child) return 0;		 // memory fail
	value=skip(parse_value(child,skip(value)));	// skip any spacing, get the value.
	if (!value) return 0;

	while (*value==',')
	{
		cJSON *new_item;
		if (!(new_item=cJSON_New_Item())) return 0; 	// memory fail
		child->next=new_item;new_item->prev=child;child=new_item;
		value=skip(parse_value(child,skip(value+1)));
		if (!value) return 0;	// memory fail
	}

	if (*value==']') return value+1;	// end of array
	return 0;	// malformed.
}

// Render an array to text
static char *print_array(cJSON *item,int depth)
{
	char *out, *ptr;
	size_t len = 3;  // minimum needed to print empty array
	
	ptr = out = (char*)cJSON_malloc(len);
	
	strcpy(ptr, "[");
	ptr += 1;
	
	cJSON *child = item->child;
	
	while (child)
	{
		char *ret = print_value(child, depth + 1);
		if (!ret)
		{
			cJSON_free(out);
			return NULL;
		}
		size_t ret_len = strlen(ret);
		
		len += ret_len + 2;
		ptr = out = (char*)cJSON_realloc(out, len);
		ptr += strlen(out);
		
		strcpy(ptr, ret);  // strcat(out, ret);
		ptr += ret_len;
		
		cJSON_free(ret);
		
		if (child->next)
		{
			strcpy(ptr, ", ");  // strcat(out, ", ");
			ptr += 2;
		}
		
		child = child->next;
	}
	
	strcpy(ptr, "]");  // strcat(out, "]");
	
	return out;
}

// Build an object from the text.
static const char *parse_object(cJSON *item,const char *value)
{
	cJSON *child;
	if (*value!='{')	return 0;	// not an object!
	
	item->type=cJSON_Object;
	value=skip(value+1);
	if (*value=='}') return value+1;	// empty array.
	
	item->child=child=cJSON_New_Item();
	value=skip(parse_string(child,skip(value)));
	if (!value) return 0;
	child->string=child->valuestring;child->valuestring=0;
	if (*value!=':') return 0;	// fail!
	value=skip(parse_value(child,skip(value+1)));	// skip any spacing, get the value.
	if (!value) return 0;
	
	while (*value==',')
	{
		cJSON *new_item;
		if (!(new_item=cJSON_New_Item()))	return 0; // memory fail
		child->next=new_item;new_item->prev=child;child=new_item;
		value=skip(parse_string(child,skip(value+1)));
		if (!value) return 0;
		child->string=child->valuestring;child->valuestring=0;
		if (*value!=':') return 0;	// fail!
		value=skip(parse_value(child,skip(value+1)));	// skip any spacing, get the value.		
		if (!value) return 0;
	}
	
	if (*value=='}') return value+1;	// end of array
	return 0;	// malformed.	
}

// Render an object to text.
static char *print_object(cJSON *item,int depth)
{
	char *out, *ptr;
	size_t len = 4 + depth;  // minimum needed to print empty object
	
	++depth;
	
	ptr = out = (char*)cJSON_malloc(len);
	
	strcpy(ptr, "{\n");
	ptr += 2;
	
	cJSON *child = item->child;
	
	while (child)
	{
		char *str = print_string_ptr(child->string);
		if (!str)
		{
			cJSON_free(out);
			return NULL;
		}
		size_t str_len = strlen(str);
		
		char *ret = print_value(child, depth);
		if (!ret)
		{
			cJSON_free(str);
			cJSON_free(out);
			return NULL;
		}
		size_t ret_len = strlen(ret);
		
		len += depth + str_len + ret_len + 4;
		out = (char*)cJSON_realloc(out, len);
		ptr = out + strlen(out);
		
		for (int i = 0; i < depth; ++i)
			*(ptr++) = '\t';
		
		strcpy(ptr, str);  // strcat(out, str);
		ptr += str_len;
		
		cJSON_free(str);
		
		strcpy(ptr, ":\t");  // strcat(out, ":\t");
		ptr += 2;
		
		strcpy(ptr, ret);  // strcat(out, ret);
		ptr += ret_len;
		
		cJSON_free(ret);
		
		if (child->next)
		{
			strcpy(ptr, ",\n");  // strcat(out, ",\n");
			ptr += 2;
		}
		else
		{
			strcpy(ptr, "\n");  // strcat(out, "\n");
			ptr += 1;
		}
		
		child = child->next;
	}
	
	--depth;
	
	for (int i = 0; i < depth; ++i)
		*(ptr++) = '\t';
	
	strcpy(ptr, "}");  // strcat(out, "}");
	
	return out;
}

// Get Array size/item / object item.
int    cJSON_GetArraySize(cJSON *array)							{cJSON *c=array->child;int i=0;while(c)i++,c=c->next;return i;}
cJSON *cJSON_GetArrayItem(cJSON *array,int item)				{cJSON *c=array->child;  while (c && item) item--,c=c->next; return c;}
cJSON *cJSON_GetObjectItem(cJSON *object,const char *string)	{cJSON *c=object->child; while (c && strcasecmp(c->string,string)) c=c->next; return c;}

// Utility for array list handling.
static void suffix_object(cJSON *prev,cJSON *item) {prev->next=item;item->prev=prev;}

// Add item to array/object.
void   cJSON_AddItemToArray(cJSON *array, cJSON *item)						{cJSON *c=array->child;if (!c) {array->child=item;} else {while (c && c->next) c=c->next; suffix_object(c,item);}}
void   cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item)	{if (item->string) cJSON_free(item->string);item->string=cJSON_strdup(string);cJSON_AddItemToArray(object,item);}

// Create basic types:
cJSON *cJSON_CreateNull()						{cJSON *item=cJSON_New_Item();item->type=cJSON_NULL;return item;}
cJSON *cJSON_CreateTrue()						{cJSON *item=cJSON_New_Item();item->type=cJSON_True;return item;}
cJSON *cJSON_CreateFalse()						{cJSON *item=cJSON_New_Item();item->type=cJSON_False;return item;}
cJSON *cJSON_CreateNumber(double num)			{cJSON *item=cJSON_New_Item();item->type=cJSON_Number;item->valuedouble=num;item->valueint=(int)num;return item;}
cJSON *cJSON_CreateString(const char *string)	{cJSON *item=cJSON_New_Item();item->type=cJSON_String;item->valuestring=cJSON_strdup(string);return item;}
cJSON *cJSON_CreateArray()						{cJSON *item=cJSON_New_Item();item->type=cJSON_Array;return item;}
cJSON *cJSON_CreateObject()						{cJSON *item=cJSON_New_Item();item->type=cJSON_Object;return item;}

// Create Arrays:
cJSON *cJSON_CreateIntArray(int *numbers,int count)				{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
cJSON *cJSON_CreateFloatArray(float *numbers,int count)			{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
cJSON *cJSON_CreateDoubleArray(double *numbers,int count)		{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
cJSON *cJSON_CreateStringArray(const char **strings,int count)	{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;i<count;i++){n=cJSON_CreateString(strings[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}

// additions to the cJSON library

cJSON *cJSON_OverwriteObjectItem( cJSON *object, const char *string, int type )
{
	cJSON *child = cJSON_GetObjectItem(object, string);
	if (child == NULL)
		cJSON_AddItemToObject(object, string, child = cJSON_CreateNull());
	
	if (child->type != type)
	{
		cJSON_Delete(child->child);
		child->child = NULL;
		
		child->type = type;
	}
	
	return child;
}

void cJSON_SetString( cJSON *object, const char *string )
{
	cJSON_Delete(object->child);
	object->child = NULL;
	
	object->type = cJSON_String;
	
	free(object->valuestring);
	object->valuestring = strdup(string);
}

void cJSON_SetInteger( cJSON *object, int value )
{
	cJSON_Delete(object->child);
	object->child = NULL;
	
	object->type = cJSON_Number;
	
	object->valuedouble = object->valueint = value;
}

void cJSON_ClearArray( cJSON *array )
{
	cJSON_Delete(array->child);
	array->child = NULL;
}
