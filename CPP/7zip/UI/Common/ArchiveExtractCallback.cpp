// ArchiveExtractCallback.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/Wildcard.h"
#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"

#include "../../Common/FilePathAutoRename.h"

#include "../Common/ExtractingFilePath.h"

#include "ArchiveExtractCallback.h"

using namespace NWindows;

static const wchar_t *kCantAutoRename = L"ERROR: Can not create file with auto name";
static const wchar_t *kCantRenameFile = L"ERROR: Can not rename existing file ";
static const wchar_t *kCantDeleteOutputFile = L"ERROR: Can not delete output file ";

void CArchiveExtractCallback::Init(
    const NWildcard::CCensorNode *wildcardCensor,
    const CArc *arc,
    IFolderArchiveExtractCallback *extractCallback2,
    bool stdOutMode, bool testMode, bool crcMode,
    const UString &directoryPath,
    const UStringVector &removePathParts,
    UInt64 packSize)
{
  _wildcardCensor = wildcardCensor;

  _stdOutMode = stdOutMode;
  _testMode = testMode;
  _crcMode = crcMode;
  _unpTotal = 1;
  _packTotal = packSize;

  _extractCallback2 = extractCallback2;
  _compressProgress.Release();
  _extractCallback2.QueryInterface(IID_ICompressProgressInfo, &_compressProgress);

  LocalProgressSpec->Init(extractCallback2, true);
  LocalProgressSpec->SendProgress = false;

 
  _removePathParts = removePathParts;
  _arc = arc;
  _directoryPath = directoryPath;
  NFile::NName::NormalizeDirPathPrefix(_directoryPath);
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 size)
{
  COM_TRY_BEGIN
  _unpTotal = size;
  if (!_multiArchives && _extractCallback2)
    return _extractCallback2->SetTotal(size);
  return S_OK;
  COM_TRY_END
}

static void NormalizeVals(UInt64 &v1, UInt64 &v2)
{
  const UInt64 kMax = (UInt64)1 << 31;
  while (v1 > kMax)
  {
    v1 >>= 1;
    v2 >>= 1;
  }
}

static UInt64 MyMultDiv64(UInt64 unpCur, UInt64 unpTotal, UInt64 packTotal)
{
  NormalizeVals(packTotal, unpTotal);
  NormalizeVals(unpCur, unpTotal);
  if (unpTotal == 0)
    unpTotal = 1;
  return unpCur * packTotal / unpTotal;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 *completeValue)
{
  COM_TRY_BEGIN
  if (!_extractCallback2)
    return S_OK;

  if (_multiArchives)
  {
    if (completeValue != NULL)
    {
      UInt64 packCur = LocalProgressSpec->InSize + MyMultDiv64(*completeValue, _unpTotal, _packTotal);
      return _extractCallback2->SetCompleted(&packCur);
    }
  }
  return _extractCallback2->SetCompleted(completeValue);
  COM_TRY_END
}

STDMETHODIMP CArchiveExtractCallback::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  COM_TRY_BEGIN
  return _localProgress->SetRatioInfo(inSize, outSize);
  COM_TRY_END
}

void CArchiveExtractCallback::CreateComplexDirectory(const UStringVector &dirPathParts, UString &fullPath)
{
  fullPath = _directoryPath;
  for (int i = 0; i < dirPathParts.Size(); i++)
  {
    if (i > 0)
      fullPath += wchar_t(NFile::NName::kDirDelimiter);
    fullPath += dirPathParts[i];
    NFile::NDirectory::MyCreateDirectory(fullPath);
  }
}

HRESULT CArchiveExtractCallback::GetTime(int index, PROPID propID, FILETIME &filetime, bool &filetimeIsDefined)
{
  filetimeIsDefined = false;
  NCOM::CPropVariant prop;
  RINOK(_arc->Archive->GetProperty(index, propID, &prop));
  if (prop.vt == VT_FILETIME)
  {
    filetime = prop.filetime;
    filetimeIsDefined = (filetime.dwHighDateTime != 0 || filetime.dwLowDateTime != 0);
  }
  else if (prop.vt != VT_EMPTY)
    return E_FAIL;
  return S_OK;
}

HRESULT CArchiveExtractCallback::GetUnpackSize()
{
  NCOM::CPropVariant prop;
  RINOK(_arc->Archive->GetProperty(_index, kpidSize, &prop));
  _curSizeDefined = (prop.vt != VT_EMPTY);
  if (_curSizeDefined)
    _curSize = ConvertPropVariantToUInt64(prop);
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode)
{
  COM_TRY_BEGIN
  _crcStream.Release();
  *outStream = 0;
  _outFileStream.Release();

  _encrypted = false;
  _isSplit = false;
  _curSize = 0;
  _curSizeDefined = false;
  _index = index;

  UString fullPath;

  IInArchive *archive = _arc->Archive;
  RINOK(_arc->GetItemPath(index, fullPath));
  RINOK(IsArchiveItemFolder(archive, index, _fi.IsDir));

  _filePath = fullPath;

  {
    NCOM::CPropVariant prop;
    RINOK(archive->GetProperty(index, kpidPosition, &prop));
    if (prop.vt != VT_EMPTY)
    {
      if (prop.vt != VT_UI8)
        return E_FAIL;
      _position = prop.uhVal.QuadPart;
      _isSplit = true;
    }
  }
    
  RINOK(GetArchiveItemBoolProp(archive, index, kpidEncrypted, _encrypted));

  RINOK(GetUnpackSize());

  if (_wildcardCensor)
  {
    if (!_wildcardCensor->CheckPath(fullPath, !_fi.IsDir))
      return S_OK;
  }

  if (askExtractMode == NArchive::NExtract::NAskMode::kExtract && !_testMode)
  {
    if (_stdOutMode)
    {
      CMyComPtr<ISequentialOutStream> outStreamLoc = new CStdOutFileStream;
      *outStream = outStreamLoc.Detach();
      return S_OK;
    }

    {
      NCOM::CPropVariant prop;
      RINOK(archive->GetProperty(index, kpidAttrib, &prop));
      if (prop.vt == VT_UI4)
      {
        _fi.Attrib = prop.ulVal;
        _fi.AttribDefined = true;
      }
      else if (prop.vt == VT_EMPTY)
        _fi.AttribDefined = false;
      else
        return E_FAIL;
    }

    RINOK(GetTime(index, kpidCTime, _fi.CTime, _fi.CTimeDefined));
    RINOK(GetTime(index, kpidATime, _fi.ATime, _fi.ATimeDefined));
    RINOK(GetTime(index, kpidMTime, _fi.MTime, _fi.MTimeDefined));

    bool isAnti = false;
    RINOK(_arc->IsItemAnti(index, isAnti));

    UStringVector pathParts;
    SplitPathToParts(fullPath, pathParts);
  
    if (pathParts.IsEmpty())
      return E_FAIL;
    
#ifdef __APPLE__
    
    UString &itemName = pathParts[pathParts.Size()-1];    
    wchar_t* wcItemName = itemName.GetBuffer(itemName.Length()+1);
    
    //Is this file in the __MACOSX dir?      
    bool isMetaEnclosed = (pathParts[0].Compare(L"__MACOSX")==0);
    
    //Is it an AppleDouble metadata file?
    bool isMetadataFile = itemName.Length()>2 && wcItemName[0]==L'.' && wcItemName[1]==L'_';
    
    //Is this just a metadata dir entry (or potentially an invalid file entry)? Skip it.
    if((isMetaEnclosed && !isMetadataFile))
        return S_OK;

    //Remove the "._" prefix    
    if(isMetadataFile)
        itemName.Delete(0, 2);
                
    //Remove the "__MACOSX" prefix
    if(isMetaEnclosed)                        
        pathParts.Delete(0);
    
#endif
    
    int numRemovePathParts = 0;
    switch(_pathMode)
    {
      case NExtract::NPathMode::kFullPathnames:
        break;
      case NExtract::NPathMode::kCurrentPathnames:
      {
        numRemovePathParts = _removePathParts.Size();
        if (pathParts.Size() <= numRemovePathParts)
          return E_FAIL;
        for (int i = 0; i < numRemovePathParts; i++)
          if (_removePathParts[i].CompareNoCase(pathParts[i]) != 0)
            return E_FAIL;
        break;
      }
      case NExtract::NPathMode::kNoPathnames:
      {
        numRemovePathParts = pathParts.Size() - 1;
        break;
      }
    }
    pathParts.Delete(0, numRemovePathParts);
    MakeCorrectPath(pathParts);
    UString processedPath = MakePathNameFromParts(pathParts);
    if (!isAnti)
    {
      if (!_fi.IsDir)
      {
        if (!pathParts.IsEmpty())
          pathParts.DeleteBack();
      }
#ifdef __APPLE__    
      if (!(pathParts.IsEmpty() || isMetaEnclosed || isMetadataFile))
#else
      if (!pathParts.IsEmpty())
#endif
      {
        UString fullPathNew;
        CreateComplexDirectory(pathParts, fullPathNew);
        if (_fi.IsDir)
          NFile::NDirectory::SetDirTime(fullPathNew,
            (WriteCTime && _fi.CTimeDefined) ? &_fi.CTime : NULL,
            (WriteATime && _fi.ATimeDefined) ? &_fi.ATime : NULL,
            (WriteMTime && _fi.MTimeDefined) ? &_fi.MTime : (_arc->MTimeDefined ? &_arc->MTime : NULL));
      }
    }
    
    UString fullProcessedPath = _directoryPath + processedPath;

    if (_fi.IsDir)
    {
      _diskFilePath = fullProcessedPath;
      if (isAnti)
        NFile::NDirectory::MyRemoveDirectory(_diskFilePath);
      return S_OK;
    }
    
#ifdef __APPLE__
        
    if(isMetadataFile) {
        
        if(_adHead==NULL) {
            
            //Create a temp dir in the extraction dir to store the metadata
            metaDirPath = UnicodeStringToMultiByte(_directoryPath + L".metadata.XXXXXXXX");
            int metaDirLen = metaDirPath.Length();
            char* metaBuffer = metaDirPath.GetBuffer(metaDirLen+2);
            mkdtemp(metaBuffer);
            metaBuffer[metaDirLen]='/';
            metaBuffer[metaDirLen+1]=0;
            metaDirPath.ReleaseBuffer();
        }

        AppleDoubleEntry* newEntry = new AppleDoubleEntry(_adHead);
        _adHead = newEntry;
        
        //The file to which the extended attributes will be applied
        newEntry->targetPath = UnicodeStringToMultiByte(fullProcessedPath);
        
        //Extract the metadata file to a temporary location
        newEntry->metadataPath = metaDirPath + "meta.XXXXXXXX";
        char* metaPathBuffer = newEntry->metadataPath.GetBuffer(newEntry->metadataPath.Length()+1);
        mktemp(metaPathBuffer);
        newEntry->metadataPath.ReleaseBuffer();
        
        fullProcessedPath = MultiByteToUnicodeString(newEntry->metadataPath);                
    } 
    
#endif

    if (!_isSplit)
    {
    NFile::NFind::CFileInfoW fileInfo;
    if (fileInfo.Find(fullProcessedPath))
    {
      switch(_overwriteMode)
      {
        case NExtract::NOverwriteMode::kSkipExisting:
          return S_OK;
        case NExtract::NOverwriteMode::kAskBefore:
        {
          Int32 overwiteResult;
          RINOK(_extractCallback2->AskOverwrite(
              fullProcessedPath, &fileInfo.MTime, &fileInfo.Size, fullPath,
              _fi.MTimeDefined ? &_fi.MTime : NULL,
              _curSizeDefined ? &_curSize : NULL,
              &overwiteResult))

          switch(overwiteResult)
          {
            case NOverwriteAnswer::kCancel:
              return E_ABORT;
            case NOverwriteAnswer::kNo:
              return S_OK;
            case NOverwriteAnswer::kNoToAll:
              _overwriteMode = NExtract::NOverwriteMode::kSkipExisting;
              return S_OK;
            case NOverwriteAnswer::kYesToAll:
              _overwriteMode = NExtract::NOverwriteMode::kWithoutPrompt;
              break;
            case NOverwriteAnswer::kYes:
              break;
            case NOverwriteAnswer::kAutoRename:
              _overwriteMode = NExtract::NOverwriteMode::kAutoRename;
              break;
            default:
              return E_FAIL;
          }
        }
      }
      if (_overwriteMode == NExtract::NOverwriteMode::kAutoRename)
      {
        if (!AutoRenamePath(fullProcessedPath))
        {
          UString message = UString(kCantAutoRename) + fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return E_FAIL;
        }
      }
      else if (_overwriteMode == NExtract::NOverwriteMode::kAutoRenameExisting)
      {
        UString existPath = fullProcessedPath;
        if (!AutoRenamePath(existPath))
        {
          UString message = kCantAutoRename + fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return E_FAIL;
        }
        if (!NFile::NDirectory::MyMoveFile(fullProcessedPath, existPath))
        {
          UString message = UString(kCantRenameFile) + fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return E_FAIL;
        }
      }
      else
        if (!NFile::NDirectory::DeleteFileAlways(fullProcessedPath))
        {
          UString message = UString(kCantDeleteOutputFile) +  fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return S_OK;
          // return E_FAIL;
        }
    }
    }
    if (!isAnti)
    {
      _outFileStreamSpec = new COutFileStream;
      CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);
      if (!_outFileStreamSpec->Open(fullProcessedPath, _isSplit ? OPEN_ALWAYS: CREATE_ALWAYS))
      {
        // if (::GetLastError() != ERROR_FILE_EXISTS || !isSplit)
        {
          UString message = L"can not open output file " + fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return S_OK;
        }
      }
      if (_isSplit)
      {
        RINOK(_outFileStreamSpec->Seek(_position, STREAM_SEEK_SET, NULL));
      }
      _outFileStream = outStreamLoc;
      *outStream = outStreamLoc.Detach();
    }
    _diskFilePath = fullProcessedPath;
  }
  else
  {
    *outStream = NULL;
  }
  if (_crcMode)
  {
    _crcStreamSpec = new COutStreamWithCRC;
    _crcStream = _crcStreamSpec;
    CMyComPtr<ISequentialOutStream> crcStream = _crcStreamSpec;
    _crcStreamSpec->SetStream(*outStream);
    if (*outStream)
      (*outStream)->Release();
    *outStream = crcStream.Detach();
    _crcStreamSpec->Init(true);
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
  COM_TRY_BEGIN
  _extractMode = false;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:
      if (_testMode)
        askExtractMode = NArchive::NExtract::NAskMode::kTest;
      else
        _extractMode = true;
      break;
  };
  return _extractCallback2->PrepareOperation(_filePath, _fi.IsDir,
      askExtractMode, _isSplit ? &_position: 0);
  COM_TRY_END
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
  COM_TRY_BEGIN
  switch(operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
    case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
    case NArchive::NExtract::NOperationResult::kCRCError:
    case NArchive::NExtract::NOperationResult::kDataError:
      break;
    default:
      _outFileStream.Release();
      return E_FAIL;
  }
  if (_crcStream)
  {
    CrcSum += _crcStreamSpec->GetCRC();
    _curSize = _crcStreamSpec->GetSize();
    _curSizeDefined = true;
    _crcStream.Release();
  }
  if (_outFileStream)
  {
    _outFileStreamSpec->SetTime(
        (WriteCTime && _fi.CTimeDefined) ? &_fi.CTime : NULL,
        (WriteATime && _fi.ATimeDefined) ? &_fi.ATime : NULL,
        (WriteMTime && _fi.MTimeDefined) ? &_fi.MTime : (_arc->MTimeDefined ? &_arc->MTime : NULL));
    _curSize = _outFileStreamSpec->ProcessedSize;
    _curSizeDefined = true;
    RINOK(_outFileStreamSpec->Close());
    _outFileStream.Release();
  }
  if (!_curSizeDefined)
    GetUnpackSize();
  if (_curSizeDefined)
    UnpackSize += _curSize;
  if (_fi.IsDir)
    NumFolders++;
  else
    NumFiles++;

  if (_extractMode && _fi.AttribDefined)
    NFile::NDirectory::MySetFileAttributes(_diskFilePath, _fi.Attrib);
  RINOK(_extractCallback2->SetOperationResult(operationResult, _encrypted));
  return S_OK;
  COM_TRY_END
}

/*
STDMETHODIMP CArchiveExtractCallback::GetInStream(
    const wchar_t *name, ISequentialInStream **inStream)
{
  COM_TRY_BEGIN
  CInFileStream *inFile = new CInFileStream;
  CMyComPtr<ISequentialInStream> inStreamTemp = inFile;
  if (!inFile->Open(_srcDirectoryPrefix + name))
    return ::GetLastError();
  *inStream = inStreamTemp.Detach();
  return S_OK;
  COM_TRY_END
}
*/

STDMETHODIMP CArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
{
  COM_TRY_BEGIN
  if (!_cryptoGetTextPassword)
  {
    RINOK(_extractCallback2.QueryInterface(IID_ICryptoGetTextPassword,
        &_cryptoGetTextPassword));
  }
  return _cryptoGetTextPassword->CryptoGetTextPassword(password);
  COM_TRY_END
}

void CArchiveExtractCallback::PerformPostProcessing()
{
#ifdef __APPLE__    
    //Process any extended attributes queued during extraction    
    if(_adHead) {
        while(_adHead) {
            AppleDoubleEntry* ce = _adHead;
            ce->mergeMetadata();
            unlink(ce->metadataPath);
            _adHead = ce->nextEntry;
            delete ce;   
        }
        rmdir((const char*)metaDirPath);
    }
#endif
}
