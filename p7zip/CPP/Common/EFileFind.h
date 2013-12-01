//  EFileFind.h
//  This subclass of CFileInfoW is used on OS X for working around sandbox restrictions.
//  For a given file path, CFileInfoW::Find attempts to access its parent directory.
//  Under sandboxing, the access is blocked and Find fails.
//  This minimal version uses lstat directly on the file instead.
//  Note that it has only been tested with ArchiveOpenCallback. Also, unlike CFileInfoW::Find,
//  the argument is expected to a valid path rather than the more flexible wildcard.
//
#ifndef __ETHEREON_FILEFIND_H
#define __ETHEREON_FILEFIND_H
#include <stdio.h>
#include "Windows/FileFind.h"

#ifndef __APPLE__
#define EFileInfoW  CFileInfoW
#else

#include "Common/StringConvert.h"
#include <sys/stat.h>

extern void WINAPI RtlSecondsSince1970ToFileTime( DWORD Seconds, FILETIME * ft );

namespace NWindows {
namespace NFile {
namespace NFind {

class EFileInfoW: public CFileInfoW
{
private:

  bool FindUsingStat(LPCWSTR fp)
  {
    AString afp = UnicodeStringToMultiByte(fp);
    const char* path = afp;
    if((path[0] == 'c') && (path[1] == ':'))
      path += 2; //Strip Windows c: prefix added by 7zip.
    struct stat s;
    if(lstat(path, &s)!=0)
      return false;
    this->Attrib = S_ISDIR(s.st_mode)?FILE_ATTRIBUTE_DIRECTORY:FILE_ATTRIBUTE_ARCHIVE;
    if (!(s.st_mode & S_IWUSR))
      this->Attrib |= FILE_ATTRIBUTE_READONLY;
    this->Attrib |= FILE_ATTRIBUTE_UNIX_EXTENSION + ((s.st_mode & 0xFFFF) << 16);
    RtlSecondsSince1970ToFileTime( s.st_ctime, &(this->CTime) );
    RtlSecondsSince1970ToFileTime( s.st_mtime, &(this->MTime) );
    RtlSecondsSince1970ToFileTime( s.st_atime, &(this->ATime) );
    UString fullPath = fp;
    this->Name = fullPath.Mid(fullPath.ReverseFind(L'/')+1);
    return true;
  }

public:

  bool Find(LPCWSTR path)
  {
    return (CFileInfoW::Find(path) || FindUsingStat(path));
  }
};

}}}

#endif  //__APPLE__
#endif //__ETHEREON_FILEFIND_H
