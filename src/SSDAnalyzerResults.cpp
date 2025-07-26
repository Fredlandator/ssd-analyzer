#include "SSDAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "SSDAnalyzer.h"
#include "SSDAnalyzerSettings.h"
#include <iostream>
#include <sstream>
#include <stdio.h>

SSDAnalyzerResults::SSDAnalyzerResults(SSDAnalyzer *analyzer, SSDAnalyzerSettings *settings)
    :   AnalyzerResults(),
        mSettings(settings),
        mAnalyzer(analyzer)
{
}

SSDAnalyzerResults::~SSDAnalyzerResults()
{
}

const char* SSDAnalyzerResults::GetCommandName(U8 command)
{
    switch(command) {
        case 0x01: return "PROGRAM";
        case 0x02: return "RACE";
        default: return "UNKNOWN";
    }
}

void SSDAnalyzerResults::DecodeCarData(U8 carData, char* buffer, int bufferSize)
{
    bool braking = (carData & 0x80) != 0;
    bool lane_change = (carData & 0x40) != 0;
    U8 speed_power = carData & 0x3F;
    
    if (braking) {
        if (speed_power <= 3) {
            snprintf(buffer, bufferSize, "B:P%d L:%s", speed_power + 1, 
                    lane_change ? "CHG" : "---");
        } else {
            snprintf(buffer, bufferSize, "B:P%d L:%s", speed_power, 
                    lane_change ? "CHG" : "---");
        }
    } else {
        snprintf(buffer, bufferSize, "S:%d L:%s", speed_power, 
                lane_change ? "CHG" : "---");
    }
}

void SSDAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel & /*channel*/, DisplayBase display_base)
{
    ClearResultStrings();
    Frame frame = GetFrame(frame_index);

    bool bit_error = ((frame.mFlags & BIT_ERROR_FLAG) != 0);
    bool framing_error = ((frame.mFlags & FRAMING_ERROR_FLAG) != 0);
    bool packet_error = ((frame.mFlags & PACKET_ERROR_FLAG) != 0);
    bool checksum_error = ((frame.mFlags & CHECKSUM_ERROR_FLAG) != 0);

    char result_str[128];
    char detail_str[64];
    
    switch ((eFrameType)frame.mType)
    {
    case FRAME_PREAMBLE:
        AddResultString("P");
        AddResultString("PREAMBLE");
        snprintf(result_str, sizeof(result_str), "%llu Preamble Bits", frame.mData1);
        AddResultString(result_str);
        break;
        
    case FRAME_PSBIT:
        AddResultString("S");
        AddResultString("START");
        AddResultString("Packet Start Bit");
        break;
        
    case FRAME_CMDBYTE:
        AddResultString("C");
        AddResultString("CMD");
        snprintf(result_str, sizeof(result_str), "Command: %s (%#02llx)", 
                GetCommandName((U8)frame.mData1), frame.mData1);
        AddResultString(result_str);
        break;
        
    case FRAME_DSBIT:
        AddResultString("S");
        AddResultString("START");
        AddResultString("Data Start Bit");
        break;
        
    case FRAME_CARDATA:
        if (mSettings->mShowCarDetails) {
            DecodeCarData((U8)frame.mData1, detail_str, sizeof(detail_str));
            snprintf(result_str, sizeof(result_str), "Car%lld: %s", frame.mData2, detail_str);
            AddResultString("C");
            AddResultString(result_str);
            AddResultString(result_str);
        } else {
            AddResultString("D");
            AddResultString("DATA");
            snprintf(result_str, sizeof(result_str), "Car %lld Data: %#02llx", frame.mData2, frame.mData1);
            AddResultString(result_str);
        }
        break;
        
    case FRAME_CHECKSUM:
        if (checksum_error) {
            AddResultString("X");
            AddResultString("CHK ERR");
            snprintf(result_str, sizeof(result_str), "Checksum Error: %#02llx", frame.mData1);
        } else {
            AddResultString("✓");
            AddResultString("CHK OK");
            snprintf(result_str, sizeof(result_str), "Checksum OK: %#02llx", frame.mData1);
        }
        AddResultString(result_str);
        break;
        
    case FRAME_PEBIT:
        AddResultString("E");
        AddResultString("END");
        AddResultString("Packet End");
        break;
        
    case FRAME_ERR:
    default:
        AddResultString("X");
        AddResultString("ERROR");
        if (bit_error) {
            AddResultString("Bit Timing Error");
        } else if (framing_error) {
            AddResultString("Framing Error");
        } else {
            AddResultString("Protocol Error");
        }
        break;
    }
}

void SSDAnalyzerResults::GenerateExportFile(const char *file, DisplayBase display_base, U32 /*export_type_user_id*/)
{
    std::stringstream ss;

    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate = mAnalyzer->GetSampleRate();
    U64 num_frames = GetNumFrames();

    void *f = AnalyzerHelpers::StartFile(file);

    ss << "Time [s],Type,Data,Hex,Details" << std::endl;

    for (U32 i = 0; i < num_frames; i++) {
        Frame frame = GetFrame(i);

        char time_str[128];
        AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);

        char number_str[128];
        AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);

        ss << time_str << ",";

        switch ((eFrameType)frame.mType) {
        case FRAME_PREAMBLE:
            ss << "PREAMBLE," << frame.mData1 << "," << number_str << ",Preamble bits";
            break;
        case FRAME_PSBIT:
            ss << "START_BIT,0,0x00,Packet start";
            break;
        case FRAME_CMDBYTE:
            ss << "COMMAND," << frame.mData1 << "," << number_str << "," << GetCommandName((U8)frame.mData1);
            break;
        case FRAME_DSBIT:
            ss << "START_BIT,0,0x00,Data start";
            break;
        case FRAME_CARDATA:
            {
                char detail_str[64];
                DecodeCarData((U8)frame.mData1, detail_str, sizeof(detail_str));
                ss << "CAR_DATA," << frame.mData1 << "," << number_str << ",Car" << frame.mData2 << ": " << detail_str;
            }
            break;
        case FRAME_CHECKSUM:
            ss << "CHECKSUM," << frame.mData1 << "," << number_str;
            if ((frame.mFlags & CHECKSUM_ERROR_FLAG) != 0) {
                ss << ",Checksum ERROR";
            } else {
                ss << ",Checksum OK";
            }
            break;
        case FRAME_PEBIT:
            ss << "END_BIT,0,0x00,Packet end";
            break;
        default:
            ss << "ERROR,0,0x00,";
            if ((frame.mFlags & BIT_ERROR_FLAG) != 0) {
                ss << "Bit timing error";
            } else if ((frame.mFlags & FRAMING_ERROR_FLAG) != 0) {
                ss << "Framing error";
            } else {
                ss << "Protocol error";
            }
            break;
        }

        ss << std::endl;

        AnalyzerHelpers::AppendToFile((U8 *)ss.str().c_str(), ss.str().length(), f);
        ss.str(std::string());

        if (UpdateExportProgressAndCheckForCancel(i, num_frames) == true) {
            AnalyzerHelpers::EndFile(f);
            return;
        }
    }

    UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
    AnalyzerHelpers::EndFile(f);
}

void SSDAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
    char result_str[128];
    char detail_str[64];

    ClearTabularText();
    Frame frame = GetFrame(frame_index);
    
    bool bit_error = ((frame.mFlags & BIT_ERROR_FLAG) != 0);
    bool framing_error = ((frame.mFlags & FRAMING_ERROR_FLAG) != 0);
    bool packet_error = ((frame.mFlags & PACKET_ERROR_FLAG) != 0);
    bool checksum_error = ((frame.mFlags & CHECKSUM_ERROR_FLAG) != 0);

    switch ((eFrameType)frame.mType)
    {
    case FRAME_PREAMBLE:
        snprintf(result_str, sizeof(result_str), "Preamble: %llu bits", frame.mData1);
        break;
    case FRAME_PSBIT:
        snprintf(result_str, sizeof(result_str), "Packet Start Bit");
        break;
    case FRAME_CMDBYTE:
        snprintf(result_str, sizeof(result_str), "Command: %s (%#02llx)", 
                GetCommandName((U8)frame.mData1), frame.mData1);
        break;
    case FRAME_DSBIT:
        snprintf(result_str, sizeof(result_str), "Data Start Bit");
        break;
    case FRAME_CARDATA:
        DecodeCarData((U8)frame.mData1, detail_str, sizeof(detail_str));
        snprintf(result_str, sizeof(result_str), "Car %lld Data (%#02llx): %s", 
                frame.mData2, frame.mData1, detail_str);
        break;
    case FRAME_CHECKSUM:
        snprintf(result_str, sizeof(result_str), "Checksum: %#02llx %s", 
                frame.mData1, checksum_error ? "[ERROR]" : "[OK]");
        break;
    case FRAME_PEBIT:
        snprintf(result_str, sizeof(result_str), "Packet End Bit");
        break;
    default:
        if (bit_error) {
            snprintf(result_str, sizeof(result_str), "Bit Timing Error");
        } else if (framing_error) {
            snprintf(result_str, sizeof(result_str), "Framing Error");
        } else {
            snprintf(result_str, sizeof(result_str), "Protocol Error");
        }
        break;
    }
    
    AddTabularText(result_str);
    
    // Add error flags if present
    if (bit_error || framing_error || packet_error || checksum_error) {
        snprintf(result_str, sizeof(result_str), " [%s%s%s%s]", 
                bit_error ? "B" : "", 
                framing_error ? "F" : "", 
                packet_error ? "P" : "",
                checksum_error ? "C" : "");
        AddTabularText(result_str);
    }
}

void SSDAnalyzerResults::GeneratePacketTabularText(U64 /*packet_id*/, DisplayBase /*display_base*/)
{
    // Not implemented for SSD protocol
}

void SSDAnalyzerResults::GenerateTransactionTabularText(U64 /*transaction_id*/, DisplayBase /*display_base*/)
{
    // Not implemented for SSD protocol
}