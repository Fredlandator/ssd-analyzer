#ifndef SSD_SIMULATION_DATA_GENERATOR
#define SSD_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>

typedef unsigned int UINT;

class SSDAnalyzerSettings;

enum SSD_Cmd_t { CMD_PROGRAM = 0x01, CMD_RACE = 0x02 };
const U32 SSD_PREAMBLE_BITS = 14;
const double HB_1 = 58.0;   // Half-bit 1 duration in microseconds
const double HB_0 = 110.0;  // Half-bit 0 duration in microseconds

class SSDSimulationDataGenerator
{
public:
    SSDSimulationDataGenerator();
    ~SSDSimulationDataGenerator();

    void Initialize(U32 simulation_sample_rate, SSDAnalyzerSettings* settings);
    U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);

protected:
    SSDAnalyzerSettings* mSettings;
    U32 mSimulationSampleRateHz;
    BitState mBitLow;
    BitState mBitHigh;
    U64 mValue;

    // SSD specific functions
    void GenerateByte(U8 nVal);
    void CreateSSDPreamble(U32 nBits);
    void CreateSSDRacePacket();
    void CreateSSDProgramPacket(U8 carId);  // PROGRAM: genere 6 bytes identiques maintenant
    void CreateIdleTime();

    // Signature corrigee avec 3 parametres
    U8 CalculateChecksum(U8 command, U8* data, int count);

    ClockGenerator mClockGenerator;
    SimulationChannelDescriptor mSSDSimulationData;
    U64 mHBit1Samples;
    U64 mHBit0Samples;
    U64 mIdleSamples;
};

#endif //SSD_SIMULATION_DATA_GENERATOR