#include "SSDSimulationDataGenerator.h"
#include "SSDAnalyzerSettings.h"
#include <AnalyzerHelpers.h>

SSDSimulationDataGenerator::SSDSimulationDataGenerator()
{
}

SSDSimulationDataGenerator::~SSDSimulationDataGenerator()
{
}

void SSDSimulationDataGenerator::Initialize(U32 simulation_sample_rate, SSDAnalyzerSettings* settings)
{
    mSimulationSampleRateHz = simulation_sample_rate;
    mSettings = settings;
    mBitLow = BIT_LOW;
    mBitHigh = BIT_HIGH;

    // Calculate timing for SSD protocol
    // Bit 1: ~58μs per half-bit
    // Bit 0: ~110μs per half-bit
    mHBit1Samples = (U64)((double)mSimulationSampleRateHz * HB_1 / 1000000.0);
    mHBit0Samples = (U64)((double)mSimulationSampleRateHz * HB_0 / 1000000.0);
    mIdleSamples = mSimulationSampleRateHz / 100; // 10ms idle between packets

    mClockGenerator.Init(mSimulationSampleRateHz, mSimulationSampleRateHz);
    mSSDSimulationData.SetChannel(mSettings->mInputChannel);
    mSSDSimulationData.SetSampleRate(simulation_sample_rate);
    mSSDSimulationData.SetInitialBitState(mBitHigh);
}

U32 SSDSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels)
{
    U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

    while (mSSDSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested)
    {
        // Generate a variety of SSD packets for testing

        // 1. RACE packet with all cars at different speeds
        CreateSSDRacePacket();
        CreateIdleTime();

        // 2. PROGRAM packet for car ID 3 - CORRECTION: 6 bytes maintenant
        CreateSSDProgramPacket(3);
        CreateSSDProgramPacket(3); // Send twice as required
        CreateIdleTime();

        // 3. PROGRAM packet for car ID 1 - Test supplementaire
        CreateSSDProgramPacket(1);
        CreateSSDProgramPacket(1); // Send twice as required
        CreateIdleTime();

        // 4. RACE packet with all cars stopped
        CreateSSDPreamble(SSD_PREAMBLE_BITS);
        GenerateByte(0); // Start bit
        GenerateByte(CMD_RACE);
        GenerateByte(0); // Start bit

        U8 raceData[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // All cars stopped
        U8 checksum = CalculateChecksum(CMD_RACE, raceData, 6);

        for (int i = 0; i < 6; i++) {
            GenerateByte(raceData[i]);
            GenerateByte(0); // Start bit before next byte
        }
        GenerateByte(checksum);
        CreateIdleTime();

        // 5. RACE packet with all cars at max speed
        CreateSSDPreamble(SSD_PREAMBLE_BITS);
        GenerateByte(0); // Start bit
        GenerateByte(CMD_RACE);
        GenerateByte(0); // Start bit

        U8 raceData2[6] = { 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F }; // All cars max speed
        checksum = CalculateChecksum(CMD_RACE, raceData2, 6);

        for (int i = 0; i < 6; i++) {
            GenerateByte(raceData2[i]);
            GenerateByte(0); // Start bit before next byte
        }
        GenerateByte(checksum);
        CreateIdleTime();

        // 6. Test case: all cars with braking (0x80)
        CreateSSDPreamble(SSD_PREAMBLE_BITS);
        GenerateByte(0); // Start bit
        GenerateByte(CMD_RACE);
        GenerateByte(0); // Start bit

        U8 raceData3[6] = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 }; // All cars braking
        checksum = CalculateChecksum(CMD_RACE, raceData3, 6);

        for (int i = 0; i < 6; i++) {
            GenerateByte(raceData3[i]);
            GenerateByte(0); // Start bit before next byte
        }
        GenerateByte(checksum);
        CreateIdleTime();

        // 7. PROGRAM packet for car ID 6 - Test autre ID
        CreateSSDProgramPacket(6);
        CreateSSDProgramPacket(6); // Send twice as required
        CreateIdleTime();
    }

    *simulation_channels = &mSSDSimulationData;
    return 1;
}

void SSDSimulationDataGenerator::CreateSSDPreamble(U32 nBits)
{
    // Generate preamble: series of '1' bits
    for (U32 i = 0; i < nBits; i++) {
        // Bit '1': Low for HB_1, then High for HB_1
        mSSDSimulationData.Advance(mHBit1Samples);
        mSSDSimulationData.Transition();
        mSSDSimulationData.Advance(mHBit1Samples);
        mSSDSimulationData.Transition();
    }
}

void SSDSimulationDataGenerator::GenerateByte(U8 nVal)
{
    for (int i = 7; i >= 0; i--) {
        U8 bit = (nVal >> i) & 1;
        if (bit == 1) {
            // Bit '1': Low for HB_1, then High for HB_1
            mSSDSimulationData.Advance(mHBit1Samples);
            mSSDSimulationData.Transition();
            mSSDSimulationData.Advance(mHBit1Samples);
            mSSDSimulationData.Transition();
        }
        else {
            // Bit '0': Low for HB_0, then High for HB_0
            mSSDSimulationData.Advance(mHBit0Samples);
            mSSDSimulationData.Transition();
            mSSDSimulationData.Advance(mHBit0Samples);
            mSSDSimulationData.Transition();
        }
    }
}

void SSDSimulationDataGenerator::CreateSSDRacePacket()
{
    CreateSSDPreamble(SSD_PREAMBLE_BITS);
    GenerateByte(0); // Start bit
    GenerateByte(CMD_RACE);
    GenerateByte(0); // Start bit

    // Create sample car data with various speeds and states
    U8 carData[6] = {
        0x3F,  // Car 1: Max speed, no braking, no lane change
        0x53,  // Car 2: Lane change + moderate speed
        0x80,  // Car 3: Braking, minimum power
        0x00,  // Car 4: Stopped
        0x21,  // Car 5: Fast speed
        0x4F   // Car 6: Lane change + slow speed
    };

    U8 checksum = CalculateChecksum(CMD_RACE, carData, 6);

    for (int i = 0; i < 6; i++) {
        GenerateByte(carData[i]);
        GenerateByte(0); // Start bit before next byte
    }
    GenerateByte(checksum);
}

void SSDSimulationDataGenerator::CreateSSDProgramPacket(U8 carId)
{
    CreateSSDPreamble(SSD_PREAMBLE_BITS);
    GenerateByte(0); // Start bit
    GenerateByte(CMD_PROGRAM);
    GenerateByte(0); // Start bit

    // CORRECTION: PROGRAM packet a maintenant 6 bytes identiques (au lieu de 4)
    U8 progData[6] = { carId, carId, carId, carId, carId, carId };
    U8 checksum = CalculateChecksum(CMD_PROGRAM, progData, 6);  // 6 au lieu de 4

    for (int i = 0; i < 6; i++) {  // 6 au lieu de 4
        GenerateByte(progData[i]);
        GenerateByte(0); // Start bit before next byte
    }
    GenerateByte(checksum);
}

void SSDSimulationDataGenerator::CreateIdleTime()
{
    // Create gap between packets (high level)
    mSSDSimulationData.Advance(mIdleSamples);
}

U8 SSDSimulationDataGenerator::CalculateChecksum(U8 command, U8* data, int count)
{
    // PROTOCOLE SSD REEL - Checksum corrige
    // Checksum = 0xFF ⊕ Commande ⊕ Data1 ⊕ Data2 ⊕ ... ⊕ DataN

    U8 checksum = 0xFF;           // Valeur initiale
    checksum ^= command;          // XOR avec la commande

    for (int i = 0; i < count; i++) {
        checksum ^= data[i];      // XOR avec chaque byte de donnees
    }

    return checksum;
}