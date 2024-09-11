#ifndef POSTMAIN_H
#define POSTMAIN_H
#include "./rtklib.h"
#include "QFileInfo"
#include "QPPPPost.h"



class Processing
{
public:
    prcopt_t prcopt;
    solopt_t solopt;
    filopt_t filopt;
    gtime_t ts,te;
    double ti,tu;
    int n,stat;
    char *infile[6],outfile[1024];
    char *rov,*base;


    QPPPPost m_ppp;

    explicit Processing();
    ~Processing();
    void addInput(const QString &);
    void addList(char * &sta, const QString & list);
    void run();

};

class postMain
{
public:
    postMain();
    void  ExecProc(void);
private:
    int   GetOption(prcopt_t &prcopt, solopt_t &solopt,filopt_t &filopt);
    void  LoadOpt(void);
    int   ObsToNav(const QString &obsfile, QString &navfile);

public:
    QString IniFile;
    int AbortFlag;

    // options
    int PosMode,Freq,Solution,DynamicModel,IonoOpt,TropOpt,RcvBiasEst;
    int ARIter,NumIter,CodeSmooth,TideCorr;
    int OutCntResetAmb,FixCntHoldAmb,LockCntFixAmb,RovPosType,RefPosType;
    int SatEphem,NavSys,BdsEphSource;
    int RovAntPcv,RefAntPcv,AmbRes,GloAmbRes,BdsAmbRes;
    int OutputHead,OutputOpt,OutputDatum;
    int OutputHeight,OutputGeoid,DebugTrace,DebugStatus,BaseLineConst;
    int SolFormat,TimeFormat,LatLonFormat,IntpRefObs,NetRSCorr,SatClkCorr;
    int SbasCorr,SbasCorr1,SbasCorr2,SbasCorr3,SbasCorr4,TimeDecimal;
    int SolStatic,SbasSat,MapFunc;
    int PosOpt[6];
    double ElMask,MaxAgeDiff,RejectThres,RejectGdop;
    double MeasErrR1,MeasErrR2,MeasErr2,MeasErr3,MeasErr4,MeasErr5;
    double SatClkStab,RovAntE,RovAntN,RovAntU,RefAntE,RefAntN,RefAntU;
    double PrNoise1,PrNoise2,PrNoise3,PrNoise4,PrNoise5;
    double ValidThresAR,ElMaskAR,ElMaskHold,SlipThres;
    double ThresAR2,ThresAR3;
    double RovPos[3],RefPos[3],BaseLine[2];
    snrmask_t SnrMask;
    //exterr_t ExtErr;

    QString RnxOpts1,RnxOpts2,PPPOpts;
    QString FieldSep,RovAnt,RefAnt,AntPcvFile,StaPosFile,PrecEphFile;
    QString NetRSCorrFile1,NetRSCorrFile2,SatClkCorrFile,GoogleEarthFile;
    QString GeoidDataFile,IonoFile,DCBFile,EOPFile,BLQFile;
    QString SbasCorrFile,SatPcvFile,ExSats;
    QString RovList,BaseList;


    QString InputFile1,InputFile2,InputFile3,InputFile4,InputFile5,InputFile6;
    QString OutDir,OutputFile;
    bool OutDirEna;



};

#endif // POSTMAIN_H
