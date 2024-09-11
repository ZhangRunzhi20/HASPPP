/*------------------------------------------------------------------------------
* HASPPP.h : HAS raw data decode
*
* options : -DENAGAL     enable Galileo (essential)
*           -DNFREQ=n    set number of obs codes/frequencies
*           -DNEXOBS=n   set number of extended obs codes
*           -DMAXOBS=n   set max number of obs data in an epoch
*           -DTRACEBNC   cash HAS satellite product in folder debugFiles
*           -DTRACEHAS   decode log in folder debugFiles
*           -DTRACECLOCK cash clock decode in folder debugFiles
*           -DTRACEPID   cash HAS page
* For example: DEFINES += _RTLDLL NO_STRICT TRACE ENAGLO ENAGAL ENACMP \
*              ENAIRN NFREQ=3 NEXOBS=3 EXTLEX TRACEBNC TRACEHAS TRACECLOCK TRACEPID
*
* Notes :  The class HASPPP can be used to decode raw High Accuracy Service (HAS) page.
*          In fact, It can support all GNSS receiver equipped with a HAS receiving module (E6B CNAV).
*          The function processCNAV directly processes the raw HAS pages, although three specific receivers has been provided,
*          Such as Septentrio M3Pro, NovAtel 729, SINO K803.
*
* Author:   zhang runzhi, National Time Service Center, Chinese Academy of Sciences
* Email :   zhangrunzhi@ntsc.ac.cn
*
*-----------------------------------------------------------------------------*/
#ifndef HASPPP_H
#define HASPPP_H

#include <string>
#include <vector>

//#include <QStringList>
#include "./rtklib.h"
//#include <QFile>
//#include <QDebug>
//#include <QString>
//#include <QVector>
//#include<QTextCodec>
//#include<QDir>
#include "./resource/gf256.h"
#include "./resource/eigen-3.4.0/Eigen/Dense"
using namespace Eigen;

#ifdef WIN32
typedef unsigned short  ushort;
typedef unsigned int    uint;
#else

#endif


#define ErrorMsgPos(myerromsg) \
{char buff[256]; sprintf(buff, "%s, line:%d, fuction:%s", __FILE__, __LINE__, __FUNCTION__); myerromsg = "----"+std::string(buff)+"----";}// __func__ or __FUNCTION__


#define GALCNAVRAWPAGELEN 116
#define MAXSATM           40
#define MAXSIG            16
#define MAXSYS            2
#define READLEN           1000


#define NVTHEADBYTE             28

#define GALCNAVNOVATEL          2239
#define CNAVOFSNOVATEL          12

#define GALCNAVSINO             1797
#define CNAVOFSSINO             2

#define SEPTENTRIO_HEADLEN      14
#define GALCNAVSEP              4024
#define CNAVOFSSEP              6

#define GALCNAVSate             4
#define ERRORINTER              10

static uint32_t U4(uint8_t *p) {uint32_t a; memcpy(&a,p,4); return a;}
static uint16_t U2(uint8_t *p) {uint16_t a; memcpy(&a,p,2); return a;}


typedef struct {
    int mid;
    int ms;
    int n_pid;
    int pid[255];
    gtime_t updateTime[255];
    MatrixXi page;
} galhas;



typedef struct {
   int sat;
   int iode;
   int SigM[MAXSIG];
   int nm;
   ssr_t ssr;
} hassatp_t;



typedef struct {
    gtime_t iodTime[6]; /*GST: toh, vaild time */
    int nsat;           /* */
    int sys[MAXSYS];    /* 0:GPS,1:Galileo */
    int iod[6];
    int nsys;
    int maskId;
    int iodSet;
    hassatp_t sat[MAXSAT];  /* */
} hassat_t;



typedef struct {        /* has message type */
    int week;
    double tow;       /* receiption time */
    int prn;            /* has satellite PRN number */
    gtime_t gpsTime;
//    QString data; /* b2b message (486bit) padded by 0 */
    std::string data;
} hasmsg_t;

typedef struct {
//    QVector <hasmsg_t> hasInfo;
    std::vector <hasmsg_t> hasInfo;
    int readReady;
    int max;
    int errorLen;
} hasInfo;

typedef struct{
    int hasSsrByte;
    int hasSsrBYteMax;
    int hasReadByteReady;
    char *binBuff;
}HasSsrBin;

typedef struct{
    int errorMid;
    gtime_t errorTime;
    int errorNum;
}typeError;

class HASPPP
{
public:
	 HASPPP(char exepath[]);// exepath is used to find folder resource
	~HASPPP();

	/* Step 3:
		1. Set ALL HAS raw data path by the function setHasFile
		2. Read ALL data set
		3. The simulated real-time decoding HAS SSR, updated to the observed time (double *obsTime)
		4. Obtain the results from the HAS decoding (Format: ssr_t)

	*/
//    void testHASExa();
//    void testHASExa1();

    void setHasFile(std::vector<std::string>receiverFilePath,ushort hasFormat = 1);//set ALL HAS cashed files,format: 1:SINO,2:NovAtel, 3: Septentrio
//   // bool readHasFiles();
    bool readHasBinFiles();/* Read the cashed HAS raw data*/
    void updateHasCorrection(double *obsTime); /* Decoding function, obsTime is the time of observation*/
	ssr_t * getSsr();  /* return the decoding results*/



    /*tool*/
    gtime_t epoch2time(const double *ep);
    void time2epoch(gtime_t t, double *ep);
    //change by zrz 2024.4.27 private->public
    void decodeRs(MatrixXi pageIn,int *pid,int pidLen,std::string &OutString);
    //bool decodePage(QString page,int pidLen,gtime_t gpstTime,hassat_t hasSat_i,hassat_t &hasSat);
    bool decodePageChange(std::string page,int pidLen,gtime_t gpstTime,const hassat_t *hasSat_i,hassat_t *hasSat_o);
    void initHassat_t(hassat_t &has_t);

private:
    typeError m_typeErr;
//    QStringList m_hasFiles;
    std::vector<std::string> m_hasFiles; // ZRZ
    ushort m_hasFormat;
    //hasInfo m_hasInfo;
    HasSsrBin m_hasBin;
    galhas m_galhas;
    galhas m_galhasSpare;

//    QString m_gPath,m_hasInvPath,m_workContent;
    std::string m_gPath,m_hasInvPath,m_workContent;
    MatrixXi m_gMatrix;
    MatrixXi m_refForm;


    gtime_t m_updatedGPSTime;
    hassat_t m_hasSat;

    /* trace */
    int     m_traceLevel;
//    QString m_tracePath;
    std::string m_tracePath;
    bool    m_traceEn;
//    std::ofstream m_pf;
    nav_t m_navs;

    /* change by zhangrunzhi 2023 2.2  -> test clock*/
    gtime_t m_obsGTime;
	void comEU(std::string pageString);
	void comIOD(std::string pageString);
	void fateErro(std::string s);
	//    void testFastSlow();
	void testClock(int sat, double ck, gtime_t timeUpdate, gtime_t timeVai);
	void testPid(gtime_t timeTe, int MS, int MID, int n_pid, int*pid, MatrixXi page, std::string pageString);
	void testPage(MatrixXi pageIn, int *pid, int pidLen, gtime_t timeTe, int MS, int MID);
	void testFrameTime(gtime_t timeFrame);
	std::string hex2string(char* datahex, int len);
	int decodeSINOHead(uint8_t navCharSina[64], gtime_t *gpstFrame, int &readOffset);
	int decodeNovAtelHead(uint8_t navCharSina[64], gtime_t *gpstFrame, int &readOffset);
	int decodeSeptentrioHead(uint8_t navCharSina[64], gtime_t *gpstFrame, int &readOffset);
	void setbitu(unsigned char *buff, int pos, int len, unsigned int data);

    void processCNAV(uint8_t *dataUChar,gtime_t gpstFrame,int cnavoffset);
    void sortUnique(const int arr[], int n, int result[], int& resultSize);
    int sortpid(const galhas galhas_i,int &n_pid,int *pid,MatrixXi &finPage);
    void initHas(galhas &galHas);
    void initHassatp_t(hassatp_t &hassat);

    void initMatrix();
    int  prn2sat(int gnssId,int svid);
    int  satOrderPos(hassat_t hasSat,int sat);
    void MatrixInv(MatrixXi matrixIn,MatrixXi &matrixOut);
    int MatrixEleInv(MatrixXi matrixXi,int data);
    void decodePageHead(uint8_t *buffer,int *iod);
    bool decodeMask(uint8_t *buffer,hassat_t &hasSat,int *indexF);
    bool decodeOrbit(uint8_t *buffer,hassat_t &hasSat,int *indexF);
    bool decodeCF(uint8_t *buffer,hassat_t &hasSat,int *indexF);
    bool decodeCS(uint8_t *buffer,hassat_t &hasSat,int *indexF);
    bool decodeCodeBias(uint8_t *buffer,hassat_t &hasSat,int *indexF);
    bool decodePhaseBias(uint8_t *buffer,hassat_t &hasSat,int *indexF);

    int cbiasCode2Index(int sat,int cbindex);
    int sat2sys(int sat);
    gtime_t gpst2gstM(gtime_t gpst);
    gtime_t gst2gpstM(gtime_t gst);
    void traces(int tracelevel,gtime_t time,std::string ErrorMsg);
    bool adjustPos(uint8_t *dataChar,int index,int indexFin);
    void updateSSR(hassat_t hasSat,nav_t &nav);

    /* tool*/
    uint32_t getbitu(const uint8_t *buff, int pos, int len);
    int32_t getbits(const uint8_t *buff, int pos, int len);
    gtime_t timeadd(gtime_t t, double sec);
    double timediff(gtime_t t1, gtime_t t2);
    void satno2id(int sat, char *id);
    gtime_t gpst2time(int week, double sec);
    int satid2no(const char *id);
    int satno(int sys, int prn);
    int satsys(int sat, int *prn);
    char *code2obs(uint8_t code);
    uint8_t obs2code(const char *obs);

};

static char cbiascodes[2][16][3]={
    {"1C",  "",  "","1S","1L","1X","2S","2L","2X","2P",  "","5I","5Q","5X",  "",  ""},
    {"1B","1C","1X","5I","5Q","5X","7I","7Q","7X","8I","8Q","8X","6B","6C","6X",  ""}
    //{"1C","1P","2C",  "",  "",  "",  "",  "",  "",  "",  "",  "",  "",  "",  "",  ""},
    //{  "","1B","1C",  "","5Q","5I",  "","7I","7Q",  "",  "","6C",  "",  "",  "",  ""}
};

#endif // HASPPP_H
