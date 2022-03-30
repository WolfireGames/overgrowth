//-----------------------------------------------------------------------------
//           Name: textfile.cpp
//      Developer: Wolfire Games LLC
//    Description: 
//        License: Read below
//-----------------------------------------------------------------------------
// textfile.cpp
//
// simple reading and writing for text files
//
// www.lighthouse3d.com
//
// You may use these functions freely.
// they are provided as is, and no warranties, either implicit,
// or explicit are given
//////////////////////////////////////////////////////////////////////

#include <Internal/error.h>
#include <Compat/fileio.h>
#include <Compat/compat.h>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <malloc.h>
#include <io.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

void textFileRead(const std::string &filename, std::string *dst) {
    FILE *fp;
    if (filename.c_str() != NULL) {
        fp = my_fopen(filename.c_str(),"rb");   
        if(!fp){
            FatalError("Error", "Could not open file txt \"%s\"", filename.c_str());
        }
        fseek(fp, 0, SEEK_END);
        int count = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (fp != NULL) {
            dst->clear();
            if (count > 0) {
                dst->resize(count);
                fread(&dst->at(0),sizeof(char),count,fp);
            }
            fclose(fp);
        }
    }
    // Remove trailing '\0' characters, they mess up include file expansion
    for(int i=(int)dst->length()-1; i>=0; --i){
        if(dst->at(i) != '\0'){
            dst->resize(i+1);
            break;
        }
    }
}

int textFileWrite(char *fn, char *s) {
    FILE *fp;
    int status = 0;

    if (fn != NULL) {
        fp = my_fopen(fn,"w");
        if (fp != NULL) {            
            if (fwrite(s,sizeof(char),strlen(s),fp) == strlen(s))
                status = 1;
            fclose(fp);
        }
    }
    return(status);
}







