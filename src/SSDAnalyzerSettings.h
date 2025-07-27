#ifndef SSD_ANALYZER_SETTINGS
#define SSD_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

namespace SSDAnalyzerEnums
{
    enum eAnalyzerMode { MODE_STANDARD, MODE_TOLERANT };
    enum eSignalPolarity { POLARITY_NORMAL, POLARITY_INVERTED };
    enum FrameType { TYPE_Preamble, TYPE_Command, TYPE_CarData, TYPE_Checksum };
};

class SSDAnalyzerSettings : public AnalyzerSettings
{
public:
    SSDAnalyzerSettings();
    virtual ~SSDAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    void UpdateInterfacesFromSettings();
    virtual void LoadSettings(const char* settings);
    virtual const char* SaveSettings();

    Channel mInputChannel;
    U64     mPreambleBits;
    SSDAnalyzerEnums::eAnalyzerMode mMode;
    SSDAnalyzerEnums::eSignalPolarity mPolarity;
    int     mCalPPM;
    bool    mShowCarDetails;

protected:
    std::unique_ptr< AnalyzerSettingInterfaceChannel >    mInputChannelInterface;
    std::unique_ptr< AnalyzerSettingInterfaceInteger >    mPreambleBitsInterface;
    std::unique_ptr< AnalyzerSettingInterfaceNumberList > mModeInterface;
    std::unique_ptr< AnalyzerSettingInterfaceNumberList > mPolarityInterface;
    std::unique_ptr< AnalyzerSettingInterfaceInteger >    mCalPPMInterface;
    std::unique_ptr< AnalyzerSettingInterfaceBool >       mShowCarDetailsInterface;
};

#endif //SSD_ANALYZER_SETTINGS