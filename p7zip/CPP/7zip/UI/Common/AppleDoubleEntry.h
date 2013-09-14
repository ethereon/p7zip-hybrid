/*

    AppleDouble Extended Attributes Handling
    Author  : Saumitro Dasgupta
    License : LGPL
    
    When files with HFS extended attributes (including resource forks) are archived,
    those attributes are stored in a separate file in the AppleDouble format.
    These files can be identified by their "._" prefix.
    
    There are currently two ways the AppleDouble files are organized in the archive:
    
        1.  If compressed using OS X's archive utility, the metadata files are
            isolated into the "__MACOSX" top level directory.
            
        2.  If compressed using ditto, they are stored along with the source file.
        
    The copyfile function can be used to merge back these attributes after extraction.
    
*/

#ifndef __APPLE_DOUBLE_ENTRY_H__
#define __APPLE_DOUBLE_ENTRY_H__

#import <copyfile.h>

class AppleDoubleEntry {
  
public:
    
    AString metadataPath;
    
    AString targetPath;
    
    AppleDoubleEntry* nextEntry;
    
    AppleDoubleEntry(AppleDoubleEntry* next) { nextEntry = next; }
    ~AppleDoubleEntry() { }
    
    bool mergeMetadata() {
        
        if(access((const char*)targetPath, F_OK)!=0) return false;
                
        return copyfile(
                (const char*)metadataPath,
                (const char*)targetPath,
                0,
                COPYFILE_UNPACK | COPYFILE_NOFOLLOW | COPYFILE_ACL | COPYFILE_XATTR)==0;
    }
};

#endif