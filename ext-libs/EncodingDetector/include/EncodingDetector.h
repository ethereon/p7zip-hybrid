//  
//  Character Encoding Detector for Mac OS X
//  Wrapper for Mozilla's Universal Character Encoding Detector
//
//  Author  : Saumitro Dasgupta
//  License : Same as Mozilla's universalchardet library.
//
//  Detector Preferences
//
//  You can alter the behavior of the encoding detector by setting the
//  following OS X preferences.
//
//  UCDEncodingDetectionEnabled : Hint to disable encoding detection.
//  This preference is not used by the encoder directly.
//
//  UCDConfidenceThreshold : The minimum confidence threshold.
//  A floating point value between 0 and 1.0.
//
//  UCFallbackEncoding : The encoding that will be used if the detected
//  encoding confidence lies below the threshold, or if the detector fails
//  to identify the encoding.
//  The value should be an integer corresponding to a valid CFStringEncoding.
//
//  If any of these preferences are not found, the default value 
//  specified in the source is used ( defines prefixed with ED_DEFAULT ).
//

#ifndef __ETHEREON_ENCODING_DETECTOR_H__
#define __ETHEREON_ENCODING_DETECTOR_H__

#include "nscore.h"
#include "nsUniversalDetector.h"
#include "nsCharSetProber.h"

typedef unsigned long EDStringEncoding;

class EncodingDetector : public nsUniversalDetector {
    
private:
        
    bool detectionEnabled;    
    float confidenceThreshold;
    EDStringEncoding fallbackEncoding;
    
    void initializeDefaults();
    
    const char* getMIMECharset();
    
public:
    
    EncodingDetector();
    ~EncodingDetector() { }
    
    void Report(const char* aCharset) { }
    
    bool isDetectionEnabled();
    
    bool isUTF8(EDStringEncoding encoding);
    
    EDStringEncoding detectEncoding(const char* cString, size_t length);
    
    char* convertToUTF8(const char* cString, EDStringEncoding encoding);
    
    void freeString(char* cStr);
};

#endif