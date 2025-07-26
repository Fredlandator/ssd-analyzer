#ifndef SSD_ANALYZER_RESULTS
#define SSD_ANALYZER_RESULTS

#include <AnalyzerResults.h>

#define BIT_ERROR_FLAG ( 1 << 1 )
#define PACKET_ERROR_FLAG ( 1 << 2 )
#define FRAMING_ERROR_FLAG (1 << 3)
#define CHECKSUM_ERROR_FLAG (1 << 4)

enum eFrameType { 
    FRAME_PREAMBLE, 
    FRAME_PSBIT,        // Packet Start Bit
    FRAME_CMDBYTE,      // Command Byte (RACE or PROGRAM)
    FRAME_DSBIT,        // Data Start Bit
    FRAME_CARDATA,      // Car Data Byte
    FRAME_CHECKSUM,     // Checksum Byte
    FRAME_PEBIT,        // Packet End Bit
    FRAME_ERR,          // Error Frame
    FRAME_END_ERR       // End Error Frame
};

class SSDAnalyzer;
class SSDAnalyzerSettings;

class SSDAnalyzerResults : public AnalyzerResults
{
public:
    SSDAnalyzerResults(SSDAnalyzer *analyzer, SSDAnalyzerSettings *settings);
    virtual ~SSDAnalyzerResults();

    virtual void GenerateBubbleText(U64 frame_index, Channel &channel, DisplayBase display_base);
    virtual void GenerateExportFile(const char *file, DisplayBase display_base, U32 export_type_user_id);

    virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base);
    virtual void GeneratePacketTabularText(U64 packet_id, DisplayBase display_base);
    virtual void GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base);

protected:  //vars
    SSDAnalyzerSettings *mSettings;
    SSDAnalyzer *mAnalyzer;
private:
    char sParseBuf[128];
    
    // Helper functions
    const char* GetCommandName(U8 command);
    void DecodeCarData(U8 carData, char* buffer, int bufferSize);
};

#endif //SSD_ANALYZER_RESULTS