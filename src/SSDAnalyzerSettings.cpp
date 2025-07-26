#include "SSDAnalyzerSettings.h"
#include <AnalyzerHelpers.h>
#include <sstream>
#include <cstring>

#define CHANNEL_NAME "SSD Signal"

SSDAnalyzerSettings::SSDAnalyzerSettings()
    : mInputChannel(UNDEFINED_CHANNEL),
      mPreambleBits(14),
      mMode(SSDAnalyzerEnums::MODE_STANDARD),
      mCalPPM(0),
      mShowCarDetails(true)
{
    mInputChannelInterface.reset(new AnalyzerSettingInterfaceChannel());
    mInputChannelInterface->SetTitleAndTooltip(CHANNEL_NAME, "SSD Protocol Signal Input");
    mInputChannelInterface->SetChannel(mInputChannel);
    AddInterface(mInputChannelInterface.get());

    mPreambleBitsInterface.reset(new AnalyzerSettingInterfaceInteger());
    mPreambleBitsInterface->SetTitleAndTooltip("Preamble Size", "Minimum preamble bit length (12-22 bits)");
    mPreambleBitsInterface->SetMin(8);
    mPreambleBitsInterface->SetMax(22);
    mPreambleBitsInterface->SetInteger(mPreambleBits);
    AddInterface(mPreambleBitsInterface.get());

    mModeInterface.reset(new AnalyzerSettingInterfaceNumberList());
    mModeInterface->SetTitleAndTooltip("Timing Mode", "Bit timing tolerance mode");
    mModeInterface->ClearNumbers();
    mModeInterface->AddNumber(SSDAnalyzerEnums::MODE_STANDARD, "Standard", "Standard SSD timing tolerance");
    mModeInterface->AddNumber(SSDAnalyzerEnums::MODE_TOLERANT, "Tolerant", "More tolerant timing for noisy signals");
    mModeInterface->SetNumber(mMode);
    AddInterface(mModeInterface.get());
    
    mCalPPMInterface.reset(new AnalyzerSettingInterfaceInteger());
    mCalPPMInterface->SetTitleAndTooltip("Calibration Factor [+/-PPM]", "Bit timing limits error correction (-100000 to +100000 PPM)");
    mCalPPMInterface->SetMin(-100000);
    mCalPPMInterface->SetMax(100000);
    mCalPPMInterface->SetInteger(mCalPPM);
    AddInterface(mCalPPMInterface.get());

    mShowCarDetailsInterface.reset(new AnalyzerSettingInterfaceBool());
    mShowCarDetailsInterface->SetTitleAndTooltip("Show Car Details", "Display detailed car data (brake, speed, lane change)");
    mShowCarDetailsInterface->SetValue(mShowCarDetails);
    AddInterface(mShowCarDetailsInterface.get());

    AddExportOption(0, "Export as text/csv file");
    AddExportExtension(0, "Text file", "txt");
    AddExportExtension(0, "CSV file", "csv");

    ClearChannels();
    AddChannel(mInputChannel, CHANNEL_NAME, false);
}

SSDAnalyzerSettings::~SSDAnalyzerSettings()
{
}

bool SSDAnalyzerSettings::SetSettingsFromInterfaces()
{
    mInputChannel = mInputChannelInterface->GetChannel();
    mPreambleBits = mPreambleBitsInterface->GetInteger();
    mMode = (SSDAnalyzerEnums::eAnalyzerMode)(int)mModeInterface->GetNumber();
    mCalPPM = mCalPPMInterface->GetInteger();
    mShowCarDetails = mShowCarDetailsInterface->GetValue();
    
    ClearChannels();
    AddChannel(mInputChannel, CHANNEL_NAME, true);

    return true;
}

void SSDAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mInputChannelInterface->SetChannel(mInputChannel);
    mPreambleBitsInterface->SetInteger(mPreambleBits);
    mModeInterface->SetNumber(mMode);
    mCalPPMInterface->SetInteger(mCalPPM);
    mShowCarDetailsInterface->SetValue(mShowCarDetails);
}

void SSDAnalyzerSettings::LoadSettings(const char *settings)
{
    SimpleArchive text_archive;
    text_archive.SetString(settings);

    const char *name_string;
    text_archive >> &name_string;
    if (strcmp(name_string, "SSDAnalyzer") != 0) {
        AnalyzerHelpers::Assert("SSDAnalyzer: Provided with a settings string that doesn't belong to us;");
    }

    text_archive >> mInputChannel;
    text_archive >> mPreambleBits;
    text_archive >> *(int *)&mMode;
    text_archive >> mCalPPM;
    text_archive >> mShowCarDetails;

    ClearChannels();
    AddChannel(mInputChannel, CHANNEL_NAME, true);

    UpdateInterfacesFromSettings();
}

const char *SSDAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << "SSDAnalyzer";
    text_archive << mInputChannel;
    text_archive << mPreambleBits;
    text_archive << (int)mMode;
    text_archive << mCalPPM;
    text_archive << mShowCarDetails;

    return SetReturnString(text_archive.GetString());
}