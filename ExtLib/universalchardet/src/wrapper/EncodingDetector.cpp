#include "EncodingDetector.h"
#include <string.h>
#include <CoreFoundation/CoreFoundation.h>

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

			if (mCharSetProbers[maxProber]) {
			    
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

EDStringEncoding EncodingDetector::detectEncoding(const char* cString, size_t length) {
    
    this->Reset(); 
       
    this->HandleData(cString, length);
    
    const char* charset =  this->getMIMECharset();
    
    if (!charset) return 0;
    
    CFStringRef cfCharset = CFStringCreateWithCString(kCFAllocatorDefault, charset, kCFStringEncodingUTF8);

	CFStringEncoding enc = CFStringConvertIANACharSetNameToEncoding(cfCharset);	
	
    CFRelease(cfCharset);

    // Adapted from UniversalDetector.m    
	// UniversalDetector detects CP949 but returns "EUC-KR" because CP949 lacks an IANA name.
	// Kludge to make strings decode properly anyway.
	if(enc==kCFStringEncodingEUC_KR) enc=kCFStringEncodingDOSKorean;
	
	// We fallback to UTF-8
    if(enc==kCFStringEncodingInvalidId) return kCFStringEncodingUTF8;
	
    return enc;	
}


char* EncodingDetector::convertToUTF8(const char* cString, EDStringEncoding encoding) {
    
    CFStringRef cfString = CFStringCreateWithCString(kCFAllocatorDefault, cString, encoding);
    
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

