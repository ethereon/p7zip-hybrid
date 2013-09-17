cmake_minimum_required(VERSION 2.8)

project(ENCDET)

set(UCD_SRC_DIR "${ENCDET_SOURCE_DIR}/universalchardet/src")

include_directories(

    ${UCD_SRC_DIR}/base/
    ${UCD_SRC_DIR}/kludge/
    ${ENCDET_SOURCE_DIR}/include/
)

set(MAC_FLAGS "-arch i386 -arch x86_64 -mmacosx-version-min=10.6")
set(CMAKE_CXX_FLAGS ${MAC_FLAGS})
set(CMAKE_C_FLAGS ${MAC_FLAGS})

add_library (
    encodingdetector
    STATIC

    # Kludge
    ${UCD_SRC_DIR}/kludge/kludge.c

    # Base
    ${UCD_SRC_DIR}/base/CharDistribution.cpp
    ${UCD_SRC_DIR}/base/JpCntx.cpp
    ${UCD_SRC_DIR}/base/LangBulgarianModel.cpp
    ${UCD_SRC_DIR}/base/LangCyrillicModel.cpp
    ${UCD_SRC_DIR}/base/LangGreekModel.cpp
    ${UCD_SRC_DIR}/base/LangHungarianModel.cpp
    ${UCD_SRC_DIR}/base/LangHebrewModel.cpp
    ${UCD_SRC_DIR}/base/LangThaiModel.cpp
    ${UCD_SRC_DIR}/base/nsHebrewProber.cpp
    ${UCD_SRC_DIR}/base/nsCharSetProber.cpp
    ${UCD_SRC_DIR}/base/nsBig5Prober.cpp
    ${UCD_SRC_DIR}/base/nsEUCJPProber.cpp
    ${UCD_SRC_DIR}/base/nsEUCKRProber.cpp
    ${UCD_SRC_DIR}/base/nsEUCTWProber.cpp
    ${UCD_SRC_DIR}/base/nsEscCharsetProber.cpp
    ${UCD_SRC_DIR}/base/nsEscSM.cpp
    ${UCD_SRC_DIR}/base/nsGB2312Prober.cpp
    ${UCD_SRC_DIR}/base/nsMBCSGroupProber.cpp
    ${UCD_SRC_DIR}/base/nsMBCSSM.cpp
    ${UCD_SRC_DIR}/base/nsSBCSGroupProber.cpp
    ${UCD_SRC_DIR}/base/nsSBCharSetProber.cpp
    ${UCD_SRC_DIR}/base/nsSJISProber.cpp
    ${UCD_SRC_DIR}/base/nsUTF8Prober.cpp
    ${UCD_SRC_DIR}/base/nsLatin1Prober.cpp
    ${UCD_SRC_DIR}/base/nsUniversalDetector.cpp

    # Wrapper
    src/EncodingDetector.cpp
)


