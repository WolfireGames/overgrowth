// This code is in the public domain -- castano@gmail.com

#ifndef NV_CORE_FILESYSTEM_H
#define NV_CORE_FILESYSTEM_H


namespace nv
{

    namespace FileSystem
    {

        bool exists(const char * path);
        bool createDirectory(const char * path);

    } // FileSystem namespace

} // nv namespace


#endif // NV_CORE_FILESYSTEM_H
