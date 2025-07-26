#ifndef SSD_ANALYZER_H
#define SSD_ANALYZER_H

#include <Analyzer.h>
#include "SSDAnalyzerResults.h"
#include "SSDSimulationDataGenerator.h"

class SSDAnalyzerSettings;

enum eFrameState {
    FSTATE_INIT,
    FSTATE_PREAMBLE,
    FSTATE_PSBIT,
    FSTATE_CMDBYTE,
    FSTATE_DSBIT,
    FSTATE_DATABYTE,
    FSTATE_CHECKSUM,
    FSTATE_PEBIT
};

enum eSSDMode {
    SSD_MODE_RACE = 0x02,
    SSD_MODE_PROGRAM = 0x01
};

class ANALYZER_EXPORT SSDAnalyzer : public Analyzer2
{
public:
    SSDAnalyzer();
    virtual ~SSDAnalyzer();
    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels);
    virtual U32 GetMinimumSampleRateHz();

    virtual const char *GetAnalyzerName() const;
    virtual bool NeedsRerun();

#pragma warning( push )
#pragma warning( disable : 4251 )

protected: // functions
    UINT LookaheadNextHBit(U64 *nSample);
    UINT GetNextHBit(U64 *nSample);
    UINT GetNextBit(U64 *nSample);
    void PostFrame(U64 nStartSample, U64 nEndSample, eFrameType ft, U8 Flags, U64 Data1, U64 Data2);
    void Setup();
    void DecodeCarData(U8 carData, char* buffer, int bufferSize);

protected: // vars
    std::unique_ptr< SSDAnalyzerSettings > mSettings;
    std::unique_ptr< SSDAnalyzerResults > mResults;
    AnalyzerChannelData *mSSD;

    SSDSimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitilized;

    // Analysis vars:
    U32 mSampleRateHz;
    std::vector<U32> mSampleOffsets;
    BitState mBitLow;
    BitState mBitHigh;
    UINT mMin1hbit;         // Minimum 1 half bit length (microseconds)
    UINT mMax1hbit;         // Maximum 1 half bit length (microseconds)
    UINT mMin0hbit;         // Minimum 0 half bit length (microseconds)
    UINT mMax0hbit;         // Maximum 0 half bit length (microseconds)
    UINT mMaxBitLen;        // overall maximum SSD bit length
    UINT mMinPEHold;        // Minimum time the SSD bitstream has to stay active past the Packet End Bit
    UINT mMaxPGap;          // Maximum Packet Gap

    // SSD specific
    U8 mCurrentMode;        // Current packet mode (RACE or PROGRAM)
    U8 mCarCount;           // Number of cars processed in current packet
    U8 mCarData[6];         // Store car data for checksum calculation
    U8 mCalculatedChecksum; // Running checksum calculation

#pragma warning( pop )
};

extern "C" ANALYZER_EXPORT const char *__cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer *__cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer *analyzer);

#endif //SSD_ANALYZER_H