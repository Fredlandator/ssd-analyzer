#ifndef SSD_ANALYZER_H
#define SSD_ANALYZER_H

#include <Analyzer.h>
#include "SSDAnalyzerResults.h"
#include "SSDSimulationDataGenerator.h"

typedef unsigned int UINT;

// SSD Protocol constants
#define SSD_MODE_PROGRAM 0x01
#define SSD_MODE_RACE    0x02

enum eFrameState {
    FSTATE_INIT,
    FSTATE_PREAMBLE,
    FSTATE_PSBIT,
    FSTATE_CMDBYTE,
    FSTATE_DSBIT,
    FSTATE_DSBIT_CHECKSUM,  // Bit start avant checksum
    FSTATE_DATABYTE,
    FSTATE_CHECKSUM,
    FSTATE_PEBIT
};

class SSDAnalyzerSettings;
class ANALYZER_EXPORT SSDAnalyzer : public Analyzer2
{
public:
    SSDAnalyzer();
    virtual ~SSDAnalyzer();

    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels);
    virtual U32 GetMinimumSampleRateHz();

    virtual const char* GetAnalyzerName() const;
    virtual bool NeedsRerun();

    // Helper functions
    void Setup();
    UINT LookaheadNextHBit(U64* nSample);
    UINT GetNextHBit(U64* nSample);
    UINT GetNextBit(U64* nSample);
    void PostFrame(U64 nStartSample, U64 nEndSample, eFrameType ft, U8 Flags, U64 Data1, U64 Data2);
    void DecodeCarData(U8 carData, char* buffer, int bufferSize);
    const char* GetCurrentPacketColor();

protected: //vars
    std::unique_ptr<SSDAnalyzerSettings> mSettings;
    std::unique_ptr<SSDAnalyzerResults> mResults;
    AnalyzerChannelData* mSSD;

    SSDSimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitilized;

    // Timing parameters
    U32 mSampleRateHz;
    UINT mMin1hbit, mMax1hbit;    // Bit 1 half-bit timing
    UINT mMin0hbit, mMax0hbit;    // Bit 0 half-bit timing
    UINT mMaxBitLen;              // Maximum bit length
    UINT mMinPEHold, mMaxPGap;    // Packet end hold and gap timing

    // SSD protocol state - RACE et PROGRAM ont tous les deux 6 bytes de donnees
    U8 mCurrentMode;              // Current packet mode (RACE/PROGRAM)
    U8 mCarCount;                 // Current car being processed (0-5 pour 6 bytes)
    U8 mCalculatedChecksum;       // Calculated checksum (starts at 0xFF)
    U8 mCarData[6];              // Car data storage (6 bytes pour RACE et PROGRAM)
};

extern "C" ANALYZER_EXPORT const char* GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* CreateAnalyzer();
extern "C" ANALYZER_EXPORT void DestroyAnalyzer(Analyzer* analyzer);

#endif //SSD_ANALYZER_H