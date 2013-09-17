//
//  Character Encoding Detector for Mac OS X
//  Wrapper for Mozilla's Universal Character Encoding Detector
//
//  Author  : Saumitro Dasgupta
//  License : Same as Mozilla's universalchardet library.
//

#include "EncodingDetector.h"
#include "nscore.h"
#include "nsUniversalDetector.h"
#include "nsCharSetProber.h"

#include <string.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>

//By default, we assume encoding detection is enabled, unless explicitly
//disabled by the user.
#define ED_DEFAULT_IS_ENCODING_ENABLED true

//Incorrect results have been observed at confidence = ~0.54.
//Correct results with confidence as low as ~0.70 have been observed.
//We'll be conservative with the threshold.
#define ED_DEFAULT_CONFIDENCE_THRESHOLD 0.7
#define ED_DEFAULT_FALLBACK_ENCODING kCFStringEncodingUTF8

#define COPY_PREFERENCE(x) CFPreferencesCopyAppValue(x, kCFPreferencesCurrentApplication)

static CFStringRef kPreferenceIsDetectionEnabled = CFSTR("UCDEncodingDetectionEnabled");
static CFStringRef kPreferenceConfidenceThreshold = CFSTR("UCDConfidenceThreshold");
static CFStringRef kPreferenceFallbackEncoding = CFSTR("UCDFallbackEncoding");

namespace EncDetect
{
    class CoreEncodingDetector : public nsUniversalDetector
    {
    private:
        
        float confidenceThreshold;
        CFStringEncoding fallbackEncoding;
        
        void initializeDefaults();
        const char* getMIMECharset();
        
    public:
        
        CoreEncodingDetector();
        ~CoreEncodingDetector() { }
        
        void Report(const char* aCharset) { }
        CFStringEncoding detectEncoding(const char* cString, int length);
        char* convertToUTF8(const char* cString, CFStringEncoding encoding);
        void freeString(char* cStr);
    };
    
    CoreEncodingDetector::CoreEncodingDetector() : nsUniversalDetector(NS_FILTER_ALL)
    {
        this->initializeDefaults();
    }
    
    void CoreEncodingDetector::initializeDefaults()
    {
        /* Set the confidence threshold */
        CFNumberRef confidenceRef = (CFNumberRef)COPY_PREFERENCE(kPreferenceConfidenceThreshold);
        if(confidenceRef!=NULL && CFNumberIsFloatType(confidenceRef))
        {
            CFNumberGetValue(confidenceRef, kCFNumberFloatType, &(this->confidenceThreshold));
            CFRelease(confidenceRef);
            
        }
        else
        {
            this->confidenceThreshold = ED_DEFAULT_CONFIDENCE_THRESHOLD;
        }
        
        /* Set the fallback encoding */
        CFNumberRef encodingRef = (CFNumberRef)COPY_PREFERENCE(kPreferenceFallbackEncoding);
        if(encodingRef!=NULL)
        {
            CFNumberGetValue(encodingRef, kCFNumberSInt32Type, &(this->fallbackEncoding));
            CFRelease(encodingRef);
        } else
        {
            this->fallbackEncoding = ED_DEFAULT_FALLBACK_ENCODING;
        }
    }
    
    const char* CoreEncodingDetector::getMIMECharset()
    {
        //Code extracted from nsUniversalDetector.cpp : void nsUniversalDetector::DataEnd()
        //We bypass the minimum threshold check & report callback here.
        
        if(!mGotData) return NULL;
        
        if(mDetectedCharset) return mDetectedCharset;
        
        switch(mInputState)
        {
            case eHighbyte:
            {
                float proberConfidence;
                float maxProberConfidence = (float)0.0;
                PRInt32 maxProber = 0;
                
                for (PRInt32 i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
                {
                    if (mCharSetProbers[i])
                    {
                        proberConfidence = mCharSetProbers[i]->GetConfidence();
                        if (proberConfidence > maxProberConfidence)
                        {
                            maxProberConfidence = proberConfidence;
                            maxProber = i;
                        }
                    }
                }
                
                if (maxProberConfidence>(this->confidenceThreshold) && mCharSetProbers[maxProber])
                {
                    const char* charsetName = mCharSetProbers[maxProber]->GetCharSetName();
                    return charsetName;
                }
            }
                break;
                
            case ePureAscii:
                return "UTF-8";
        }
        
        return NULL;
    }
    
    CFStringEncoding CoreEncodingDetector::detectEncoding(const char* cString, int length)
    {
        this->Reset();
        this->HandleData(cString, length);
        const char* charset =  this->getMIMECharset();
        
        if (charset==NULL) return this->fallbackEncoding;
        
        CFStringRef cfCharset = CFStringCreateWithCString(kCFAllocatorDefault, charset, kCFStringEncodingUTF8);
        
        CFStringEncoding enc = CFStringConvertIANACharSetNameToEncoding(cfCharset);
        
        CFRelease(cfCharset);
        
        // Adapted from UniversalDetector.m
        // UniversalDetector detects CP949 but returns "EUC-KR" because CP949 lacks an IANA name.
        // Kludge to make strings decode properly anyway.
        if(enc==kCFStringEncodingEUC_KR) enc=kCFStringEncodingDOSKorean;
        
        if(enc==kCFStringEncodingInvalidId) return this->fallbackEncoding;
        
        return enc;
    }
    
    char* CoreEncodingDetector::convertToUTF8(const char* cString, CFStringEncoding encoding)
    {
        CFStringRef cfString = CFStringCreateWithCString(kCFAllocatorDefault, cString, encoding);
        if(!cfString) return NULL;
        CFIndex bufferSize = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfString), kCFStringEncodingUTF8);
        char* buffer = (char*)malloc(bufferSize);
        Boolean success = CFStringGetCString(cfString, buffer, bufferSize, kCFStringEncodingUTF8);
        CFRelease(cfString);
        if(success) return buffer;
        return NULL;
    }
    
    EncodingDetector::EncodingDetector() : coreDetector(new CoreEncodingDetector())
    {
        /* Check if encoding detection is enabled */
        CFBooleanRef isEnabledRef = (CFBooleanRef)COPY_PREFERENCE(kPreferenceIsDetectionEnabled);
        if(isEnabledRef!=NULL)
        {
            this->isDetectionEnabled = CFBooleanGetValue(isEnabledRef);
            CFRelease(isEnabledRef);
        }
        else
        {
            this->isDetectionEnabled = ED_DEFAULT_IS_ENCODING_ENABLED;
        }
    }
    
    EncodingDetector::~EncodingDetector()
    {
        delete coreDetector;
    }
    
    char* EncodingDetector::getUTF8Representation(const char* cString, int length)
    {
        if(!(this->isDetectionEnabled)) return NULL;
        CFStringEncoding encoding = coreDetector->detectEncoding(cString, length);
        if(encoding==kCFStringEncodingUTF8) return NULL;
        return coreDetector->convertToUTF8(cString, encoding);
    }
    
    void EncodingDetector::freeString(char* cStr)
    {
        free(cStr);
    }    
}
