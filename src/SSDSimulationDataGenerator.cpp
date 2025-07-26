#include "SSDSimulationDataGenerator.h"
#include "SSDAnalyzerSettings.h"

SSDSimulationDataGenerator::SSDSimulationDataGenerator()
{
}

SSDSimulationDataGenerator::~SSDSimulationDataGenerator()
{
}

void SSDSimulationDataGenerator::Initialize(U32 simulation_sample_rate, SSDAnalyzerSettings *settings)
{
    mSimulationSampleRateHz = simulation_sample_rate;
    mSettings = settings;

    mClockGenerator.Init(10000.0, simulation_sample_rate);
    mSSDSimulationData.SetChannel(mSettings->mInputChannel);
    mSSDSimulationData.SetSampleRate(simulation_sample_rate);

    mBitLow = BIT_LOW;
    mBitHigh = BIT_HIGH;
    mHBit1Samples = HB_1 * simulation_sample_rate / 1000000.0;
    mHBit0Samples = HB_0 * simulation_sample_rate / 1000000.0;
    mIdleSamples = 1000.0 * simulation_sample_rate / 1000000.0; // 1ms idle between packets

    mSSDSimulationData.SetInitialBitState(mBitHigh);
    mSSDSimulationData.Advance(10 * mHBit1Samples);  // Insert initial idle time

    mValue = 0;
}

U32 SSDSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, 
        SimulationChannelDescriptor **simulation_channels)
{
    U64 adjusted_largest_sample_requested = 
        AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

    while (mSSDSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested) {
        // Generate a RACE packet with sample car data
        CreateSSDRacePacket();
        CreateIdleTime();
        
        // Generate a PROGRAM packet for car ID 3
        CreateSSDProgramPacket(3);
        CreateSSDProgramPacket(3); // Program packets are sent twice
        CreateIdleTime();
        
        // Generate another RACE packet with different data
        CreateSSDRacePacket();
        CreateIdleTime();
    }
        
    *simulation_channels = &mSSDSimulationData;
    return 1;
}

void SSDSimulationDataGenerator::CreateSSDPreamble(U32 nBits)
{
    // Generate preamble: all 1 bits
    for (UINT i = 0; i < (nBits * 2); i++)
    {
        mSSDSimulationData.Transition();  // flip bit
        mSSDSimulationData.Advance(mHBit1Samples);    // add half-bit time
    }
}

void SSDSimulationDataGenerator::GenerateByte(U8 nVal)
{
    // Start bit (0)
    mSSDSimulationData.Transition();  // flip bit
    mSSDSimulationData.Advance(mHBit0Samples);    // add half-bit time
    mSSDSimulationData.Transition();  // flip bit
    mSSDSimulationData.Advance(mHBit0Samples);    // add half-bit time

    // Data bits (MSB first)
    for (int i = 0; i < 8; i++)
    {
        if ((nVal & 0x80) != 0)
        {
            // Bit 1
            mSSDSimulationData.Transition();  // flip bit
            mSSDSimulationData.Advance(mHBit1Samples);    // add half-bit time
            mSSDSimulationData.Transition();  // flip bit
            mSSDSimulationData.Advance(mHBit1Samples);    // add half-bit time
        } 
        else    
        {
            // Bit 0
            mSSDSimulationData.Transition();  // flip bit
            mSSDSimulationData.Advance(mHBit0Samples);    // add half-bit time
            mSSDSimulationData.Transition();  // flip bit
            mSSDSimulationData.Advance(mHBit0Samples);    // add half-bit time
        }
        nVal <<= 1;
    }
}

U8 SSDSimulationDataGenerator::CalculateChecksum(U8* data, int count)
{
    U8 checksum = 0;
    for (int i = 0; i < count; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

void SSDSimulationDataGenerator::CreateSSDRacePacket()
{
    // Sample car data for simulation
    U8 carData[6] = {
        0x3F,  // Car 1: No brake, no lane change, max speed (63)
        0x80,  // Car 2: Brake active, no lane change, power 1
        0x40,  // Car 3: No brake, lane change, speed 0
        0x1F,  // Car 4: No brake, no lane change, speed 31
        0x00,  // Car 5: No brake, no lane change, speed 0
        0x20   // Car 6: No brake, no lane change, speed 32
    };
    
    // Generate preamble
    CreateSSDPreamble(SSD_PREAMBLE_BITS);
    
    // Generate command byte (RACE = 0x02)
    GenerateByte(CMD_RACE);
    
    // Generate car data
    for (int i = 0; i < 6; i++) {
        GenerateByte(carData[i]);
    }
    
    // Generate checksum
    U8 checksum = CalculateChecksum(carData, 6);
    GenerateByte(checksum);
}

void SSDSimulationDataGenerator::CreateSSDProgramPacket(U8 carId)
{
    // Program packet data (4 identical bytes)
    U8 progData[4] = {carId, carId, carId, carId};
    
    // Generate preamble
    CreateSSDPreamble(SSD_PREAMBLE_BITS);
    
    // Generate command byte (PROGRAM = 0x01)
    GenerateByte(CMD_PROGRAM);
    
    // Generate 4 identical car ID bytes
    for (int i = 0; i < 4; i++) {
        GenerateByte(progData[i]);
    }
    
    // Generate checksum
    U8 checksum = CalculateChecksum(progData, 4);
    GenerateByte(checksum);
}

void SSDSimulationDataGenerator::CreateIdleTime()
{
    // Create idle time between packets (high level)
    mSSDSimulationData.Advance(mIdleSamples);
}