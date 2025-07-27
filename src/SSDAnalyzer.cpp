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

UINT SSDAnalyzer::LookaheadNextHBit(U64* nSample)
{
    UINT nHBitLen = (UINT)(mSSD->GetSampleOfNextEdge() - *nSample);
    *nSample = mSSD->GetSampleOfNextEdge();

    if (nHBitLen >= mMin1hbit && nHBitLen <= mMax1hbit)
        return 1;
    else if (nHBitLen >= mMin0hbit && nHBitLen <= mMax0hbit)
        return 0;
    else if (nHBitLen >= mMinPEHold && nHBitLen <= mMaxPGap) {
        return 3; // Packet gap
    }
    else
        return BIT_ERROR_FLAG; // bit error
}

UINT SSDAnalyzer::GetNextHBit(U64* nSample)
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

UINT SSDAnalyzer::GetNextBit(U64* nSample)
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

const char* SSDAnalyzer::GetCurrentPacketColor()
{
    // Retourne la couleur appropriée selon le mode du paquet actuel
    switch (mCurrentMode) {
    case SSD_MODE_RACE:
        return "ssd_race";      // VERT
    case SSD_MODE_PROGRAM:
        return "ssd_program";   // BLEU
    default:
        return "ssd_unknown";   // ORANGE
    }
}

void SSDAnalyzer::PostFrame(U64 nStartSample, U64 nEndSample, eFrameType ft, U8 Flags, U64 Data1, U64 Data2)
{
    Frame frame;
    frame.mStartingSampleInclusive = nStartSample;
    frame.mEndingSampleInclusive = nEndSample;
    frame.mData1 = Data1;
    frame.mData2 = Data2;
    frame.mType = ft;
    frame.mFlags = Flags;
    mResults->AddFrame(frame);

    // FrameV2 for modern Saleae Logic 2 interface with consistent colors per packet
    FrameV2 framev2;

    switch (ft)
    {
    case FRAME_PREAMBLE:
        framev2.AddString("type", "preamble");
        framev2.AddByte("length", (U8)Data1);
        // Couleur neutre pour le préambule
        mResults->AddFrameV2(framev2, "ssd_preamble", nStartSample, nEndSample);
        break;

    case FRAME_PSBIT:
        framev2.AddString("type", "start_bit");
        // Couleur neutre pour les bits de start
        mResults->AddFrameV2(framev2, "ssd_start", nStartSample, nEndSample);
        break;

    case FRAME_CMDBYTE:
        framev2.AddString("type", "command");
        framev2.AddByte("data", (U8)Data1);
        if (Data1 == SSD_MODE_RACE) {
            framev2.AddString("mode", "RACE");
            // VERT pour les commandes RACE
            mResults->AddFrameV2(framev2, "ssd_race", nStartSample, nEndSample);
        }
        else if (Data1 == SSD_MODE_PROGRAM) {
            framev2.AddString("mode", "PROGRAM");
            // BLEU pour les commandes PROGRAM
            mResults->AddFrameV2(framev2, "ssd_program", nStartSample, nEndSample);
        }
        else {
            framev2.AddString("mode", "UNKNOWN");
            // ORANGE pour les commandes inconnues
            mResults->AddFrameV2(framev2, "ssd_unknown", nStartSample, nEndSample);
        }
        break;

    case FRAME_DSBIT:
        framev2.AddString("type", "start_bit");
        // Couleur neutre pour les bits de start
        mResults->AddFrameV2(framev2, "ssd_start", nStartSample, nEndSample);
        break;

    case FRAME_CARDATA:
    {
        framev2.AddString("type", "car_data");
        framev2.AddByte("data", (U8)Data1);
        framev2.AddByte("car_id", (U8)Data2);

        // Decode car data details
        bool braking = (Data1 & 0x80) != 0;
        bool lane_change = (Data1 & 0x40) != 0;
        U8 speed_power = (U8)(Data1 & 0x3F);

        framev2.AddByte("braking", braking ? 1 : 0);
        framev2.AddByte("lane_change", lane_change ? 1 : 0);
        framev2.AddByte("speed_power", speed_power);

        // Utiliser la couleur du paquet actuel (cohérence)
        mResults->AddFrameV2(framev2, GetCurrentPacketColor(), nStartSample, nEndSample);
    }
    break;

    case FRAME_CHECKSUM:
        framev2.AddString("type", "checksum");
        framev2.AddByte("data", (U8)Data1);
        framev2.AddByte("valid", (Flags & CHECKSUM_ERROR_FLAG) == 0 ? 1 : 0);

        if ((Flags & CHECKSUM_ERROR_FLAG) != 0) {
            // ROUGE seulement pour les erreurs de checksum
            mResults->AddFrameV2(framev2, "ssd_error", nStartSample, nEndSample);
        }
        else {
            // Utiliser la couleur du paquet actuel si checksum OK
            mResults->AddFrameV2(framev2, GetCurrentPacketColor(), nStartSample, nEndSample);
        }
        break;

    case FRAME_PEBIT:
        framev2.AddString("type", "end_bit");
        // Couleur neutre pour la fin de paquet
        mResults->AddFrameV2(framev2, "ssd_end", nStartSample, nEndSample);
        break;

    default:
        framev2.AddString("type", "error");
        // ROUGE seulement pour les erreurs
        mResults->AddFrameV2(framev2, "ssd_error", nStartSample, nEndSample);
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
    double dMaxCorrection = 1.0 + (double)mSettings->mCalPPM / 1000000.0;
    double dMinCorrection = 1.0 - (double)mSettings->mCalPPM / 1000000.0;

    // SSD Protocol timing - TOLERANCES ELARGIES
    // Bit 1: 57μs à 63μs par demi-bit (période complète: 114μs à 126μs)
    // Bit 0: 106μs à 125μs par demi-bit (période complète: 212μs à 250μs)

    mMaxBitLen = (UINT)round(500.0 * dSamplesPerMicrosecond * dMaxCorrection);  // Maximum bit length
    mMinPEHold = (UINT)round(26.0 * dSamplesPerMicrosecond * dMinCorrection);
    mMaxPGap = (UINT)round(30000.0 * dSamplesPerMicrosecond * dMaxCorrection);

    if (mSettings->mMode == SSDAnalyzerEnums::MODE_TOLERANT) {
        // Mode tolérant : plages encore plus larges
        mMin1hbit = (UINT)round(55.0 * dSamplesPerMicrosecond * dMinCorrection);  // 57μs - 2μs marge
        mMax1hbit = (UINT)round(65.0 * dSamplesPerMicrosecond * dMaxCorrection);  // 63μs + 2μs marge
        mMin0hbit = (UINT)round(104.0 * dSamplesPerMicrosecond * dMinCorrection); // 106μs - 2μs marge
        mMax0hbit = (UINT)round(127.0 * dSamplesPerMicrosecond * dMaxCorrection); // 125μs + 2μs marge
    }
    else {
        // Mode standard : tolérances demandées
        mMin1hbit = (UINT)round(57.0 * dSamplesPerMicrosecond * dMinCorrection);  // 57μs minimum
        mMax1hbit = (UINT)round(63.0 * dSamplesPerMicrosecond * dMaxCorrection);  // 63μs maximum
        mMin0hbit = (UINT)round(106.0 * dSamplesPerMicrosecond * dMinCorrection); // 106μs minimum
        mMax0hbit = (UINT)round(125.0 * dSamplesPerMicrosecond * dMaxCorrection); // 125μs maximum
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
    }
    else {
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
                if (nHBitCnt == ((U32)mSettings->mPreambleBits * 2)) {
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
            switch (nHBitVal) {
            case 0: // Start bit ends preamble
                PostFrame(nPreambleStart, nCurSample, FRAME_PREAMBLE, 0, nHBitCnt / 2, 0);
                ReportProgress(nCurSample);
                nFrameStart = nCurSample + 1;
                ef = FSTATE_PSBIT;
                break;
            case 1:
                nHBitVal = GetNextHBit(&nCurSample);
                ++nHBitCnt;
                break;
            default:
                PostFrame(nBitStartSample, nCurSample, FRAME_ERR, nHBitVal, 0, 0);
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
                PostFrame(nFrameStart, nCurSample, FRAME_PSBIT, 0, 0, 0);
                mResults->AddMarker(nFrameStart, AnalyzerResults::Start, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nBits = nVal = 0;
                mCalculatedChecksum = 0;  // Reset checksum - NE PAS inclure la commande
                nFrameStart = nCurSample + 1;
                ef = FSTATE_CMDBYTE;
            }
            else {
                PostFrame(nBitStartSample, nCurSample, FRAME_ERR, FRAMING_ERROR_FLAG, 0, 0);
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
                    mCurrentMode = nVal;  // Définir le mode pour tout le paquet
                    mCarCount = 0;  // Reset car count for new packet

                    // CORRECTION IMPORTANTE : NE PAS inclure la commande dans le checksum
                    // Le checksum SSD = XOR des données voitures SEULEMENT
                    // mCalculatedChecksum reste à 0 (déjà initialisé)

                    PostFrame(nFrameStart, nCurSample, FRAME_CMDBYTE, 0, nVal, 0);
                    ReportProgress(nCurSample);
                    nFrameStart = nCurSample + 1;
                    ef = FSTATE_DSBIT;
                    nBits = nVal = 0;
                }
                break;
            default:
                PostFrame(nBitStartSample, nCurSample, FRAME_ERR, nHBitVal, 0, 0);
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
                PostFrame(nFrameStart, nCurSample, FRAME_DSBIT, 0, 0, 0);
                mResults->AddMarker(nFrameStart, AnalyzerResults::Start, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nBits = nVal = 0;
                nFrameStart = nCurSample + 1;
                ef = FSTATE_DATABYTE;
            }
            else {
                PostFrame(nBitStartSample, nCurSample, FRAME_ERR, FRAMING_ERROR_FLAG, 0, 0);
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
                    // Store the car data byte
                    if (mCarCount < 6) {  // Protection contre débordement
                        mCarData[mCarCount] = nVal;
                    }

                    // CORRECTION : Ajouter SEULEMENT les données voitures au checksum
                    mCalculatedChecksum ^= nVal;

                    // Afficher les données avec le numéro de voiture correct (1-6)
                    U8 carNumber = mCarCount + 1;
                    PostFrame(nFrameStart, nCurSample, FRAME_CARDATA, 0, nVal, carNumber);

                    // Determine if this is the last data byte or if more follow
                    bool isLastByte = false;

                    if (mCurrentMode == SSD_MODE_RACE) {
                        // En mode RACE, on attend exactement 6 voitures (mCarCount 0-5)
                        isLastByte = (mCarCount >= 5);
                    }
                    else if (mCurrentMode == SSD_MODE_PROGRAM) {
                        // En mode PROGRAM, on attend exactement 4 bytes (mCarCount 0-3)
                        isLastByte = (mCarCount >= 3);
                    }
                    else {
                        // Mode inconnu - supposer que c'est le checksum après ce byte
                        isLastByte = true;
                    }

                    // Incrémenter le compteur de voitures APRÈS l'avoir utilisé
                    mCarCount++;

                    // CORRECTION PRINCIPALE: Toujours passer par un état DSBIT
                    // car il y a TOUJOURS un bit start avant le prochain byte (données ou checksum)
                    ReportProgress(nCurSample);
                    nFrameStart = nCurSample + 1;
                    nBits = nVal = 0;

                    if (isLastByte) {
                        // Prochain byte sera le checksum, mais il faut d'abord lire son bit start
                        ef = FSTATE_DSBIT_CHECKSUM;  // Nouvel état pour différencier
                    }
                    else {
                        // Plus de bytes de données à suivre
                        ef = FSTATE_DSBIT;
                    }
                }
                break;
            default:
                PostFrame(nBitStartSample, nCurSample, FRAME_ERR, nHBitVal, 0, 0);
                mResults->AddMarker(nBitStartSample, AnalyzerResults::ErrorSquare, mSettings->mInputChannel);
                mResults->AddMarker(nCurSample, AnalyzerResults::ErrorX, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nHBitCnt = 0;
                ef = FSTATE_INIT;
            }
            break;

        case FSTATE_DSBIT_CHECKSUM:
            nHBitVal = GetNextBit(&nCurSample);
            if (nHBitVal == 0) { // Data start bit avant checksum
                PostFrame(nFrameStart, nCurSample, FRAME_DSBIT, 0, 0, 0);
                mResults->AddMarker(nFrameStart, AnalyzerResults::Start, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nBits = nVal = 0;
                nFrameStart = nCurSample + 1;
                ef = FSTATE_CHECKSUM;  // Maintenant on peut lire le checksum
            }
            else {
                PostFrame(nBitStartSample, nCurSample, FRAME_ERR, FRAMING_ERROR_FLAG, 0, 0);
                mResults->AddMarker(nBitStartSample, AnalyzerResults::ErrorDot, mSettings->mInputChannel);
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

                    // Comparer le checksum reçu avec le checksum calculé
                    // Le checksum calculé inclut SEULEMENT les données voitures (pas la commande)
                    if (nVal != mCalculatedChecksum) {
                        flags |= CHECKSUM_ERROR_FLAG;
                        mResults->AddMarker(nFrameStart, AnalyzerResults::ErrorX, mSettings->mInputChannel);
                    }

                    PostFrame(nFrameStart, nCurSample, FRAME_CHECKSUM, flags, nVal, mCalculatedChecksum);
                    ReportProgress(nCurSample);
                    nFrameStart = nCurSample + 1;
                    ef = FSTATE_PEBIT;
                    nBits = nVal = 0;
                }
                break;
            default:
                PostFrame(nBitStartSample, nCurSample, FRAME_ERR, nHBitVal, 0, 0);
                mResults->AddMarker(nBitStartSample, AnalyzerResults::ErrorSquare, mSettings->mInputChannel);
                mResults->AddMarker(nCurSample, AnalyzerResults::ErrorX, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nHBitCnt = 0;
                ef = FSTATE_INIT;
            }
            break;

        case FSTATE_PEBIT:
        {
            // Bloc pour isoler la portée de la variable lookahead
            // Après le checksum, chercher le gap entre paquets ou le début du prochain
            nTemp = nCurSample;
            UINT lookahead = LookaheadNextHBit(&nTemp);

            if (lookahead == 3) {
                // Packet gap détecté - fin normale de paquet
                PostFrame(nFrameStart, nCurSample, FRAME_PEBIT, 0, 0, 0);
                mResults->AddMarker(nFrameStart, AnalyzerResults::Stop, mSettings->mInputChannel);
                ReportProgress(nCurSample);

                // Avancer jusqu'à la fin du gap
                while (LookaheadNextHBit(&nCurSample) == 3) {
                    GetNextHBit(&nCurSample);
                }

                nFrameStart = nCurSample + 1;
                nPreambleStart = nFrameStart;
                nHBitCnt = 0;
                ef = FSTATE_INIT;
            }
            else if (lookahead == 1) {
                // Début immédiat du prochain paquet (preamble)
                PostFrame(nFrameStart, nCurSample, FRAME_PEBIT, 0, 0, 0);
                mResults->AddMarker(nFrameStart, AnalyzerResults::Stop, mSettings->mInputChannel);
                ReportProgress(nCurSample);

                nFrameStart = nCurSample + 1;
                nPreambleStart = nFrameStart;
                nHBitCnt = 1; // On a déjà vu le premier bit '1' du préambule
                ef = FSTATE_INIT;
            }
            else {
                // Erreur ou bit inattendu
                PostFrame(nFrameStart, nCurSample, FRAME_ERR, BIT_ERROR_FLAG, 0, 0);
                mResults->AddMarker(nFrameStart, AnalyzerResults::ErrorX, mSettings->mInputChannel);
                ReportProgress(nCurSample);
                nHBitCnt = 0;
                ef = FSTATE_INIT;
            }
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

U32 SSDAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels)
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

const char* SSDAnalyzer::GetAnalyzerName() const
{
    return "SSD";
}

const char* GetAnalyzerName()
{
    return "SSD";
}

Analyzer* CreateAnalyzer()
{
    return new SSDAnalyzer();
}

void DestroyAnalyzer(Analyzer* analyzer)
{
    delete analyzer;
}