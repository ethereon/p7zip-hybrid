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

namespace EncDetect
{
    class CoreEncodingDetector;

    class EncodingDetector
    {
    public:

        EncodingDetector();
        ~EncodingDetector();

        //Auto-detects the given string's encoding and, if necessary, converts
        //and returns the UTF8 representation.
        //The converted string can be freed using the freeString method.
        //This method returns NULL if any of the following is true:
        //1. Encoding detection is disabled (via preferences).
        //2. The string is detected to be already UTF8.
        //3. The encoding detection or conversion operation failed.
        char* getUTF8Representation(const char* cString, int length);

        void freeString(char* cStr);

    private:

        bool isDetectionEnabled;
        CoreEncodingDetector* coreDetector;
    };
}

#endif