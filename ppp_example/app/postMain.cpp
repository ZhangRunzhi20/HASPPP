#include "postMain.h"
#include "QSettings"
#include <iostream>
#ifdef WIN32
#include <QTextCodec>
#endif
using namespace std;

#ifdef WIN32
#define FILEPATHSEP '\\'
#else
#define FILEPATHSEP '/'
#endif

#ifdef WIN32
#define FILEPATHSEPSTR "\\"
#else
#define FILEPATHSEPSTR "/"
#endif

#ifdef WIN32
#define CONFIGNAME "rtkpost.ini"
#else
#define CONFIGNAME "rtkpost_qt.ini"
#endif

extern char debugwork[1024];

Processing::Processing()
{
    n=stat=0;
    prcopt=prcopt_default;
    solopt=solopt_default;
    ts.time=ts.sec=0;
    te.time=te.sec=0;
    ti=tu=0;
    rov=base=NULL;
    for (int i=0;i<6;i++) {infile[i]=new char[1024];infile[i][0]='\0';};
    outfile[0]='\0';

    memset(&prcopt,0,sizeof(prcopt_t));
    memset(&solopt,0,sizeof(solopt_t));
    memset(&filopt,0,sizeof(filopt_t));
}
Processing::~Processing()
{
    for (int i=0;i<6;i++) delete infile[i];
    if (rov) delete rov;
    if (base) delete base;
    rov=base=NULL;
}
void Processing::addInput(const QString & file) {
    if (file!="") {
        strcpy(infile[n++],qPrintable(file));
    }
}
void Processing::addList(char * &sta, const QString & list) {
    char *r;
    sta =new char [list.length()+1 +1];

    QStringList lines=list.split("\n");

    r=sta;
    foreach(QString line,lines)
    {
        int index=line.indexOf("#");
        if (index==-1) index=line.length();
        strcpy(r,qPrintable(line.mid(0,index)));
        r+=index;
        strcpy(r++," ");
    }
}
void Processing::run()
{
    //tracelevel(999);
    //traceopen("/home/david/Documents/rtklib_b34_my/rtkppp_postQTMy/BRUXPPP/log.log");
    if ((stat=m_ppp.postpos(ts,te,ti,tu,&prcopt,&solopt,&filopt,infile,n,outfile,
                      rov,base))==1) {
  //      showmsg("aborted");
    };
    //if ((stat=postpos(ts,te,ti,tu,&prcopt,&solopt,&filopt,infile,n,outfile,
       //               rov,base))==1) {
  //      showmsg("aborted");
    //};
    //emit done(stat);
    //int i = 1;
    //i++;
    //traceclose();
}


postMain::postMain()
{
    int i;

    /*QString file= "/home/david/Documents/rtklib_b34_my/rtkppp_post/";
    QFileInfo fi(file);
    IniFile=fi.absolutePath()+"/"+fi.baseName()+".ini";*/
//    IniFile="/media/david/Passport/HASPPP/ppp_example/Linux/HAS_PPP_OS/rtkpost_qt.ini";
    std::cout<< debugwork<<std::endl;
    std::string strCurPath = std::string(debugwork);
    std::string m_workContent;

    m_workContent = strCurPath.substr( 0, strCurPath.find_last_of(FILEPATHSEP));
    m_workContent = m_workContent.substr( 0, m_workContent.find_last_of(FILEPATHSEP));
    m_workContent = m_workContent + std::string(FILEPATHSEPSTR)+std::string("config")+std::string(FILEPATHSEPSTR)+std::string(CONFIGNAME);

    std::cout<< m_workContent<<std::endl;
    IniFile = m_workContent.c_str();



    DynamicModel=IonoOpt=TropOpt=RovAntPcv=RefAntPcv=AmbRes=0;
    RovPosType=RefPosType=0;
    OutCntResetAmb=5; LockCntFixAmb=5; FixCntHoldAmb=10;
    MaxAgeDiff=30.0; RejectThres=30.0; RejectGdop=30.0;
    MeasErrR1=MeasErrR2=100.0; MeasErr2=0.004; MeasErr3=0.003; MeasErr4=1.0;
    SatClkStab=1E-11; ValidThresAR=3.0;
    RovAntE=RovAntN=RovAntU=RefAntE=RefAntN=RefAntU=0.0;

    for (i=0;i<3;i++) RovPos[i]=0.0;
    for (i=0;i<3;i++) RefPos[i]=0.0;

}

void postMain::ExecProc(void)
{

    /*QString InputFile1_Text=InputFile1->currentText(),InputFile2_Text=InputFile2->currentText();
    QString InputFile3_Text=InputFile3->currentText(),InputFile4_Text=InputFile4->currentText();
    QString InputFile5_Text=InputFile5->currentText(),InputFile6_Text=InputFile6->currentText();
    QString OutputFile_Text=OutputFile->currentText();*/

    LoadOpt();
    QString InputFile1_Text=InputFile1,InputFile2_Text=InputFile2;
    QString InputFile3_Text=InputFile3,InputFile4_Text=InputFile4;
    QString InputFile5_Text=InputFile5,InputFile6_Text=InputFile6;
    QString OutputFile_Text=OutputFile;
    QString temp;

    Processing *thread= new Processing();
    // get processing options
    gtime_t t0={0};
    thread->ts=t0;
    thread->te=t0;
    thread->ti=0.0;
    thread->tu=0.0*3600.0;

    thread->prcopt=prcopt_default;
    if (!GetOption(thread->prcopt,thread->solopt,thread->filopt)) {return;}

    // set input/output files

    thread->addInput(InputFile1_Text);

    if (PMODE_DGPS<=thread->prcopt.mode&&thread->prcopt.mode<=PMODE_FIXED) {
        thread->addInput(InputFile2_Text);
    }
    if (InputFile3_Text!="") {
        thread->addInput(InputFile3_Text);
    }
    else if (!ObsToNav(InputFile1_Text,temp)) {
        //showmsg("error: no navigation data");
        //ProcessingFinished(0);
        return;
    } else thread->addInput(temp);

    if (InputFile4_Text!="") {
        thread->addInput(InputFile4_Text);
    }
    if (InputFile5_Text!="") {
        thread->addInput(InputFile5_Text);
    }
    if (InputFile6_Text!="") {
        thread->addInput(InputFile6_Text);
    }
    strcpy(thread->outfile,qPrintable(OutputFile_Text));

    // confirm overwrite
    /*if (!TimeStart->isChecked()||!TimeEnd->isChecked()) {
        if (QFileInfo::exists(thread->outfile)) {
            if (QMessageBox::question(this,tr("Overwrite"),QString(tr("Overwrite existing file %1.")).arg(thread->outfile))!=QMessageBox::Yes) {ProcessingFinished(0);return;}
        }
    }  */

    // set rover and base station list
    thread->addList(thread->rov,RovList);
    thread->addList(thread->base,BaseList);

    /*Progress->setValue(0);
    Progress->setVisible(true);
    showmsg("reading...");

    setCursor(Qt::WaitCursor);

    // post processing positioning
    connect(thread,SIGNAL(done(int)),this,SLOT(ProcessingFinished(int)));

    thread->start();*/

    thread->run();


    delete thread;
    return;
}
int postMain::ObsToNav(const QString &obsfile, QString &navfile)
{
    int p;
    QFileInfo f(obsfile);
    navfile=f.canonicalPath()+f.completeBaseName();
    QString suffix=f.suffix();

    if (suffix=="") return 0;
    if ((suffix.length()==3)&&(suffix.at(2).toLower()=='o')) suffix[2]='*';
    else if ((suffix.length()==3)&&(suffix.at(2).toLower()=='d')) suffix[2]='*';
    else if (suffix.toLower()=="obs") suffix="*nav";
    else if (((p=suffix.indexOf("gz"))!=-1)||((p=suffix.indexOf('Z'))!=-1)) {
        if (p<1) return 0;
        if (suffix.at(p-1).toLower()=='o') suffix[p-1]='*';
        else if (suffix.at(p-1).toLower()=='d') suffix[p-1]='*';
        else return 0;
    }
    else return 0;
    return 1;
}
// get processing and solution options --------------------------------------
int postMain::GetOption(prcopt_t &prcopt, solopt_t &solopt,
                                    filopt_t &filopt)
{
    char buff[1024],*p;
    int sat,ex;

    // processing options
    prcopt.bdsephsource = BdsEphSource;
    prcopt.mode     =PosMode;
    prcopt.soltype  =Solution;
    prcopt.nf       =Freq+1;
    prcopt.navsys   =NavSys;
    prcopt.elmin    =ElMask*D2R;
    prcopt.snrmask  =SnrMask;
    prcopt.sateph   =SatEphem;
    prcopt.modear   =AmbRes;
    prcopt.glomodear=GloAmbRes;
    prcopt.bdsmodear=BdsAmbRes;
    prcopt.maxout   =OutCntResetAmb;
    prcopt.minfix   =FixCntHoldAmb;
    prcopt.minlock  =LockCntFixAmb;
    prcopt.ionoopt  =IonoOpt;
    prcopt.tropopt  =TropOpt;
    prcopt.posopt[0]=PosOpt[0];
    prcopt.posopt[1]=PosOpt[1];
    prcopt.posopt[2]=PosOpt[2];
    prcopt.posopt[3]=PosOpt[3];
    prcopt.posopt[4]=PosOpt[4];
    prcopt.posopt[5]=PosOpt[5];
    prcopt.dynamics =DynamicModel;
    prcopt.tidecorr =TideCorr;
    prcopt.armaxiter=ARIter;
    prcopt.niter    =NumIter;
    prcopt.intpref  =IntpRefObs;
    prcopt.sbassatsel=SbasSat;
    prcopt.eratio[0]=MeasErrR1;
    prcopt.eratio[1]=MeasErrR2;
    prcopt.err[1]   =MeasErr2;
    prcopt.err[2]   =MeasErr3;
    prcopt.err[3]   =MeasErr4;
    prcopt.err[4]   =MeasErr5;
    prcopt.prn[0]   =PrNoise1;
    prcopt.prn[1]   =PrNoise2;
    prcopt.prn[2]   =PrNoise3;
    prcopt.prn[3]   =PrNoise4;
    prcopt.prn[4]   =PrNoise5;
    prcopt.sclkstab =SatClkStab;
    prcopt.thresar[0]=ValidThresAR;
    prcopt.thresar[1]=ThresAR2;
    prcopt.thresar[2]=ThresAR3;
    prcopt.elmaskar =ElMaskAR*D2R;
    prcopt.elmaskhold=ElMaskHold*D2R;
    prcopt.thresslip=SlipThres;
    prcopt.maxtdiff =MaxAgeDiff;
    prcopt.maxgdop  =RejectGdop;
    prcopt.maxinno  =RejectThres;
    if (BaseLineConst) {
        prcopt.baseline[0]=BaseLine[0];
        prcopt.baseline[1]=BaseLine[1];
    }
    else {
        prcopt.baseline[0]=0.0;
        prcopt.baseline[1]=0.0;
    }
    if (PosMode!=PMODE_FIXED&&PosMode!=PMODE_PPP_FIXED) {
        for (int i=0;i<3;i++) prcopt.ru[i]=0.0;
    }
    else if (RovPosType<=2) {
        for (int i=0;i<3;i++) prcopt.ru[i]=RovPos[i];
    }
    else prcopt.rovpos=RovPosType-2; /* 1:single,2:posfile,3:rinex */

    if (PosMode==PMODE_SINGLE||PosMode==PMODE_MOVEB) {
        for (int i=0;i<3;i++) prcopt.rb[i]=0.0;
    }
    else if (RefPosType<=2) {
        for (int i=0;i<3;i++) prcopt.rb[i]=RefPos[i];
    }
    else prcopt.refpos=RefPosType-2;

    if (RovAntPcv) {
        strcpy(prcopt.anttype[0],qPrintable(RovAnt));
        prcopt.antdel[0][0]=RovAntE;
        prcopt.antdel[0][1]=RovAntN;
        prcopt.antdel[0][2]=RovAntU;
    }
    if (RefAntPcv) {
        strcpy(prcopt.anttype[1],qPrintable(RefAnt));
        prcopt.antdel[1][0]=RefAntE;
        prcopt.antdel[1][1]=RefAntN;
        prcopt.antdel[1][2]=RefAntU;
    }
    if (ExSats!="") { // excluded satellites
        strcpy(buff,qPrintable(ExSats));
        for (p=strtok(buff," ");p;p=strtok(NULL," ")) {
            if (*p=='+') {ex=2; p++;} else ex=1;
            if (!(sat=satid2no(p))) continue;
            prcopt.exsats[sat-1]=ex;
        }
    }
    // extended receiver error model option
    //prcopt.exterr=ExtErr;

    strcpy(prcopt.rnxopt[0],qPrintable(RnxOpts1));
    strcpy(prcopt.rnxopt[1],qPrintable(RnxOpts2));
    strcpy(prcopt.pppopt,qPrintable(PPPOpts));

    // solution options
    solopt.posf     =SolFormat;
    solopt.times    =TimeFormat==0?0:TimeFormat-1;
    solopt.timef    =TimeFormat==0?0:1;
    solopt.timeu    =TimeDecimal<=0?0:TimeDecimal;
    solopt.degf     =LatLonFormat;
    solopt.outhead  =OutputHead;
    solopt.outopt   =OutputOpt;
    solopt.datum    =OutputDatum;
    solopt.height   =OutputHeight;
    solopt.geoid    =OutputGeoid;
    solopt.solstatic=SolStatic;
    solopt.sstat    =DebugStatus;
    solopt.trace    =DebugTrace;
    strcpy(solopt.sep,FieldSep!=""?qPrintable(FieldSep):" ");
    //strcpy(solopt.prog,qPrintable(QString("%1 ver.%2 %3").arg(PRGNAME).arg(VER_RTKLIB).arg(PATCH_LEVEL)));

    // file options
    strcpy(filopt.satantp,qPrintable(SatPcvFile));
    strcpy(filopt.rcvantp,qPrintable(AntPcvFile));
    strcpy(filopt.stapos, qPrintable(StaPosFile));
    strcpy(filopt.geoid,  qPrintable(GeoidDataFile));
    strcpy(filopt.iono,   qPrintable(IonoFile));
    strcpy(filopt.eop,    qPrintable(EOPFile));
    strcpy(filopt.dcb,    qPrintable(DCBFile));
    strcpy(filopt.blq,    qPrintable(BLQFile));

    return 1;
}
#ifdef WIN32
void postMain::LoadOpt(void)
{
    char temppath[1024];
    QSettings ini(IniFile,QSettings::IniFormat);
    ini.setIniCodec(QTextCodec::codecForName("UTF-8"));

    /*TimeStart->setChecked(ini.value("set/timestart",   0).toBool());
    TimeEnd->setChecked(ini.value("set/timeend",     0).toBool());
    TimeY1->setDate(ini.value ("set/timey1",      "2000/01/01").toDate());
    TimeH1->setTime(ini.value ("set/timeh1",      "00:00:00").toTime());
    TimeY2->setDate(ini.value ("set/timey2",      "2000/01/01").toDate());
    TimeH2->setTime(ini.value ("set/timeh2",      "00:00:00").toTime());
    TimeIntF ->setChecked(ini.value("set/timeintf",    0).toBool());
    TimeInt->setCurrentText(ini.value ("set/timeint",     "0").toString());
    TimeUnitF->setChecked(ini.value("set/timeunitf",   0).toBool());
    TimeUnit->setText(ini.value ("set/timeunit",    "24").toString());
    InputFile1->setCurrentText(ini.value ("set/inputfile1",  "").toString());
    InputFile2->setCurrentText(ini.value ("set/inputfile2",  "").toString());
    InputFile3->setCurrentText(ini.value ("set/inputfile3",  "").toString());
    InputFile4->setCurrentText(ini.value ("set/inputfile4",  "").toString());
    InputFile5->setCurrentText(ini.value ("set/inputfile5",  "").toString());
    InputFile6->setCurrentText(ini.value ("set/inputfile6",  "").toString());
    OutDirEna->setChecked(ini.value("set/outputdirena", 0).toBool());
    OutDir->setText(ini.value ("set/outputdir",   "").toString());
    OutputFile->setCurrentText(ini.value ("set/outputfile",  "").toString());

    ReadList(InputFile1,&ini,"hist/inputfile1");
    ReadList(InputFile2,&ini,"hist/inputfile2");
    ReadList(InputFile3,&ini,"hist/inputfile3");
    ReadList(InputFile4,&ini,"hist/inputfile4");
    ReadList(InputFile5,&ini,"hist/inputfile5");
    ReadList(InputFile6,&ini,"hist/inputfile6");
    ReadList(OutputFile,&ini,"hist/outputfile");*/

    InputFile1         =ini.value ("set/inputfile1",  "").toString();
    InputFile2         =ini.value ("set/inputfile2",  "").toString();
    InputFile3         =ini.value ("set/inputfile3",  "").toString();
    InputFile4         =ini.value ("set/inputfile4",  "").toString();
    InputFile5         =ini.value ("set/inputfile5",  "").toString();
    InputFile6         =ini.value ("set/inputfile6",  "").toString();
    /*change by ZRZ 2024 2 23 */
    QByteArray byteArray = IniFile.toUtf8();

    const char* charArrayInicnt = byteArray.constData();
    char charArrayIni[1024];
    strcpy(charArrayIni,charArrayInicnt);
    strcpy(temppath,GetIniKeyString("set","inputfile1","",charArrayIni));
    InputFile1=QString(temppath);
    strcpy(temppath,GetIniKeyString("set","inputfile2","",charArrayIni));
    InputFile2=QString(temppath);
    strcpy(temppath,GetIniKeyString("set","inputfile3","",charArrayIni));
    InputFile3=QString(temppath);
    strcpy(temppath,GetIniKeyString("set","inputfile4","",charArrayIni));
    InputFile4=QString(temppath);
    strcpy(temppath,GetIniKeyString("set","inputfile5","",charArrayIni));
    InputFile5=QString(temppath);
    strcpy(temppath,GetIniKeyString("set","inputfile6","",charArrayIni));
    InputFile6=QString(temppath);

    //    InputFile1         =ini.value ("set/inputfile1",  "").toString();
    //    InputFile2         =ini.value ("set/inputfile2",  "").toString();
    //    InputFile3         =ini.value ("set/inputfile3",  "").toString();
    //    InputFile4         =ini.value ("set/inputfile4",  "").toString();
    //    InputFile5         =ini.value ("set/inputfile5",  "").toString();
    //    InputFile6         =ini.value ("set/inputfile6",  "").toString();


    OutDirEna          =ini.value("set/outputdirena", 0).toBool();
    OutDir             =ini.value ("set/outputdir",   "").toString();
    OutputFile         =ini.value ("set/outputfile",  "").toString();
    strcpy(temppath,GetIniKeyString("set","outputfile","",charArrayIni));
    OutputFile=QString(temppath);



    PosMode            =ini.value("opt/posmode",        0).toInt();
    Freq               =ini.value("opt/freq",           1).toInt();
    Solution           =ini.value("opt/solution",       0).toInt();
    ElMask             =ini.value  ("opt/elmask",      15.0).toDouble();
    SnrMask.ena[0]     =ini.value("opt/snrmask_ena1",   0).toInt();
    SnrMask.ena[1]     =ini.value("opt/snrmask_ena2",   0).toInt();
    for (int i=0;i<3;i++) for (int j=0;j<9;j++) {
            SnrMask.mask[i][j]=
                ini.value(QString("opt/snrmask_%1_%2").arg(i+1).arg(j+1),0.0).toDouble();
        }
    IonoOpt            =ini.value("opt/ionoopt",     IONOOPT_BRDC).toInt();
    TropOpt            =ini.value("opt/tropopt",     TROPOPT_SAAS).toInt();
    RcvBiasEst         =ini.value("opt/rcvbiasest",     0).toInt();
    DynamicModel       =ini.value("opt/dynamicmodel",   0).toInt();
    TideCorr           =ini.value("opt/tidecorr",       0).toInt();
    SatEphem           =ini.value("opt/satephem",       0).toInt();
    BdsEphSource       =ini.value("opt/ephsource",     0).toInt();
    //BdsEphSource=4;
    ExSats             =ini.value ("opt/exsats",        "").toString();
    NavSys             =ini.value("opt/navsys",   SYS_GPS).toInt();
    PosOpt[0]          =ini.value("opt/posopt1",        0).toInt();
    PosOpt[1]          =ini.value("opt/posopt2",        0).toInt();
    PosOpt[2]          =ini.value("opt/posopt3",        0).toInt();
    PosOpt[3]          =ini.value("opt/posopt4",        0).toInt();
    PosOpt[4]          =ini.value("opt/posopt5",        0).toInt();
    PosOpt[5]          =ini.value("opt/posopt6",        0).toInt();
    MapFunc            =ini.value("opt/mapfunc",        0).toInt();

    AmbRes             =ini.value("opt/ambres",         1).toInt();
    GloAmbRes          =ini.value("opt/gloambres",      1).toInt();
    BdsAmbRes          =ini.value("opt/bdsambres",      1).toInt();
    ValidThresAR       =ini.value  ("opt/validthresar", 3.0).toDouble();
    ThresAR2           =ini.value  ("opt/thresar2",  0.9999).toDouble();
    ThresAR3           =ini.value  ("opt/thresar3",    0.25).toDouble();
    LockCntFixAmb      =ini.value("opt/lockcntfixamb",  0).toInt();
    FixCntHoldAmb      =ini.value("opt/fixcntholdamb", 10).toInt();
    ElMaskAR           =ini.value  ("opt/elmaskar",     0.0).toDouble();
    ElMaskHold         =ini.value  ("opt/elmaskhold",   0.0).toDouble();
    OutCntResetAmb     =ini.value("opt/outcntresetbias",5).toInt();
    SlipThres          =ini.value  ("opt/slipthres",   0.05).toDouble();
    MaxAgeDiff         =ini.value  ("opt/maxagediff",  30.0).toDouble();
    RejectThres        =ini.value  ("opt/rejectthres", 30.0).toDouble();
    RejectGdop         =ini.value  ("opt/rejectgdop",  30.0).toDouble();
    ARIter             =ini.value("opt/ariter",         1).toInt();
    NumIter            =ini.value("opt/numiter",        1).toInt();
    CodeSmooth         =ini.value("opt/codesmooth",     0).toInt();
    BaseLine[0]        =ini.value  ("opt/baselinelen",  0.0).toDouble();
    BaseLine[1]        =ini.value  ("opt/baselinesig",  0.0).toDouble();
    BaseLineConst      =ini.value("opt/baselineconst",  0).toInt();

    SolFormat          =ini.value("opt/solformat",      0).toInt();
    TimeFormat         =ini.value("opt/timeformat",     1).toInt();
    TimeDecimal        =ini.value("opt/timedecimal",    3).toInt();
    LatLonFormat       =ini.value("opt/latlonformat",   0).toInt();
    FieldSep           =ini.value ("opt/fieldsep",      "").toString();
    OutputHead         =ini.value("opt/outputhead",     1).toInt();
    OutputOpt          =ini.value("opt/outputopt",      1).toInt();
    OutputDatum        =ini.value("opt/outputdatum",    0).toInt();
    OutputHeight       =ini.value("opt/outputheight",   0).toInt();
    OutputGeoid        =ini.value("opt/outputgeoid",    0).toInt();
    SolStatic          =ini.value("opt/solstatic",      0).toInt();
    DebugTrace         =ini.value("opt/debugtrace",     0).toInt();
    DebugStatus        =ini.value("opt/debugstatus",    0).toInt();

    MeasErrR1          =ini.value  ("opt/measeratio1",100.0).toDouble();
    MeasErrR2          =ini.value  ("opt/measeratio2",100.0).toDouble();
    MeasErr2           =ini.value  ("opt/measerr2",   0.003).toDouble();
    MeasErr3           =ini.value  ("opt/measerr3",   0.003).toDouble();
    MeasErr4           =ini.value  ("opt/measerr4",   0.000).toDouble();
    MeasErr5           =ini.value  ("opt/measerr5",  10.000).toDouble();
    SatClkStab         =ini.value  ("opt/satclkstab", 5E-12).toDouble();
    PrNoise1           =ini.value  ("opt/prnoise1",    1E-4).toDouble();
    PrNoise2           =ini.value  ("opt/prnoise2",    1E-3).toDouble();
    PrNoise3           =ini.value  ("opt/prnoise3",    1E-4).toDouble();
    PrNoise4           =ini.value  ("opt/prnoise4",    1E+1).toDouble();
    PrNoise5           =ini.value  ("opt/prnoise5",    1E+1).toDouble();

    RovPosType         =ini.value("opt/rovpostype",     0).toInt();
    RefPosType         =ini.value("opt/refpostype",     0).toInt();
    RovPos[0]          =ini.value  ("opt/rovpos1",      0.0).toDouble();
    RovPos[1]          =ini.value  ("opt/rovpos2",      0.0).toDouble();
    RovPos[2]          =ini.value  ("opt/rovpos3",      0.0).toDouble();
    RefPos[0]          =ini.value  ("opt/refpos1",      0.0).toDouble();
    RefPos[1]          =ini.value  ("opt/refpos2",      0.0).toDouble();
    RefPos[2]          =ini.value  ("opt/refpos3",      0.0).toDouble();
    RovAntPcv          =ini.value("opt/rovantpcv",      0).toInt();
    RefAntPcv          =ini.value("opt/refantpcv",      0).toInt();
    RovAnt             =ini.value ("opt/rovant",        "").toString();
    RefAnt             =ini.value ("opt/refant",        "").toString();
    RovAntE            =ini.value  ("opt/rovante",      0.0).toDouble();
    RovAntN            =ini.value  ("opt/rovantn",      0.0).toDouble();
    RovAntU            =ini.value  ("opt/rovantu",      0.0).toDouble();
    RefAntE            =ini.value  ("opt/refante",      0.0).toDouble();
    RefAntN            =ini.value  ("opt/refantn",      0.0).toDouble();
    RefAntU            =ini.value  ("opt/refantu",      0.0).toDouble();

    RnxOpts1           =ini.value ("opt/rnxopts1",      "").toString();
    RnxOpts2           =ini.value ("opt/rnxopts2",      "").toString();
    PPPOpts            =ini.value ("opt/pppopts",       "").toString();



    AntPcvFile         =ini.value ("opt/antpcvfile",    "").toString();
    strcpy(temppath,GetIniKeyString("opt","antpcvfile","",charArrayIni));
    AntPcvFile=QString(temppath);


    IntpRefObs         =ini.value("opt/intprefobs",     0).toInt();
    SbasSat            =ini.value("opt/sbassat",        0).toInt();
    NetRSCorr          =ini.value("opt/netrscorr",      0).toInt();
    SatClkCorr         =ini.value("opt/satclkcorr",     0).toInt();
    SbasCorr           =ini.value("opt/sbascorr",       0).toInt();
    SbasCorr1          =ini.value("opt/sbascorr1",      0).toInt();
    SbasCorr2          =ini.value("opt/sbascorr2",      0).toInt();
    SbasCorr3          =ini.value("opt/sbascorr3",      0).toInt();
    SbasCorr4          =ini.value("opt/sbascorr4",      0).toInt();
    SbasCorrFile       =ini.value ("opt/sbascorrfile",  "").toString();
    PrecEphFile        =ini.value ("opt/precephfile",   "").toString();
    SatPcvFile         =ini.value ("opt/satpcvfile",    "").toString();
    strcpy(temppath,GetIniKeyString("opt","satpcvfile","",charArrayIni));
    SatPcvFile=QString(temppath);


    StaPosFile         =ini.value ("opt/staposfile",    "").toString();
    GeoidDataFile      =ini.value ("opt/geoiddatafile", "").toString();
    IonoFile           =ini.value ("opt/ionofile",      "").toString();
    EOPFile            =ini.value ("opt/eopfile",       "").toString();
    DCBFile            =ini.value ("opt/dcbfile",       "").toString();
    BLQFile            =ini.value ("opt/blqfile",       "").toString();
    strcpy(temppath,GetIniKeyString("opt","blqfile","",charArrayIni));
    BLQFile=QString(temppath);
    //GoogleEarthFile    =ini.value ("opt/googleearthfile",GOOGLE_EARTH).toString();

    RovList="";
    for (int i=0;i<10;i++) {
        RovList +=ini.value(QString("opt/rovlist%1").arg(i+1),"").toString();
    }
    BaseList="";
    for (int i=0;i<10;i++) {
        BaseList+=ini.value(QString("opt/baselist%1").arg(i+1),"").toString();
    }
    RovList.replace("@@","\n");
    BaseList.replace("@@","\n");

    /*ExtErr.ena[0]      =ini.value("opt/exterr_ena0",    0).toInt();
    ExtErr.ena[1]      =ini.value("opt/exterr_ena1",    0).toInt();
    ExtErr.ena[2]      =ini.value("opt/exterr_ena2",    0).toInt();
    ExtErr.ena[3]      =ini.value("opt/exterr_ena3",    0).toInt();
    for (int i=0;i<3;i++) for (int j=0;j<6;j++) {
        ExtErr.cerr[i][j]=ini.value(QString("opt/exterr_cerr%1%2").arg(i).arg(j),0.3).toDouble();
    }
    for (int i=0;i<3;i++) for (int j=0;j<6;j++) {
        ExtErr.perr[i][j]=ini.value(QString("exterr_perr%1%2").arg(i).arg(j),0.003).toDouble();
    }
    ExtErr.gloicb[0]   =ini.value  ("opt/exterr_gloicb0",0.0).toDouble();
    ExtErr.gloicb[1]   =ini.value  ("opt/exterr_gloicb1",0.0).toDouble();
    ExtErr.gpsglob[0]  =ini.value  ("opt/exterr_gpsglob0",0.0).toDouble();
    ExtErr.gpsglob[1]  =ini.value  ("opt/exterr_gpsglob1",0.0).toDouble();*/

    /*convDialog->TimeSpan  ->setChecked(ini.value("conv/timespan",  0).toInt());
    convDialog->TimeIntF  ->setChecked(ini.value("conv/timeintf",  0).toInt());
    convDialog->TimeY1    ->setDate(ini.value ("conv/timey1","2000/01/01").toDate());
    convDialog->TimeH1    ->setTime(ini.value ("conv/timeh1","00:00:00"  ).toTime());
    convDialog->TimeY2    ->setDate(ini.value ("conv/timey2","2000/01/01").toDate());
    convDialog->TimeH2    ->setTime(ini.value ("conv/timeh2","00:00:00"  ).toTime());
    convDialog->TimeInt   ->setText(ini.value ("conv/timeint", "0").toString());
    convDialog->TrackColor->setCurrentIndex(ini.value("conv/trackcolor",5).toInt());
    convDialog->PointColor->setCurrentIndex(ini.value("conv/pointcolor",5).toInt());
    convDialog->OutputAlt ->setCurrentIndex(ini.value("conv/outputalt", 0).toInt());
    convDialog->OutputTime->setCurrentIndex(ini.value("conv/outputtime",0).toInt());
    convDialog->AddOffset ->setChecked(ini.value("conv/addoffset", 0).toInt());
    convDialog->Offset1   ->setText(ini.value ("conv/offset1", "0").toString());
    convDialog->Offset2   ->setText(ini.value ("conv/offset2", "0").toString());
    convDialog->Offset3   ->setText(ini.value ("conv/offset3", "0").toString());
    convDialog->Compress  ->setChecked(ini.value("conv/compress",  0).toInt());
    convDialog->FormatKML ->setChecked(ini.value("conv/format",    0).toInt());

    textViewer->Color1=ini.value("viewer/color1",QColor(Qt::black)).value<QColor>();
    textViewer->Color2=ini.value("viewer/color2",QColor(Qt::white)).value<QColor>();
    textViewer->FontD.setFamily(ini.value ("viewer/fontname","Courier New").toString());
    textViewer->FontD.setPointSize(ini.value("viewer/fontsize",9).toInt());*/
}
#else
// load options from ini file -----------------------------------------------
void postMain::LoadOpt(void)
{
    QSettings ini(IniFile,QSettings::IniFormat);
    /*TimeStart->setChecked(ini.value("set/timestart",   0).toBool());
    TimeEnd->setChecked(ini.value("set/timeend",     0).toBool());
    TimeY1->setDate(ini.value ("set/timey1",      "2000/01/01").toDate());
    TimeH1->setTime(ini.value ("set/timeh1",      "00:00:00").toTime());
    TimeY2->setDate(ini.value ("set/timey2",      "2000/01/01").toDate());
    TimeH2->setTime(ini.value ("set/timeh2",      "00:00:00").toTime());
    TimeIntF ->setChecked(ini.value("set/timeintf",    0).toBool());
    TimeInt->setCurrentText(ini.value ("set/timeint",     "0").toString());
    TimeUnitF->setChecked(ini.value("set/timeunitf",   0).toBool());
    TimeUnit->setText(ini.value ("set/timeunit",    "24").toString());
    InputFile1->setCurrentText(ini.value ("set/inputfile1",  "").toString());
    InputFile2->setCurrentText(ini.value ("set/inputfile2",  "").toString());
    InputFile3->setCurrentText(ini.value ("set/inputfile3",  "").toString());
    InputFile4->setCurrentText(ini.value ("set/inputfile4",  "").toString());
    InputFile5->setCurrentText(ini.value ("set/inputfile5",  "").toString());
    InputFile6->setCurrentText(ini.value ("set/inputfile6",  "").toString());
    OutDirEna->setChecked(ini.value("set/outputdirena", 0).toBool());
    OutDir->setText(ini.value ("set/outputdir",   "").toString());
    OutputFile->setCurrentText(ini.value ("set/outputfile",  "").toString());

    ReadList(InputFile1,&ini,"hist/inputfile1");
    ReadList(InputFile2,&ini,"hist/inputfile2");
    ReadList(InputFile3,&ini,"hist/inputfile3");
    ReadList(InputFile4,&ini,"hist/inputfile4");
    ReadList(InputFile5,&ini,"hist/inputfile5");
    ReadList(InputFile6,&ini,"hist/inputfile6");
    ReadList(OutputFile,&ini,"hist/outputfile");*/

    InputFile1         =ini.value ("set/inputfile1",  "").toString();
    InputFile2         =ini.value ("set/inputfile2",  "").toString();
    InputFile3         =ini.value ("set/inputfile3",  "").toString();
    InputFile4         =ini.value ("set/inputfile4",  "").toString();
    InputFile5         =ini.value ("set/inputfile5",  "").toString();
    InputFile6         =ini.value ("set/inputfile6",  "").toString();
    OutDirEna          =ini.value("set/outputdirena", 0).toBool();
    OutDir             =ini.value ("set/outputdir",   "").toString();
    OutputFile         =ini.value ("set/outputfile",  "").toString();



    PosMode            =ini.value("opt/posmode",        0).toInt();
    Freq               =ini.value("opt/freq",           1).toInt();
    Solution           =ini.value("opt/solution",       0).toInt();
    ElMask             =ini.value  ("opt/elmask",      15.0).toDouble();
    SnrMask.ena[0]     =ini.value("opt/snrmask_ena1",   0).toInt();
    SnrMask.ena[1]     =ini.value("opt/snrmask_ena2",   0).toInt();
    for (int i=0;i<3;i++) for (int j=0;j<9;j++) {
        SnrMask.mask[i][j]=
            ini.value(QString("opt/snrmask_%1_%2").arg(i+1).arg(j+1),0.0).toDouble();
    }
    IonoOpt            =ini.value("opt/ionoopt",     IONOOPT_BRDC).toInt();
    TropOpt            =ini.value("opt/tropopt",     TROPOPT_SAAS).toInt();
    RcvBiasEst         =ini.value("opt/rcvbiasest",     0).toInt();
    DynamicModel       =ini.value("opt/dynamicmodel",   0).toInt();
    TideCorr           =ini.value("opt/tidecorr",       0).toInt();
    SatEphem           =ini.value("opt/satephem",       0).toInt();
    BdsEphSource       =ini.value("opt/ephsource",     0).toInt();
    //BdsEphSource=4;
    ExSats             =ini.value ("opt/exsats",        "").toString();
    NavSys             =ini.value("opt/navsys",   SYS_GPS).toInt();
    PosOpt[0]          =ini.value("opt/posopt1",        0).toInt();
    PosOpt[1]          =ini.value("opt/posopt2",        0).toInt();
    PosOpt[2]          =ini.value("opt/posopt3",        0).toInt();
    PosOpt[3]          =ini.value("opt/posopt4",        0).toInt();
    PosOpt[4]          =ini.value("opt/posopt5",        0).toInt();
    PosOpt[5]          =ini.value("opt/posopt6",        0).toInt();
    MapFunc            =ini.value("opt/mapfunc",        0).toInt();

    AmbRes             =ini.value("opt/ambres",         1).toInt();
    GloAmbRes          =ini.value("opt/gloambres",      1).toInt();
    BdsAmbRes          =ini.value("opt/bdsambres",      1).toInt();
    ValidThresAR       =ini.value  ("opt/validthresar", 3.0).toDouble();
    ThresAR2           =ini.value  ("opt/thresar2",  0.9999).toDouble();
    ThresAR3           =ini.value  ("opt/thresar3",    0.25).toDouble();
    LockCntFixAmb      =ini.value("opt/lockcntfixamb",  0).toInt();
    FixCntHoldAmb      =ini.value("opt/fixcntholdamb", 10).toInt();
    ElMaskAR           =ini.value  ("opt/elmaskar",     0.0).toDouble();
    ElMaskHold         =ini.value  ("opt/elmaskhold",   0.0).toDouble();
    OutCntResetAmb     =ini.value("opt/outcntresetbias",5).toInt();
    SlipThres          =ini.value  ("opt/slipthres",   0.05).toDouble();
    MaxAgeDiff         =ini.value  ("opt/maxagediff",  30.0).toDouble();
    RejectThres        =ini.value  ("opt/rejectthres", 30.0).toDouble();
    RejectGdop         =ini.value  ("opt/rejectgdop",  30.0).toDouble();
    ARIter             =ini.value("opt/ariter",         1).toInt();
    NumIter            =ini.value("opt/numiter",        1).toInt();
    CodeSmooth         =ini.value("opt/codesmooth",     0).toInt();
    BaseLine[0]        =ini.value  ("opt/baselinelen",  0.0).toDouble();
    BaseLine[1]        =ini.value  ("opt/baselinesig",  0.0).toDouble();
    BaseLineConst      =ini.value("opt/baselineconst",  0).toInt();

    SolFormat          =ini.value("opt/solformat",      0).toInt();
    TimeFormat         =ini.value("opt/timeformat",     1).toInt();
    TimeDecimal        =ini.value("opt/timedecimal",    3).toInt();
    LatLonFormat       =ini.value("opt/latlonformat",   0).toInt();
    FieldSep           =ini.value ("opt/fieldsep",      "").toString();
    OutputHead         =ini.value("opt/outputhead",     1).toInt();
    OutputOpt          =ini.value("opt/outputopt",      1).toInt();
    OutputDatum        =ini.value("opt/outputdatum",    0).toInt();
    OutputHeight       =ini.value("opt/outputheight",   0).toInt();
    OutputGeoid        =ini.value("opt/outputgeoid",    0).toInt();
    SolStatic          =ini.value("opt/solstatic",      0).toInt();
    DebugTrace         =ini.value("opt/debugtrace",     0).toInt();
    DebugStatus        =ini.value("opt/debugstatus",    0).toInt();

    MeasErrR1          =ini.value  ("opt/measeratio1",100.0).toDouble();
    MeasErrR2          =ini.value  ("opt/measeratio2",100.0).toDouble();
    MeasErr2           =ini.value  ("opt/measerr2",   0.003).toDouble();
    MeasErr3           =ini.value  ("opt/measerr3",   0.003).toDouble();
    MeasErr4           =ini.value  ("opt/measerr4",   0.000).toDouble();
    MeasErr5           =ini.value  ("opt/measerr5",  10.000).toDouble();
    SatClkStab         =ini.value  ("opt/satclkstab", 5E-12).toDouble();
    PrNoise1           =ini.value  ("opt/prnoise1",    1E-4).toDouble();
    PrNoise2           =ini.value  ("opt/prnoise2",    1E-3).toDouble();
    PrNoise3           =ini.value  ("opt/prnoise3",    1E-4).toDouble();
    PrNoise4           =ini.value  ("opt/prnoise4",    1E+1).toDouble();
    PrNoise5           =ini.value  ("opt/prnoise5",    1E+1).toDouble();

    RovPosType         =ini.value("opt/rovpostype",     0).toInt();
    RefPosType         =ini.value("opt/refpostype",     0).toInt();
    RovPos[0]          =ini.value  ("opt/rovpos1",      0.0).toDouble();
    RovPos[1]          =ini.value  ("opt/rovpos2",      0.0).toDouble();
    RovPos[2]          =ini.value  ("opt/rovpos3",      0.0).toDouble();
    RefPos[0]          =ini.value  ("opt/refpos1",      0.0).toDouble();
    RefPos[1]          =ini.value  ("opt/refpos2",      0.0).toDouble();
    RefPos[2]          =ini.value  ("opt/refpos3",      0.0).toDouble();
    RovAntPcv          =ini.value("opt/rovantpcv",      0).toInt();
    RefAntPcv          =ini.value("opt/refantpcv",      0).toInt();
    RovAnt             =ini.value ("opt/rovant",        "").toString();
    RefAnt             =ini.value ("opt/refant",        "").toString();
    RovAntE            =ini.value  ("opt/rovante",      0.0).toDouble();
    RovAntN            =ini.value  ("opt/rovantn",      0.0).toDouble();
    RovAntU            =ini.value  ("opt/rovantu",      0.0).toDouble();
    RefAntE            =ini.value  ("opt/refante",      0.0).toDouble();
    RefAntN            =ini.value  ("opt/refantn",      0.0).toDouble();
    RefAntU            =ini.value  ("opt/refantu",      0.0).toDouble();

    RnxOpts1           =ini.value ("opt/rnxopts1",      "").toString();
    RnxOpts2           =ini.value ("opt/rnxopts2",      "").toString();
    PPPOpts            =ini.value ("opt/pppopts",       "").toString();

    AntPcvFile         =ini.value ("opt/antpcvfile",    "").toString();
    IntpRefObs         =ini.value("opt/intprefobs",     0).toInt();
    SbasSat            =ini.value("opt/sbassat",        0).toInt();
    NetRSCorr          =ini.value("opt/netrscorr",      0).toInt();
    SatClkCorr         =ini.value("opt/satclkcorr",     0).toInt();
    SbasCorr           =ini.value("opt/sbascorr",       0).toInt();
    SbasCorr1          =ini.value("opt/sbascorr1",      0).toInt();
    SbasCorr2          =ini.value("opt/sbascorr2",      0).toInt();
    SbasCorr3          =ini.value("opt/sbascorr3",      0).toInt();
    SbasCorr4          =ini.value("opt/sbascorr4",      0).toInt();
    SbasCorrFile       =ini.value ("opt/sbascorrfile",  "").toString();
    PrecEphFile        =ini.value ("opt/precephfile",   "").toString();
    SatPcvFile         =ini.value ("opt/satpcvfile",    "").toString();
    StaPosFile         =ini.value ("opt/staposfile",    "").toString();
    GeoidDataFile      =ini.value ("opt/geoiddatafile", "").toString();
    IonoFile           =ini.value ("opt/ionofile",      "").toString();
    EOPFile            =ini.value ("opt/eopfile",       "").toString();
    DCBFile            =ini.value ("opt/dcbfile",       "").toString();
    BLQFile            =ini.value ("opt/blqfile",       "").toString();
    //GoogleEarthFile    =ini.value ("opt/googleearthfile",GOOGLE_EARTH).toString();

    RovList="";
    for (int i=0;i<10;i++) {
        RovList +=ini.value(QString("opt/rovlist%1").arg(i+1),"").toString();
    }
    BaseList="";
    for (int i=0;i<10;i++) {
        BaseList+=ini.value(QString("opt/baselist%1").arg(i+1),"").toString();
    }
    RovList.replace("@@","\n");
    BaseList.replace("@@","\n");

    /*ExtErr.ena[0]      =ini.value("opt/exterr_ena0",    0).toInt();
    ExtErr.ena[1]      =ini.value("opt/exterr_ena1",    0).toInt();
    ExtErr.ena[2]      =ini.value("opt/exterr_ena2",    0).toInt();
    ExtErr.ena[3]      =ini.value("opt/exterr_ena3",    0).toInt();
    for (int i=0;i<3;i++) for (int j=0;j<6;j++) {
        ExtErr.cerr[i][j]=ini.value(QString("opt/exterr_cerr%1%2").arg(i).arg(j),0.3).toDouble();
    }
    for (int i=0;i<3;i++) for (int j=0;j<6;j++) {
        ExtErr.perr[i][j]=ini.value(QString("exterr_perr%1%2").arg(i).arg(j),0.003).toDouble();
    }
    ExtErr.gloicb[0]   =ini.value  ("opt/exterr_gloicb0",0.0).toDouble();
    ExtErr.gloicb[1]   =ini.value  ("opt/exterr_gloicb1",0.0).toDouble();
    ExtErr.gpsglob[0]  =ini.value  ("opt/exterr_gpsglob0",0.0).toDouble();
    ExtErr.gpsglob[1]  =ini.value  ("opt/exterr_gpsglob1",0.0).toDouble();*/

    /*convDialog->TimeSpan  ->setChecked(ini.value("conv/timespan",  0).toInt());
    convDialog->TimeIntF  ->setChecked(ini.value("conv/timeintf",  0).toInt());
    convDialog->TimeY1    ->setDate(ini.value ("conv/timey1","2000/01/01").toDate());
    convDialog->TimeH1    ->setTime(ini.value ("conv/timeh1","00:00:00"  ).toTime());
    convDialog->TimeY2    ->setDate(ini.value ("conv/timey2","2000/01/01").toDate());
    convDialog->TimeH2    ->setTime(ini.value ("conv/timeh2","00:00:00"  ).toTime());
    convDialog->TimeInt   ->setText(ini.value ("conv/timeint", "0").toString());
    convDialog->TrackColor->setCurrentIndex(ini.value("conv/trackcolor",5).toInt());
    convDialog->PointColor->setCurrentIndex(ini.value("conv/pointcolor",5).toInt());
    convDialog->OutputAlt ->setCurrentIndex(ini.value("conv/outputalt", 0).toInt());
    convDialog->OutputTime->setCurrentIndex(ini.value("conv/outputtime",0).toInt());
    convDialog->AddOffset ->setChecked(ini.value("conv/addoffset", 0).toInt());
    convDialog->Offset1   ->setText(ini.value ("conv/offset1", "0").toString());
    convDialog->Offset2   ->setText(ini.value ("conv/offset2", "0").toString());
    convDialog->Offset3   ->setText(ini.value ("conv/offset3", "0").toString());
    convDialog->Compress  ->setChecked(ini.value("conv/compress",  0).toInt());
    convDialog->FormatKML ->setChecked(ini.value("conv/format",    0).toInt());

    textViewer->Color1=ini.value("viewer/color1",QColor(Qt::black)).value<QColor>();
    textViewer->Color2=ini.value("viewer/color2",QColor(Qt::white)).value<QColor>();
    textViewer->FontD.setFamily(ini.value ("viewer/fontname","Courier New").toString());
    textViewer->FontD.setPointSize(ini.value("viewer/fontsize",9).toInt());*/
}
#endif
