cmake_minimum_required(VERSION 2.8)

project(UNICHARDET)

include_directories(

    ${UNICHARDET_SOURCE_DIR}/src/base/
    ${UNICHARDET_SOURCE_DIR}/src/kludge/
    ${UNICHARDET_SOURCE_DIR}/src/wrapper/
)

set(MAC_FLAGS "-arch i386 -arch x86_64 -mmacosx-version-min=10.5")
set(CMAKE_CXX_FLAGS ${MAC_FLAGS}) 
set(CMAKE_C_FLAGS ${MAC_FLAGS})

add_library (
    universalchardet
    STATIC
    
    # Kludge
    src/kludge/kludge.c
    
    # Base
    src/base/CharDistribution.cpp
    src/base/JpCntx.cpp
    src/base/LangBulgarianModel.cpp
    src/base/LangCyrillicModel.cpp
    src/base/LangGreekModel.cpp
    src/base/LangHungarianModel.cpp
    src/base/LangHebrewModel.cpp
    src/base/LangThaiModel.cpp
    src/base/nsHebrewProber.cpp
    src/base/nsCharSetProber.cpp
    src/base/nsBig5Prober.cpp
    src/base/nsEUCJPProber.cpp
    src/base/nsEUCKRProber.cpp
    src/base/nsEUCTWProber.cpp
    src/base/nsEscCharsetProber.cpp
    src/base/nsEscSM.cpp
    src/base/nsGB2312Prober.cpp
    src/base/nsMBCSGroupProber.cpp
    src/base/nsMBCSSM.cpp
    src/base/nsSBCSGroupProber.cpp
    src/base/nsSBCharSetProber.cpp
    src/base/nsSJISProber.cpp
    src/base/nsUTF8Prober.cpp
    src/base/nsLatin1Prober.cpp
    src/base/nsUniversalDetector.cpp
    
    # Wrapper
    src/wrapper/EncodingDetector.cpp

)


