#include "strings.h"

#include <Logging/logdata.h>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using std::string;
using std::wstring;
using std::vector;
using std::pair;
using std::endl;

bool endswith(const char* str, const char* ending) {
    size_t str_len = strlen(str);
    size_t end_len = strlen(ending);

    if( str_len >= end_len ) {
        for( unsigned i = 0; i < end_len; i++ ) {
            int str_len_index = i + str_len - end_len;
            if( str[str_len_index] != ending[i] ) {
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

bool beginswith(const char* str, const char* begin) {
    size_t str_len = strlen(str);
    size_t begin_len = strlen(begin);

    if( str_len >= begin_len   ) {
        for( unsigned i = 0; i < begin_len; i++ ) {
            if( str[i] != begin[i] ) {
                return false;
            }
        }
        return true;
    } else {
        return false;
    }

}

bool pstrmtch( const char* first, const char* second, size_t count) {
    if( strlen(first) >= count && strlen(second) >= count ) {
        for( size_t i = 0; i < count; i++ ) {
            if( first[i] != second[i] ) {
                return false; 
            }
        }
        return true;
    } else {
        return false;
    }
}

bool strcont( const char* string, const char* substring ) {
    size_t sub_len = strlen(substring);
    size_t str_len = strlen(string);
    for( size_t i = 0; i < str_len; i++ ) {
        bool match = true;
        if( (str_len - i) >= sub_len ) {
            for( size_t k = 0; k < sub_len; k++ ) {
               if( string[i] != substring[k] ) {
                    match = false; 
                    break;
               }  
            }  
            if( match ) { 
                return true; 
            }
        } 
    }
    return false;
}

#ifdef WIN32
wstring fromUTF8( const string& path_utf8 ) {
    int size = MultiByteToWideChar(CP_UTF8, 0, path_utf8.c_str(), -1, NULL, 0);
    wstring path_utf16;
    path_utf16.resize(size);
    MultiByteToWideChar(CP_UTF8, 0, path_utf8.c_str(), -1, &path_utf16[0], size);
    return path_utf16;
}

string fromUTF16( const wstring& path_utf16 ){
	int size = WideCharToMultiByte(CP_UTF8, 0, path_utf16.c_str(), -1, NULL, 0, NULL, NULL );
	string path_utf8;
	path_utf8.resize(size);
	WideCharToMultiByte(CP_UTF8, 0, path_utf16.c_str(), -1, &path_utf8[0], size, NULL, NULL);
	return path_utf8;
}
#endif

int FindStringInArray( const char** values, size_t values_size, const char* q )
{
    for( unsigned i = 0; i < values_size; i++ )
    {
        if( strmtch( values[i], q ) )
        {
            return i;
        }
    }
    return -1;
}

int FindStringInArray( const vector<pair< const char*, const char* > >& values, const char* q )
{
    for( unsigned i = 0; i < values.size(); i++ )
    {
        if( strmtch( values[i].first, q ) )
        {
            return i;
        }
    }
    return -1;
}

int FindStringInArray( const vector< const char* >& values, const char* q )
{
    for( unsigned i = 0; i < values.size(); i++ )
    {
        if( strmtch( values[i], q ) )
        {
            return i;
        }
    }
    return -1;
}

void UTF8InPlaceLower( char* arr ) {
    for( unsigned i = 0; i < strlen( arr ); i++ ) {
        //A to Z in unicode, this is all the letters in the world, right?
        if( arr[i] >= 0x41 && arr[i] <= 0x5A ) {
            arr[i] = arr[i]+0x20;
        } else {
            arr[i] = arr[i];
        }
    }
}

int UTF8ToUpper( char* out, size_t outsize, const char* in )
{
    //We should be using the ICU lib for this, but good god, it's massive and difficult, 
    //so we just start of with using the first 128 characters which have simple rules. (ASCII).
    //The version in ICU is much more context sensitive, handles some really interesting cases.
    //http://site.icu-project.org/download
    memset(out,'\0',outsize);
    for( unsigned i = 0; i < strlen( in ) && i < outsize-1; i++ )
    {
        //A to Z in unicode, this is all the letters in the world, right?
        if( in[i] >= 0x61 && in[i] <= 0x7a )
        {
            out[i] = in[i]-0x20;
        }
        else
        {
            out[i] = in[i];
        }
    }
    return strlen(in)+1; 
    //Return how many bytes we wrote to the output string, inluding the terminating.
    //this is a placeholder for a smart solution in the future, where expansion may occur;
}

string UTF8ToLower(const string& in) {
    string out;
    out.resize(in.size());
    for( unsigned i = 0; i < out.size(); i++ )
    {
        //A to Z in unicode, this is all the letters in the world, right?
        if( in[i] >= 0x41 && in[i] <= 0x5A )
        {
            out[i] = in[i]+0x20;
        }
        else
        {
            out[i] = in[i];
        }
    }
    return out;
}

int UTF8ToLower( char* out, size_t outsize, const char* in ) {
    //We should be using the ICU lib for this, but good god, it's massive and difficult, 
    //so we just start of with using the first 128 characters which have simple rules. (ASCII).
    //The version in ICU is much more context sensitive, handles some really interesting cases.
    //http://site.icu-project.org/download
    memset(out,'\0',outsize);
    for( unsigned i = 0; i < strlen( in ) && i < outsize-1; i++ )
    {
        //A to Z in unicode, this is all the letters in the world, right?
        if( in[i] >= 0x41 && in[i] <= 0x5A )
        {
            out[i] = in[i]+0x20;
        }
        else
        {
            out[i] = in[i];
        }
    }
    return strlen(in)+1; 
    //Return how many bytes we wrote to the output string, inluding the terminating.
    //this is a placeholder for a smart solution in the future, where expansion may occur;
}

bool hasBeginning( const string &fullString, const string &beginning ) {
    if( fullString.length() >= beginning.length() ) {
        if( fullString.substr(0,beginning.length()) == beginning ) {
            return true;
        }
    }
    return false;
}

bool hasEnding (string const &fullString, string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool hasAnyOfEndings( const string &fullString, const vector<string> &endings)
{
    for( vector<string>::const_iterator sit = endings.begin();
         sit != endings.end();
         sit++ )
    {
        if( hasEnding( fullString, *sit ) )
        {
            return true;
        }
    }
    return false;
}

int CountCharsInString(const char* str, char the_char) {
    int count = 0;
    int str_length = (int)strlen(str);
    for(int i=0; i<str_length; ++i){
        if(str[i] == the_char){
            ++count;
        }
    }
    return count;
}

int FindNthCharFromBack(const char* str, char the_char, int n){
    int count = 0;
    int str_length = (int)strlen(str);
    for(int i= str_length-1; i>=0; --i){
        if(str[i] == the_char){
            ++count;
            if(count == n){
                return i;
            }
        }
    }
    return -1;
}

int saysTrue(const char* str) {
    if(str) {
        if(strmtch(str, "true")) {
            return SAYS_TRUE;
        } else if(strmtch(str, "false")) {
            return SAYS_FALSE;
        } else {
            return SAYS_TRUE_NO_MATCH;
        }
    } else {
        return SAYS_TRUE_NULL_INPUT;
    }
}

const char* nullAsEmpty( const char* str ) {
    if( str ) { 
        return str;
    } else {
        return "";
    }
}

uint32_t GetLengthInBytesForNCodepoints( const string& utf8in, uint32_t codepoint_index ) {
    uint32_t codepoints = 0;

    if( codepoint_index == 0 )
        return 0;
     
    for( unsigned i = 0; i < utf8in.size(); i++ ) {
        uint8_t c = utf8in[i];

        if( ( c & 0x80) == 0 ) {
            codepoints++;
        } else if( (c & 0xC0) == 0xC0 ) {  
            codepoints++; 
            if( (c & 0x20) == 0)  {
                i += 1; 
            } else if( (c & 0x10) == 0 ) {
                i += 2;
            } else if( (c & 0x08) == 0 ) {
                i += 3; 
            }
        } else  {
            LOGE << "Malformed utf-8 string" << endl;
        }

        if( i >= utf8in.size() ) {
            LOGE << "Malformed utf-8 string, codepoint indication doesn't match offset" << endl;
        }
    
        if( codepoints == codepoint_index ) {
            return i+1;
        }
    }
    return utf8in.size();
}

uint32_t GetCodepointCount( const string &utf8in ) {
    uint32_t codepoints = 0;
    for( unsigned i = 0; i < utf8in.size(); i++ ) {
        uint8_t c = utf8in[i];
        if( ( c & 0x80) == 0 ) {
            codepoints++;
        } else if( (c & 0xC0) == 0xC0 ) { 
            codepoints++; 
            if( (c & 0x20) == 0)  {
                i += 1; 
            } else if( (c & 0x10) == 0 ) {
                i += 2;
            } else if( (c & 0x08) == 0 ) {
                i += 3; 
            }
        } else  {
            LOGE << "Malformed utf-8 string" << endl;
        }
    }
    return codepoints;
}

string RemoveFileEnding(string in) {
    size_t last_point = in.size();

    for( int i = in.size()-1; i >= 0; i-- ) {
        if( in[i] == '.' ) {
            last_point = i;
        }
        if( in[i] == '/' ) {
            return in.substr(0,last_point); 
        }
    }

    if( last_point > 0 ) {
        return in.substr(0,last_point);
    } else {
        return in;
    }
}

