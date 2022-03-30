#ifndef _TREX_H_
#define _TREX_H_
/***************************************************************
	T-Rex a tiny regular expression library

	Copyright (C) 2003-2006 Alberto Demichelis

	This software is provided 'as-is', without any express 
	or implied warranty. In no event will the authors be held 
	liable for any damages arising from the use of this software.

	Permission is granted to anyone to use this software for 
	any purpose, including commercial applications, and to alter
	it and redistribute it freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented;
		you must not claim that you wrote the original software.
		If you use this software in a product, an acknowledgment
		in the product documentation would be appreciated but
		is not required.

		2. Altered source versions must be plainly marked as such,
		and must not be misrepresented as being the original software.

		3. This notice may not be removed or altered from any
		source distribution.

***************************************************************

===version 1.3

-fixed a bug for GCC users(thx Brendan)



===version 1.2

-added word boundary match \b and \B

-added vertical tab escape \v

-\w now also matches '_' (underscore)

-fixed greediness for * and +



===version 1.1 , April 1, 2004

-fixed some minor bug

-added predefined character classes(\w,\W,\s,\S etc...)



===version 1.0 , February 23, 2004

-first public realase

****************************************************************

T-REX 1.3 http://tiny-rex.sourceforge.net
----------------------------------------------------------------------
	T-Rex a tiny regular expression library

	Copyright (C) 2003-2006 Alberto Demichelis

	This software is provided 'as-is', without any express 
	or implied warranty. In no event will the authors be held 
	liable for any damages arising from the use of this software.

	Permission is granted to anyone to use this software for 
	any purpose, including commercial applications, and to alter
	it and redistribute it freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented;
		you must not claim that you wrote the original software.
		If you use this software in a product, an acknowledgment
		in the product documentation would be appreciated but
		is not required.

		2. Altered source versions must be plainly marked as such,
		and must not be misrepresented as being the original software.

		3. This notice may not be removed or altered from any
		source distribution.
		
----------------------------------------------------------------------
TRex implements the following expressions

\	Quote the next metacharacter
^	Match the beginning of the string
.	Match any character
$	Match the end of the string
|	Alternation
()	Grouping (creates a capture)
[]	Character class  

==GREEDY CLOSURES==
*	   Match 0 or more times
+	   Match 1 or more times
?	   Match 1 or 0 times
{n}    Match exactly n times
{n,}   Match at least n times
{n,m}  Match at least n but not more than m times  

==ESCAPE CHARACTERS==
\t		tab                   (HT, TAB)
\n		newline               (LF, NL)
\r		return                (CR)
\f		form feed             (FF)

==PREDEFINED CLASSES==
\l		lowercase next char
\u		uppercase next char
\a		letters
\A		non letters
\w		alphanimeric [0-9a-zA-Z]
\W		non alphanimeric
\s		space
\S		non space
\d		digits
\D		non nondigits
\x		exadecimal digits
\X		non exadecimal digits
\c		control charactrs
\C		non control charactrs
\p		punctation
\P		non punctation
\b		word boundary
\B		non word boundary

----------------------------------------------------------------------
API DOC
----------------------------------------------------------------------
TRex *trex_compile(const TRexChar *pattern,const TRexChar **error);

compiles an expression and returns a pointer to the compiled version.
in case of failure returns NULL.The returned object has to be deleted
through the function trex_free().

pattern
	a pointer to a zero terminated string containing the pattern that 
	has to be compiled.
error
	apointer to a string pointer that will be set with an error string
	in case of failure.
	
----------------------------------------------------------------------
void trex_free(TRex *exp)

deletes a expression structure created with trex_compile()

exp
	the expression structure that has to be deleted

----------------------------------------------------------------------
TRexBool trex_match(TRex* exp,const TRexChar* text)

returns TRex_True if the string specified in the parameter text is an
exact match of the expression, otherwise returns TRex_False.

exp
	the compiled expression
text
	the string that has to be tested
	
----------------------------------------------------------------------
TRexBool trex_search(TRex* exp,const TRexChar* text, const TRexChar** out_begin, const TRexChar** out_end)

searches the first match of the expressin in the string specified in the parameter text.
if the match is found returns TRex_True and the sets out_begin to the beginning of the
match and out_end at the end of the match; otherwise returns TRex_False.

exp
	the compiled expression
text
	the string that has to be tested
out_begin
	a pointer to a string pointer that will be set with the beginning of the match
out_end
	a pointer to a string pointer that will be set with the end of the match

----------------------------------------------------------------------
TREX_API TRexBool trex_searchrange(TRex* exp,const TRexChar* text_begin,const TRexChar* text_end,const TRexChar** out_begin, const TRexChar** out_end)

searches the first match of the expressin in the string delimited 
by the parameter text_begin and text_end.
if the match is found returns TRex_True and the sets out_begin to the beginning of the
match and out_end at the end of the match; otherwise returns TRex_False.

exp
	the compiled expression
text_begin
	a pointer to the beginnning of the string that has to be tested
text_end
	a pointer to the end of the string that has to be tested
out_begin
	a pointer to a string pointer that will be set with the beginning of the match
out_end
	a pointer to a string pointer that will be set with the end of the match
	
----------------------------------------------------------------------
int trex_getsubexpcount(TRex* exp)

returns the number of sub expressions matched by the expression

exp
	the compiled expression

---------------------------------------------------------------------
TRexBool trex_getsubexp(TRex* exp, int n, TRexMatch *submatch)

retrieve the begin and and pointer to the length of the sub expression indexed
by n. The result is passed trhough the struct TRexMatch:

typedef struct {
	const TRexChar *begin;
	int len;
} TRexMatch;

the function returns TRex_True if n is valid index otherwise TRex_False.

exp
	the compiled expression
n
	the index of the submatch
submatch
	a pointer to structure that will store the result
	
this function works also after a match operation has been performend.
	


*/

#define TRexChar char
#define MAX_CHAR 0xFF
#define _TREXC(c) (c) 
#define trex_strlen strlen
#define trex_printf printf

#ifndef TREX_API
#define TREX_API extern
#endif

#define TRex_True 1
#define TRex_False 0

typedef unsigned int TRexBool;
typedef struct TRex TRex;

typedef struct {
	const TRexChar *begin;
	int len;
} TRexMatch;

TREX_API TRex *trex_compile(const TRexChar *pattern,const TRexChar **error);
TREX_API void trex_free(TRex *exp);
TREX_API TRexBool trex_match(TRex* exp,const TRexChar* text);
TREX_API TRexBool trex_search(TRex* exp,const TRexChar* text, const TRexChar** out_begin, const TRexChar** out_end);
TREX_API TRexBool trex_searchrange(TRex* exp,const TRexChar* text_begin,const TRexChar* text_end,const TRexChar** out_begin, const TRexChar** out_end);
TREX_API int trex_getsubexpcount(TRex* exp);
TREX_API TRexBool trex_getsubexp(TRex* exp, int n, TRexMatch *subexp);

struct TRexParseException{TRexParseException(const TRexChar *c):desc(c){}const TRexChar *desc;};

class TRexpp {
public:
	TRexpp() { _exp = (TRex *)0; }
	~TRexpp() { CleanUp(); }
	// compiles a regular expression
	void Compile(const TRexChar *pattern) { 
		const TRexChar *error;
		CleanUp();
		if(!(_exp = trex_compile(pattern,&error)))
			throw TRexParseException(error);
	}
	// return true if the given text match the expression
	bool Match(const TRexChar* text) { 
		return _exp?(trex_match(_exp,text) != 0):false; 
	}
	// Searches for the first match of the expression in a zero terminated string
	bool Search(const TRexChar* text, const TRexChar** out_begin, const TRexChar** out_end) { 
		return _exp?(trex_search(_exp,text,out_begin,out_end) != 0):false; 
	}
	// Searches for the first match of the expression in a string sarting at text_begin and ending at text_end
	bool SearchRange(const TRexChar* text_begin,const TRexChar* text_end,const TRexChar** out_begin, const TRexChar** out_end) { 
		return _exp?(trex_searchrange(_exp,text_begin,text_end,out_begin,out_end) != 0):false; 
	}
	bool GetSubExp(int n, const TRexChar** out_begin, int *out_len)
	{
		TRexMatch match;
		TRexBool res = _exp?(trex_getsubexp(_exp,n,&match)):TRex_False; 
		if(res) {
			*out_begin = match.begin;
			*out_len = match.len;
			return true;
		}
		return false;
	}
	int GetSubExpCount() { return _exp?trex_getsubexpcount(_exp):0; }
private:
	void CleanUp() { if(_exp) trex_free(_exp); _exp = (TRex *)0; }
	TRex *_exp;
};

#endif
