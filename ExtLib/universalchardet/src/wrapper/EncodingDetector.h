#ifndef __ETHEREON_ENCODING_DETECTOR_H__
#define __ETHEREON_ENCODING_DETECTOR_H__

#include "nscore.h"
#include "nsUniversalDetector.h"
#include "nsCharSetProber.h"

typedef unsigned long EDStringEncoding;

class EncodingDetector : public nsUniversalDetector {
    
private:
    
    const char* getMIMECharset();
    
public:
    
    EncodingDetector(): nsUniversalDetector(NS_FILTER_ALL) { }
    
    void Report(const char* aCharset) { }
    
    bool isUTF8(EDStringEncoding encoding);
    
    EDStringEncoding detectEncoding(const char* cString, size_t length);
    
    char* convertToUTF8(const char* cString, EDStringEncoding encoding);
    
    void freeString(char* cStr);
};

#endif