#include "SSDAnalyzer.h"
#include "SSDAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <math.h>
#include <stdio.h>

SSDAnalyzer::SSDAnalyzer()
    : Analyzer2(),
      mSettings(new SSDAnalyzerSettings()),
      mSimulationInitilized(false)
{
    SetAnalyzerSettings(mSettings.get());
}

SSDAnalyzer::~SSDAnalyzer()
{
    KillThread();
}

void SSDAnalyzer::SetupResults()
{
    mResults.reset(new SSDAnalyzerResults(this, mSettings.get()));
    SetAnalyzerResults(mResults.get());
    mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannel);
}

UINT SSDAnalyzer::LookaheadNextHBit(U64 *nSample)
{
    UINT nHBitLen = (UINT)(mSSD->GetSampleOfNextEdge() - *nSample);
    *nSample = mSSD->GetSampleOfNextEdge();
    
    if (nHBitLen >= mMin1hbit && nHBitLen <= mMax1hbit)
        return 1;
    else if (nHBitLen >= mMin0hbit && nHBitLen <= mMax0hbit)
        return 0;
    else if (nHBitLen >= mMinPEHold && nHBitLen <= mMaxPGap) {
        return 3; // Packet gap
    } else
        return BIT_ERROR_FLAG; // bit error
}

UINT SSDAnalyzer::GetNextHBit(U64 *nSample)
{
    U64 nSampNumber = *nSample;
    mSSD->AdvanceToNextEdge();
    *nSample = mSSD->GetSampleNumber();
    UINT nHBitLen = (UINT)(mSSD->GetSampleNumber() - nSampNumber);
    
    if (nHBitLen >= mMin1hbit && nHBitLen <= mMax1hbit)
        return 1;
    else if (nHBitLen >= mMin0hbit && nHBitLen <= mMax0hbit)
        return 0;
    else 
        return BIT_ERROR_FLAG; // bit error
}

UINT SSDAnalyzer::GetNextBit(U64 *nSample)
{
    U64 nTemp = *nSample;
    UINT nHBit1 = GetNextHBit(nSample);
    UINT nHBit2 = GetNextHBit(nSample);
    
    if ((nHBit1 > 1) || (nHBit2 > 1) || ((UINT)(*nSample - nTemp) > mMaxBitLen))
        return BIT_ERROR_FLAG;      // bit error
    else if (nHBit1 != nHBit2)
        return FRAMING_ERROR_FLAG;  // frame error
    else 
        return nHBit1;
}

void SSDAnalyzer::PostFrame(U64 nStartSample, U64 nEndSample, eFrameType ft, U8 Flags, U64 Data1 = 0, U64 Data2 = 0)
{
    Frame frame;
    frame.mStartingSampleInclusive = nStartSample;
    frame.mEndingSampleInclusive = nEndSample;
    frame.mData1 = Data1;
    frame.mData2 = Data2;
    frame.mType = ft;
    frame.mFlags = Flags;
    mResults->AddFrame(frame);

    // FrameV2 for modern Saleae Logic 2 interface
    FrameV2 framev2;

    switch (ft)
    {
    case FRAME_PREAMBLE:
        framev2.AddString("type", "preamble");                        
        framev2.AddByte("length", (U8)Data1);
        mResults->AddFrameV2(framev2, "preamble", nStartSample, nEndSample);
        break;
    case FRAME_PSBIT:
        framev2.AddString("type", "start_bit");                        
        mResults->AddFrameV2(framev2, "packet_start", nStartSample, nEndSample);
        break;
    case FRAME_CMDBYTE:
        framev2.AddString("type", "command");
        framev2.AddByte("data", (U8)Data1);
        if (Data1 == SSD_MODE_RACE) {
            framev2.AddString("mode", "RACE");
            mResults->AddFrameV2(framev2, "RACE_cmd", nStartSample, nEndSample);
        } else if (Data1 == SSD_MODE_PROGRAM) {
            framev2.AddString("mode", "PROGRAM");
            mResults->AddFrameV2(framev2, "PROG_cmd", nStartSample, nEndSample);
        } else {
            framev2.AddString("mode", "UNKNOWN");
            mResults->AddFrameV2(framev2, "UNK_cmd", nStartSample, nEndSample);
        }
        break;
    case FRAME_DSBIT:
        framev2.AddString("type", "start_bit");                        
        mResults->AddFrameV2(framev2, "data_start", nStartSample, nEndSample);
        break;
    case FRAME_CARDATA:
        framev2.AddString("type", "car_data");
        framev2.AddByte("data", (U8)Data1);
        framev2.AddByte("car_id", (U8)Data2);
        
        // Decode car data details
        bool braking = (Data1 & 0x80) != 0;
        bool lane_change = (Data1 & 0x40) != 0;
        U8 speed_power = Data1 & 0x3F;
        
        framev2.AddBool("braking", braking);
        framev2.AddBool("lane_change", lane_change);
        framev2.AddByte("speed_power", speed_power);
        
        char car_label[16];
        snprintf(car_label, sizeof(car_label), "car_%d", (int)Data2);
        mResults->AddFrameV2(framev2, car_label, nStartSample, nEndSample);
        break;
    case FRAME_CHECKSUM:
        framev2.AddString("type", "checksum");
        framev2.AddByte("data", (U8)Data1);
        framev2.AddBool("valid", (Flags & CHECKSUM_ERROR_FLAG) == 0);
        mResults->AddFrameV2(framev2, "checksum", nStartSample, nEndSample);
        break;
    case FRAME_PEBIT:
        framev2.AddString("type", "end_bit");                        
        mResults->AddFrameV2(framev2, "packet_end", nStartSample, nEndSample);
        break;
    default:
        framev2.AddString("type", "error");                        
        mResults->AddFrameV2(framev2, "error", nStartSample, nEndSample);
        break;
    }

    mResults->CommitResults();
}

void SSDAnalyzer::Setup()
{
    // Sample Rate
    mSampleRateHz = GetSampleRate();
    double dSamplesPerMicrosecond = (mSampleRateHz / 1000000.0);
    
    // Use the mCalPPM setting to adjust the resolution of the measurements
    double dMaxCorrection = 1.0 + mSettings->mCalPPM / 1000000.0;
    double dMinCorrection = 1.0 - mSettings->mCalPPM / 1000000.0;
    
    // SSD Protocol timing (from specification)
    // Bit 1: ~58μs per half-bit (total ~116μs)
    // Bit 0: ~110μs per half-bit (total ~220μs)
    
    mMaxBitLen = round(12000.0 * dSamplesPerMicrosecond * dMaxCorrection);
    mMinPEHold = round(26.0 * dSamplesPerMicrosecond * dMinCorrection);
    mMaxPGap = round(30000.0 * dSamplesPerMicrosecond * dMaxCorrection);
    
    if (mSettings->mMode == SSDAnalyzerEnums::MODE_TOLERANT) {
        // More tolerant timing for noisy signals
        mMin1hbit = round(50.0 * dSamplesPerMicrosecond * dMinCorrection);
        mMax1hbit = round(70.0 * dSamplesPerMicrosecond * dMaxCorrection);
        mMin0hbit = round(100.0 * dSamplesPerMicrosecond * dMinCorrection);
        mMax0hbit = round(130.0 * dSamplesPerMicrosecond * dMaxCorrection);
    } else {
        // Standard timing as per specification
        mMin1hbit = round(52.0 * dSamplesPerMicrosecond * dMinCorrection);  // ~58μs ±10%
        mMax1hbit = round(64.0 * dSamplesPerMicrosecond * dMaxCorrection);
        mMin0hbit = round(99.0 * dSamplesPerMicrosecond * dMinCorrection);  // ~110μs ±10%
        mMax0hbit = round(121.0 * dSamplesPerMicrosecond * dMaxCorrection);
    }

    mSSD = GetAnalyzerChannelData(mSettings->mInputChannel);
}

void SSDAnalyzer::DecodeCarData(U8 carData, char* buffer, int bufferSize)
{
    bool braking = (carData & 0x80) != 0;
    bool lane_change = (carData & 0x40) != 0;
    U8 speed_power = carData & 0x3F;
    
    if (braking) {
        snprintf(buffer, bufferSize, "BRAKE=%d LANE=%s", 
                speed_power, lane_change ? "CHG" : "---");
    } else {
        snprintf(buffer, bufferSize, "SPEED=%d LANE=%s", 
                speed_power, lane_change ? "CHG" : "---");
    }
}

void SSDAnalyzer::WorkerThread()
{
    Setup();
    U32 nHBitCnt = 0;
    U8  nHBitVal = 0;
    U32 nBits = 0;
    U8  nVal = 0;
    U8  nChecksum = 0;
    eFrameState ef = FSTATE_INIT;
    U64 nFrameStart = mSSD->GetSampleNumber();
    U64 nCurSample = nFrameStart;
    U64 nPreambleStart = 0;
    U64 nBitStartSample = 0;
    U64 nTemp = nCurSample;
    
    // SSD specific variables
    mCurrentMode = 0;
    mCarCount = 0;
    mCalculatedChecksum = 0;
    
    for (;;) {
        nBitStartSample = nCurSample;
        
        switch (ef)
        {
        case FSTATE_INIT:
            nHBitVal = GetNextHBit(&nCurSample);
            switch (nHBitVal)
            {
            case 1:
                ++nHBitCnt;
                if (nHBitCnt == (mSettings->mPreambleBits * 2)) {
                    ef = FSTATE_PREAMBLE;
                }
                break;
            default:
                nHBitCnt = 0;
                nFrameStart = nCurSample + 1;
                nPreambleStart = nFrameStart;
                break;
            }
            break;

        case FSTATE_PREAMBLE:
            nTemp = nCurSample;
            nHBitVal = LookaheadNextHBit(&nTemp);
            switch(nHBitVal) {
            case 0: // Start bit ends preamble
                PostFrame(nPreambleStart, nCurSample, FRAME_PREAMBLE, 0, nHBitCnt / 2);
                ReportProgress(nCurSample);
                nFrameStart = nCurSample + 1;
                ef = FSTATE_PSBIT;
                break;
            case 1:
                nHBitVal = GetNextHBit(&nCurSample);
                ++nHBitCnt;
                break;
            default:
                PostFrame(nBitStartSample, nCurSample, FRAME_ERR, nHBitVal);
                mResults->AddMarker(nBitStartSample, AnalyzerResults::ErrorSquare, mSettings->mInputChannel);
                mResults->AddMarker(nCurSample, AnalyzerResults::ErrorX, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nHBitCnt = 0;
                ef = FSTATE_INIT;
            }
            break;

        case FSTATE_PSBIT:
            nHBitVal = GetNextBit(&nCurSample);
            nHBitCnt = 0;
            if (nHBitVal == 0) { // Packet start bit
                PostFrame(nFrameStart, nCurSample, FRAME_PSBIT, 0);
                mResults->AddMarker(nFrameStart, AnalyzerResults::Start, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nBits = nVal = 0;
                mCalculatedChecksum = 0;
                nFrameStart = nCurSample + 1;
                ef = FSTATE_CMDBYTE;
            } else {
                PostFrame(nBitStartSample, nCurSample, FRAME_ERR, FRAMING_ERROR_FLAG);
                mResults->AddMarker(nBitStartSample, AnalyzerResults::ErrorDot, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nHBitCnt = 0;
                ef = FSTATE_INIT;
            }
            break;

        case FSTATE_CMDBYTE:
            switch (nHBitVal = GetNextBit(&nCurSample))
            {
            case 0:
            case 1:
                nVal <<= 1;
                nVal |= nHBitVal;
                nBits++;
                if (nBits == 8)
                {
                    mCurrentMode = nVal;
                    mCarCount = 0;
                    PostFrame(nFrameStart, nCurSample, FRAME_CMDBYTE, 0, nVal);
                    ReportProgress(nCurSample);
                    nFrameStart = nCurSample + 1;
                    ef = FSTATE_DSBIT;
                }
                break;
            default:
                PostFrame(nBitStartSample, nCurSample, FRAME_ERR, nHBitVal);
                mResults->AddMarker(nBitStartSample, AnalyzerResults::ErrorSquare, mSettings->mInputChannel);
                mResults->AddMarker(nCurSample, AnalyzerResults::ErrorX, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nHBitCnt = 0;
                ef = FSTATE_INIT;
            }
            break;

        case FSTATE_DSBIT:
            nHBitVal = GetNextBit(&nCurSample);
            if (nHBitVal == 0) { // Data start bit
                PostFrame(nFrameStart, nCurSample, FRAME_DSBIT, 0);
                mResults->AddMarker(nFrameStart, AnalyzerResults::Start, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nBits = nVal = 0;
                nFrameStart = nCurSample + 1;
                ef = FSTATE_DATABYTE;
            } else {
                PostFrame(nBitStartSample, nCurSample, FRAME_ERR, FRAMING_ERROR_FLAG);
                mResults->AddMarker(nBitStartSample, AnalyzerResults::ErrorDot, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nHBitCnt = 0;
                ef = FSTATE_INIT;
            }
            break;

        case FSTATE_DATABYTE:
            switch (nHBitVal = GetNextBit(&nCurSample))
            {
            case 0:
            case 1:
                nVal <<= 1;
                nVal |= nHBitVal;
                nBits++;
                if (nBits == 8)
                {
                    mCarData[mCarCount] = nVal;
                    mCalculatedChecksum ^= nVal;
                    
                    nTemp = nCurSample;
                    UINT nLABit = LookaheadNextHBit(&nTemp);
                    
                    // Determine if this is the last data byte or if more follow
                    bool isLastByte = false;
                    
                    if (mCurrentMode == SSD_MODE_RACE) {
                        // In RACE mode, expect 6 car data bytes
                        isLastByte = (mCarCount >= 5);
                    } else if (mCurrentMode == SSD_MODE_PROGRAM) {
                        // In PROGRAM mode, expect 4 identical bytes
                        isLastByte = (mCarCount >= 3);
                    }
                    
                    if (isLastByte && nLABit == 1) {
                        // This is the last data byte, next should be checksum
                        PostFrame(nFrameStart, nCurSample, FRAME_CARDATA, 0, nVal, mCarCount + 1);
                        ef = FSTATE_CHECKSUM;
                    } else if (!isLastByte && nLABit == 0) {
                        // More data bytes to follow
                        PostFrame(nFrameStart, nCurSample, FRAME_CARDATA, 0, nVal, mCarCount + 1);
                        mCarCount++;
                        ef = FSTATE_DSBIT;
                    } else {
                        // Unexpected sequence
                        PostFrame(nBitStartSample, nCurSample, FRAME_ERR, FRAMING_ERROR_FLAG);
                        mResults->AddMarker(nBitStartSample, AnalyzerResults::ErrorDot, mSettings->mInputChannel);
                        ReportProgress(nCurSample);
                        nHBitCnt = 0;
                        ef = FSTATE_INIT;
                        break;
                    }
                    
                    ReportProgress(nCurSample);
                    nFrameStart = nCurSample + 1;
                    nBits = nVal = 0;
                }
                break;
            default:
                PostFrame(nBitStartSample, nCurSample, FRAME_ERR, nHBitVal);
                mResults->AddMarker(nBitStartSample, AnalyzerResults::ErrorSquare, mSettings->mInputChannel);
                mResults->AddMarker(nCurSample, AnalyzerResults::ErrorX, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nHBitCnt = 0;
                ef = FSTATE_INIT;
            }
            break;

        case FSTATE_CHECKSUM:
            switch (nHBitVal = GetNextBit(&nCurSample))
            {
            case 0:
            case 1:
                nVal <<= 1;
                nVal |= nHBitVal;
                nBits++;
                if (nBits == 8)
                {
                    U8 flags = 0;
                    if (nVal != mCalculatedChecksum) {
                        flags |= CHECKSUM_ERROR_FLAG;
                        mResults->AddMarker(nFrameStart, AnalyzerResults::ErrorX, mSettings->mInputChannel);
                    }
                    
                    PostFrame(nFrameStart, nCurSample, FRAME_CHECKSUM, flags, nVal);
                    ReportProgress(nCurSample);
                    nFrameStart = nCurSample + 1;
                    ef = FSTATE_PEBIT;
                    nBits = nVal = 0;
                }
                break;
            default:
                PostFrame(nBitStartSample, nCurSample, FRAME_ERR, nHBitVal);
                mResults->AddMarker(nBitStartSample, AnalyzerResults::ErrorSquare, mSettings->mInputChannel);
                mResults->AddMarker(nCurSample, AnalyzerResults::ErrorX, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nHBitCnt = 0;
                ef = FSTATE_INIT;
            }
            break;

        case FSTATE_PEBIT:
            // In SSD, after checksum we should see the packet gap (high level hold)
            nTemp = nCurSample;
            if (LookaheadNextHBit(&nTemp) == 3) { // Packet gap detected
                PostFrame(nFrameStart, nCurSample, FRAME_PEBIT, 0);
                mResults->AddMarker(nFrameStart, AnalyzerResults::Stop, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nFrameStart = nCurSample + 1;
                nPreambleStart = nFrameStart;
                nHBitCnt = 0;
                ef = FSTATE_INIT;
            } else {
                // Immediate transition to next packet (or error)
                PostFrame(nFrameStart, nCurSample, FRAME_PEBIT, 0);
                mResults->AddMarker(nFrameStart, AnalyzerResults::Stop, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nFrameStart = nCurSample + 1;
                nPreambleStart = nFrameStart;
                nHBitCnt = 0;
                ef = FSTATE_INIT;
            }
            break;

        default:
            nHBitCnt = 0;
            ef = FSTATE_INIT;
        }
        CheckIfThreadShouldExit();
    }
}

bool SSDAnalyzer::NeedsRerun()
{
    return false;
}

U32 SSDAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor **simulation_channels)
{
    if (mSimulationInitilized == false) {
        mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 SSDAnalyzer::GetMinimumSampleRateHz()
{
    return 1000000; // 1 MHz minimum for proper SSD decoding
}

const char *SSDAnalyzer::GetAnalyzerName() const
{
    return "SSD";
}

const char *GetAnalyzerName()
{
    return "SSD";
}

Analyzer *CreateAnalyzer()
{
    return new SSDAnalyzer();
}

void DestroyAnalyzer(Analyzer *analyzer)
{
    delete analyzer;
}