//  
//  Character Encoding Detector for Mac OS X
//  Wrapper for Mozilla's Universal Character Encoding Detector
//
//  Author  : Saumitro Dasgupta
//  License : Same as Mozilla's universalchardet library.
//

#include "EncodingDetector.h"
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

#define PREF_KEY_ENCODING_ENABLED "UCDEncodingDetectionEnabled"
#define PREF_KEY_CONFIDENCE_THRESHOLD "UCDConfidenceThreshold"
#define PREF_KEY_FALLBACK_ENCODING "UCDFallbackEncoding"

#define GET_PREFERENCE(x) CFPreferencesCopyAppValue(x, kCFPreferencesCurrentApplication)

EncodingDetector::EncodingDetector() : nsUniversalDetector(NS_FILTER_ALL) {
     
    this->initializeDefaults();
       
}

void EncodingDetector::initializeDefaults() {
    
    /* Check if encoding detection is enabled */    
    CFStringRef keyDetectionEnabled = CFSTR(PREF_KEY_ENCODING_ENABLED);
    CFBooleanRef isEnabledRef = (CFBooleanRef)GET_PREFERENCE(keyDetectionEnabled);
    
    if(isEnabledRef!=NULL) {
                
        this->detectionEnabled = CFBooleanGetValue(isEnabledRef);
        CFRelease(isEnabledRef);
        
    } else {
        
        //By default, we assume encoding detection is enabled,
        //unless explicitly disabled by the user.
        this->detectionEnabled = ED_DEFAULT_IS_ENCODING_ENABLED;
    }
    
    /* Set the confidence threshold */
    CFStringRef keyConfidenceThreshold = CFSTR(PREF_KEY_CONFIDENCE_THRESHOLD);
    CFNumberRef confidenceRef = (CFNumberRef)GET_PREFERENCE(keyConfidenceThreshold);
    
    if(confidenceRef!=NULL && CFNumberIsFloatType(confidenceRef)) {
        
        CFNumberGetValue(confidenceRef, kCFNumberFloatType, &(this->confidenceThreshold));
        CFRelease(confidenceRef);
        
    } else {
        
        this->confidenceThreshold = ED_DEFAULT_CONFIDENCE_THRESHOLD;
    }
    
    /* Set the fallback encoding */
    CFStringRef keyFallbackEncoding = CFSTR(PREF_KEY_FALLBACK_ENCODING);
    CFNumberRef encodingRef = (CFNumberRef)GET_PREFERENCE(keyFallbackEncoding);
    
    if(encodingRef!=NULL) {
        
        CFNumberGetValue(encodingRef, kCFNumberSInt32Type, &(this->fallbackEncoding));
        CFRelease(encodingRef);
        
    } else {
        
        this->fallbackEncoding = ED_DEFAULT_FALLBACK_ENCODING;
    }
    
}

const char* EncodingDetector::getMIMECharset() {
    
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

			if (maxProberConfidence>(this->confidenceThreshold) && mCharSetProbers[maxProber]) {

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

bool EncodingDetector::isUTF8(EDStringEncoding encoding) {
    
    return (encoding==kCFStringEncodingUTF8);
    
}

bool EncodingDetector::isDetectionEnabled() {
    
    return (this->detectionEnabled);
}

EDStringEncoding EncodingDetector::detectEncoding(const char* cString, size_t length) {
        
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


char* EncodingDetector::convertToUTF8(const char* cString, EDStringEncoding encoding) {
    
    CFStringRef cfString = CFStringCreateWithCString(kCFAllocatorDefault, cString, encoding);
    
    if(!cfString) return NULL;
    
    CFIndex bufferSize = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfString), kCFStringEncodingUTF8);
    
    char* buffer = (char*)malloc(bufferSize);
    
    Boolean success = CFStringGetCString(cfString, buffer, bufferSize, kCFStringEncodingUTF8);
    
    CFRelease(cfString);
    
    if(success) return buffer;
    
    return NULL;
}

void EncodingDetector::freeString(char* cStr) {
    
    free(cStr);
}

