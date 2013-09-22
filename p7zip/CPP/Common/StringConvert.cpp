// Common/StringConvert.cpp

#include "StdAfx.h"
#include <stdlib.h>

#include "StringConvert.h"
extern "C"
{
int global_use_utf16_conversion = 0;
}


#ifdef LOCALE_IS_UTF8

#ifdef __APPLE_CC__
#define UInt32  macUIn32
#include <CoreFoundation/CoreFoundation.h>
#undef UInt32

UString MultiByteToUnicodeString(const AString &srcString, UINT codePage)
{
  const char* cString = &srcString[0];
  CFMutableStringRef stringRep = CFStringCreateMutable(kCFAllocatorDefault, strlen(cString));
  CFStringAppendCString(stringRep, cString, kCFStringEncodingUTF8);
  CFStringNormalize(stringRep, kCFStringNormalizationFormC);
  UString resultString;
  size_t n = CFStringGetLength(stringRep);
  for(size_t i=0; i<n; ++i)
  {
    resultString += CFStringGetCharacterAtIndex(stringRep, i);
  }
  CFRelease(stringRep);
  return resultString;
}

AString UnicodeStringToMultiByte(const UString &srcString, UINT codePage)
{
  const wchar_t * wcs = &srcString[0];
  const CFIndex bufferCapacity = 4096;
  char utf8[bufferCapacity];
  UniChar uniChars[bufferCapacity];
  size_t n = wcslen(wcs);
  for(size_t i=0; i<=n; ++i)
  {
    uniChars[i] = wcs[i];
  }
  CFMutableStringRef stringRep = CFStringCreateMutableWithExternalCharactersNoCopy(kCFAllocatorDefault,
                                                                                   uniChars,
                                                                                   n,
                                                                                   bufferCapacity,
                                                                                   kCFAllocatorNull);
  CFStringNormalize(stringRep, kCFStringNormalizationFormD);
  CFStringGetCString(stringRep, utf8, bufferCapacity, kCFStringEncodingUTF8);
  CFRelease(stringRep);
  return AString(utf8);
}

UString NormalizeUnicodeString(const UString &unicodeString)
{
  const wchar_t* wcs = &unicodeString[0];
  const CFIndex bufferCapacity = 4096;
  UniChar uniChars[bufferCapacity];
  size_t n = wcslen(wcs);
  for(size_t i=0; i<n; ++i)
  {
    uniChars[i] = wcs[i];
  }
  CFMutableStringRef stringRep = CFStringCreateMutableWithExternalCharactersNoCopy(kCFAllocatorDefault,
                                                                                   uniChars,
                                                                                   n,
                                                                                   bufferCapacity,
                                                                                   kCFAllocatorNull);
  CFStringNormalize(stringRep, kCFStringNormalizationFormC);
  CFIndex normalizedLength = CFStringGetLength(stringRep);
  UString normalizedString;
  for(CFIndex i=0; i<normalizedLength; ++i)
  {
    normalizedString += uniChars[i];
  }
  CFRelease(stringRep);
  return normalizedString;
}

#else /* __APPLE_CC__ */


#include "UTFConvert.h"

UString MultiByteToUnicodeString(const AString &srcString, UINT codePage)
{
  if ((global_use_utf16_conversion) && (!srcString.IsEmpty()))
  {
    UString resultString;
    bool bret = ConvertUTF8ToUnicode(srcString,resultString);
    if (bret) return resultString;
  }

  UString resultString;
  for (int i = 0; i < srcString.Length(); i++)
    resultString += wchar_t(srcString[i] & 255);

  return resultString;
}

AString UnicodeStringToMultiByte(const UString &srcString, UINT codePage)
{
  if ((global_use_utf16_conversion) && (!srcString.IsEmpty()))
  {
    AString resultString;
    bool bret = ConvertUnicodeToUTF8(srcString,resultString);
    if (bret) return resultString;
  }

  AString resultString;
  for (int i = 0; i < srcString.Length(); i++)
  {
    if (srcString[i] >= 256) resultString += '?';
    else                     resultString += char(srcString[i]);
  }
  return resultString;
}

#endif /* __APPLE_CC__ */

#else /* LOCALE_IS_UTF8 */

UString MultiByteToUnicodeString(const AString &srcString, UINT /* codePage */ )
{
#ifdef ENV_HAVE_MBSTOWCS
  if ((global_use_utf16_conversion) && (!srcString.IsEmpty()))
  {
    UString resultString;
    int numChars = mbstowcs(resultString.GetBuffer(srcString.Length()),srcString,srcString.Length()+1);
    if (numChars >= 0) {
      resultString.ReleaseBuffer(numChars);
      return resultString;
    }
  }
#endif

  UString resultString;
  for (int i = 0; i < srcString.Length(); i++)
    resultString += wchar_t(srcString[i] & 255);

  return resultString;
}

AString UnicodeStringToMultiByte(const UString &srcString, UINT /* codePage */ )
{
#ifdef ENV_HAVE_WCSTOMBS
  if ((global_use_utf16_conversion) && (!srcString.IsEmpty()))
  {
    AString resultString;
    int numRequiredBytes = srcString.Length() * 6+1;
    int numChars = wcstombs(resultString.GetBuffer(numRequiredBytes),srcString,numRequiredBytes);
    if (numChars >= 0) {
      resultString.ReleaseBuffer(numChars);
      return resultString;
    }
  }
#endif

  AString resultString;
  for (int i = 0; i < srcString.Length(); i++)
  {
    if (srcString[i] >= 256) resultString += '?';
    else                     resultString += char(srcString[i]);
  }
  return resultString;
}

#endif /* LOCALE_IS_UTF8 */

