#include "HASPPP.h"
//#include <QDebug>
#include <fstream>
#include <iostream>
#include <set>
#define HAS_SINO       1
#define HAS_NOVATEL    2
#define HAS_SEPTENTRIO 3

#ifdef WIN32
#define FILEPATHSEP '\\'
#else
#define FILEPATHSEP '/'
#endif

using namespace std;

static char *obscodes[]={       /* observation code strings */

    ""  ,"1C","1P","1W","1Y", "1M","1N","1S","1L","1E", /*  0- 9 */
    "1A","1B","1X","1Z","2C", "2D","2S","2L","2X","2P", /* 10-19 */
    "2W","2Y","2M","2N","5I", "5Q","5X","7I","7Q","7X", /* 20-29 */
    "6A","6B","6C","6X","6Z", "6S","6L","8L","8Q","8X", /* 30-39 */
    "2I","2Q","6I","6Q","3I", "3Q","3X","1I","1Q","5A", /* 40-49 */
    "5B","5C","9A","9B","9C", "9X","1D","5D","5P","5Z", /* 50-59 */
    "6E","7D","7P","7Z","8D", "8P","4A","4B","4X",""    /* 60-69 */
};

HASPPP::HASPPP(char exepath[])
{

    m_typeErr.errorMid = -1;
    m_typeErr.errorTime = {0};
    m_typeErr.errorNum = 0;
    m_hasFiles.clear();
    m_hasFormat = 1;
    //m_hasInfo.max = 0;
    //m_hasInfo.readReady = 0;
    //m_hasInfo.errorLen = 0;
    //m_hasInfo.hasInfo.clear();
    m_hasBin.hasReadByteReady=0;
    m_hasBin.hasSsrByte=0;
    m_hasBin.hasSsrBYteMax=0;
    m_hasBin.binBuff=NULL;

    for(int i = 0;i<1;i++){
        initHas(m_galhas);
        initHas(m_galhasSpare);
    }

//    QString strCurPath = QDir::currentPath();
    std::string strCurPath = std::string(exepath);

    m_workContent = strCurPath.substr( 0, strCurPath.find_last_of(FILEPATHSEP));
    m_workContent = m_workContent.substr( 0, m_workContent.find_last_of(FILEPATHSEP));
//    m_workContent = m_workContent.substr( 0, m_workContent.find_last_of(FILEPATHSEP));// change for windows QT,2024 2 22
//    int index = strCurPath.lastIndexOf('/');
//    m_workContent = strCurPath.left(index);
    m_gPath = m_workContent +FILEPATHSEP+"resource"+FILEPATHSEP+"GMatrix.txt"; // Decoding coefficient from HAS ICD
    m_hasInvPath = m_workContent +FILEPATHSEP+"resource"+FILEPATHSEP+"HASInv.txt"; // Galois field (GF) multiplication table derived from Matlab
    cout << m_gPath << endl;
    initMatrix();
    m_updatedGPSTime = {0};

    initHassat_t(m_hasSat);
    memset(&m_navs,0,sizeof(nav_t)) ;
    m_navs.eph =NULL;
    m_navs.geph =NULL;
    m_navs.seph =NULL;
    m_navs.peph = NULL;
    m_navs.pclk =NULL;
    m_navs.alm =NULL;
    m_navs.tec = NULL;

#ifdef TRACEHAS
    m_traceLevel = 2;
    m_tracePath  = m_workContent+FILEPATHSEP+"debugFiles"+FILEPATHSEP+"trace.txt";
    m_traceEn    = true;

//    QTextCodec *code = QTextCodec::codecForName("GB2312");
//    std::string name = code->fromUnicode(m_tracePath).data();
//    m_pf = fopen(name.c_str(),"w");
//    m_pf = fopen(trace_path_char,"w");


//    if (m_pf == NULL)
//    {
//    }
//    else{
//        QFile file;
//        if(file.open(m_pf,QIODevice::WriteOnly)){
//            m_traceEn    = true;
//            QTextStream out(&file);
//            file.close();
//        }

//    }
#else
    m_traceEn    = false;
#endif
}
HASPPP::~HASPPP(){
    free(m_hasBin.binBuff);
    m_hasBin.binBuff = NULL;
}

/* convert calendar day/time to time -------------------------------------------
* convert calendar day/time to gtime_t struct
* args   : double *ep       I   day/time {year,month,day,hour,min,sec}
* return : gtime_t struct
* notes  : proper in 1970-2037 or 1970-2099 (64bit time_t)
*-----------------------------------------------------------------------------*/
gtime_t HASPPP::epoch2time(const double *ep)
{
    const int doy[]={1,32,60,91,121,152,182,213,244,274,305,335};
    gtime_t time={0};
    int days,sec,year=(int)ep[0],mon=(int)ep[1],day=(int)ep[2];

    if (year<1970||2099<year||mon<1||12<mon) return time;

    /* leap year if year%4==0 in 1901-2099 */
    days=(year-1970)*365+(year-1969)/4+doy[mon-1]+day-2+(year%4==0&&mon>=3?1:0);
    sec=(int)floor(ep[5]);
    time.time=(time_t)days*86400+(int)ep[3]*3600+(int)ep[4]*60+sec;
    time.sec=ep[5]-sec;
    return time;
}
void HASPPP::traces(int tracelevel,gtime_t gpstime,std::string ErrorMsg){

#ifdef TRACEHAS
    double time[6];
    char buffstr[1024];
    time2epoch(gpstime,time);
    if(tracelevel > m_traceLevel || m_traceEn == false){
        return;
    }
    else{
        char trace_path_char[1024];
        memcpy(trace_path_char,m_tracePath.data(), m_tracePath.size()*sizeof(char));// change 1024->  m_tracePath.size()
		trace_path_char[m_tracePath.size()]='\0';
        std::ofstream file_ofs(trace_path_char,std::ios::app);
//        m_pf = file_ofs;

        if(!file_ofs.is_open()){
            cout<<"traces file open error!"<<endl;
            return;
        }
        std::sprintf(buffstr,"%d %.0f-%.0f-%.0f %.0f:%.0f:%.2f  ",tracelevel,time[0],time[1],time[2],time[3],time[4],time[5]);
        file_ofs << std::string(buffstr)+ErrorMsg<<endl;;
        file_ofs.close();

//        QFile file;
//        if(file.open(m_pf,QIODevice::WriteOnly|QIODevice::Append)){
//            QTextStream out(&file);
//            out<<QString::number(tracelevel)+QString(" ")+QString::number(time[0])+"-"+QString::number(time[1])+"-"+QString::number(time[2])+" "+QString::number(time[3])+":"+QString::number(time[4])+":"+QString::number(time[5])+QString("  ")+ErrorMsg<<endl;
//            file.close();
//        }


    }

#else
#endif
}
void HASPPP::initMatrix(){
	/* Read decoding Matrix from files in folder resource*/
    char g_Path_char[1024];
    char templinechar[102400];
    char* token;
    MatrixXi GMatrix;
    GMatrix.resize(255,32);
    GMatrix.setZero(255,32);
    memcpy(g_Path_char,m_gPath.data(), m_gPath.size()*sizeof(char)); //change 2024 2 22,1024-> m_gPath.size()
	g_Path_char[m_gPath.size()]='\0';
    FILE *fp_g_path = fopen(g_Path_char,"r");
//    QFile Gfile(m_gPath);
//    if(!Gfile.open(QFile::ReadOnly)){
//        return ;
//    }
    if (fp_g_path==NULL){
        cout<< "m_gPath open error!"<<endl;
    }

    int j=0,i;
//    QString tempLine;
//    QStringList tempList;

    while (fgets(templinechar,10240,fp_g_path)){
        token = strtok(templinechar," ");
        GMatrix(j,0) = atoi(token);
        for (i=1;i<32;i++){
            token = strtok(NULL," ");
            GMatrix(j,i) = atoi(token);
        }
        j++;
    }
    fclose(fp_g_path);

//    while(!Gfile.atEnd()){
//        tempLine=Gfile.readLine();
//        tempList=tempLine.split(' ');
//        for(int i=0;i<32;i++){
//            GMatrix(j,i)=tempList.at(i).toUInt();
//        }
//        j++;
//    }
//    Gfile.close();
    m_gMatrix = GMatrix;

    m_refForm.resize(256,256);
    m_refForm.setZero(256,256);

    memcpy(g_Path_char,m_hasInvPath.data(), m_hasInvPath.size()*sizeof(char)); //change 2024 2 22, 1024->m_hasInvPath.size()
	g_Path_char[m_hasInvPath.size()] = '\0';
    FILE *fp_hasinv_path = fopen(g_Path_char,"r");
    if (fp_hasinv_path==NULL){
        cout<< "m_hasInvPath open error!"<<endl;
    }

    j = 0;
    while (fgets(templinechar,102400,fp_hasinv_path)){
        token = strtok(templinechar," ");
        m_refForm(j,0) = atoi(token);
        for (i=1;i<256;i++){
            token = strtok(NULL," ");
            m_refForm(j,i) = atoi(token);
        }
        j++;
    }
    fclose(fp_hasinv_path);


//    QFile Formfile(m_hasInvPath);
//    if(!Formfile.open(QFile::ReadOnly)){
//        return ;
//    }
//    j=0;
//    while(!Formfile.atEnd()){
//        tempLine=Formfile.readLine();
//        tempList=tempLine.split(' ');
//        for(int i=0;i<256;i++){
//            m_refForm(j,i)=tempList.at(i).toUInt();
//        }
//        j++;
//    }
//    Formfile.close();
}
void HASPPP::setHasFile(std::vector<std::string> receiverFilePath,ushort hasFormat){
    m_hasFiles = receiverFilePath;
    m_hasFormat = hasFormat;
    for (int i=0;i<m_hasFiles.size();i++){
        cout<< "HASFile "<<i<<": "<<m_hasFiles.at(i)<<endl;
    }
//    qDebug()<<m_hasFiles<<endl;
}
bool HASPPP::readHasBinFiles(){
    char file_char[1024];
    if(m_hasFiles.size()==0){
       return false;
    }
    int fileNum = m_hasFiles.size();

    for (int i = 0;i<fileNum;i++){
        std::string fileTemp = m_hasFiles.at(i);
        memcpy(file_char,fileTemp.data(), fileTemp.size()*sizeof(char)); //change 2024 2 22,1024-> fileTemp.size()
		file_char[fileTemp.size()] = '\0';
        FILE *fp_has = fopen(file_char,"rb");
        if (fp_has == NULL){
            cout<< "read HAS binary file error!" <<endl;
            return false;
        }
//        QFile hasInfo(fileTemp);
//        if(!hasInfo.open(QFile::ReadOnly)){
//            qDebug() << "open file path " <<fileTemp << "is failed";
//            continue;
//        }

        char aa[READLEN];
//        int len = hasInfo.read(aa,READLEN);
        int len = fread(aa,sizeof(char),READLEN,fp_has);
        while(len != 0){
            if (m_hasBin.hasSsrByte + READLEN>=m_hasBin.hasSsrBYteMax) {
                m_hasBin.hasSsrBYteMax=m_hasBin.hasSsrBYteMax==0?1024:m_hasBin.hasSsrBYteMax+100000;
                if (!(m_hasBin.binBuff=(char *)realloc(m_hasBin.binBuff,m_hasBin.hasSsrBYteMax*sizeof(char)))) {
                    free(m_hasBin.binBuff); m_hasBin.binBuff=NULL; m_hasBin.hasSsrByte=m_hasBin.hasSsrBYteMax=0;
                    return false;
                }
            }
            memcpy(m_hasBin.binBuff +m_hasBin.hasSsrByte,aa,len*sizeof(char) );
            m_hasBin.hasSsrByte = m_hasBin.hasSsrByte +len;
//            len = hasInfo.read(aa,READLEN);
            len = fread(aa,sizeof(char),READLEN,fp_has);
        }

//        hasInfo.close();
        fclose(fp_has);
    }
    return true;
}

/*bool HASPPP::readHasFiles(){
    if(m_hasFiles.length()==0){
       return false;
    }
    int fileNum = m_hasFiles.length();
    for (int i = 0;i<fileNum;i++){
        QString fileTemp = m_hasFiles.at(i);
        QFile hasInfo(fileTemp);
        if(!hasInfo.open(QFile::ReadOnly)){
            qDebug() << "open file path " <<fileTemp << "is failed";
            continue;
        }
        QString lineTemp ;
        QStringList navHead,navBody;
        double week,tow;
        double prn;
        QString data;
        gtime_t logtime;
        hasmsg_t hasMsg;
        while (!hasInfo.atEnd()) {
            lineTemp = hasInfo.readLine();
            if(lineTemp.contains("GALCNAVRAWPAGE")){
            //if(lineTemp.contains("GALCNAVRAWPAGE") && lineTemp.contains("02000800")){
                lineTemp.replace(QRegExp("[\\s]+"), " ");
                navHead = lineTemp.trimmed().split(" ");
                if(!navHead.at(0).contains("GALCNAVRAWPAGE")){
                    continue;
                }
                week = navHead.at(5).toDouble();
                tow = navHead.at(6).toDouble();
                //tow = tow/1000;
                logtime = gpst2time(week, tow);

                lineTemp = hasInfo.readLine();
                lineTemp.replace(QRegExp("[\\s]+"), " ");
                navBody = lineTemp.split(' ');
                prn = navBody.at(2).toDouble();
                data = navBody.at(4);
                if (data.length() != GALCNAVRAWPAGELEN){
                    m_hasInfo.errorLen++;
                    qDebug()<< QString::number(m_hasInfo.errorLen);
                    continue;
                }
                hasMsg.data = data;
                hasMsg.gpsTime = logtime;
                hasMsg.week = week;
                hasMsg.tow = tow;
                hasMsg.prn = prn;
                m_hasInfo.hasInfo.append(hasMsg);
                m_hasInfo.max++;
            }

        }
        hasInfo.close();

    }
}*/
void HASPPP::initHas(galhas &galHas){
    galHas.mid=-1;
    galHas.ms=-1;
    galHas.n_pid=-1;
    memset(galHas.pid,-1,255*sizeof(int));
    galHas.page.resize(255,53);
    memset(galHas.updateTime,0,255*sizeof(gtime_t));
}
void HASPPP::initHassatp_t(hassatp_t &hassat){
    hassat.sat = 0;
    //memset(hassat.iodcorr,0,6*sizeof(int));
    hassat.iode = -1;
    memset(hassat.SigM,0,MAXSIG*sizeof(int));
    hassat.nm = 0;
    for(int i = 0;i<6;i++){
        hassat.ssr.t0[i].sec = 0;
        hassat.ssr.t0[i].time = 0;
    }
    memset(hassat.ssr.udi,0,6*sizeof(double));
    memset(hassat.ssr.iod,0,6*sizeof(int));
    hassat.ssr.iode = 0;
    hassat.ssr.iodcrc = 0;
    hassat.ssr.ura = 0;
    hassat.ssr.refd = 0;
    memset(hassat.ssr.deph,0,3*sizeof(double));
    memset(hassat.ssr.ddeph,0,3*sizeof(double));
    memset(hassat.ssr.dclk,0,3*sizeof(double));
    hassat.ssr.hrclk = 0;
    memset(hassat.ssr.cbias,0,MAXCODE*sizeof(float));
    memset(hassat.ssr.pbias,0,MAXCODE*sizeof(double));
    memset(hassat.ssr.stdpb,0,MAXCODE*sizeof(float));


}
void HASPPP::initHassat_t(hassat_t &has_t){
    has_t.nsat = 0;
    has_t.nsys = 0;
    has_t.maskId = 0;
    has_t.iodSet = 0;
    for(int i = 0;i<6;i++){
        has_t.iodTime[i].sec = 0;
        has_t.iodTime[i].time = 0;
    }
    memset(has_t.iod,0,6*sizeof(int));
    memset(has_t.sys,0,MAXSYS*sizeof(int));
    for(int i = 0;i<MAXSAT;i++){
        initHassatp_t(has_t.sat[i]);
    }
}
int HASPPP::MatrixEleInv(MatrixXi matrixXi,int data){
    for(int i=0;i<256;i++){
        if(matrixXi(data,i)==1){
            return i;
        }
    }
    return -1;
}
void HASPPP::fateErro(std::string pageString){
    char fate_Erro_char[1024];
    std::string matchResults = m_workContent+"/debugFiles"+"/fateErro.txt";
    memcpy(fate_Erro_char,matchResults.data(), matchResults.size()*sizeof(char));  //change 2024 2 22, 1024->  matchResults.size()
	fate_Erro_char[matchResults.size()] = '\0';
    std::ofstream file_ofs(fate_Erro_char,std::ios::app);
    if(!file_ofs.is_open()){
        cout<<"fateErro file open error!"<<endl;
        return;
    }
    file_ofs <<pageString<<endl;
    file_ofs.close();
//    QFile saveFile(matchResults);//Saved file
//    if (!saveFile.open(QFile::WriteOnly|QFile::Text|QFile::Append))
//    {
//        return;
//    }
//    QTextStream saveFileOut(&saveFile);
//    saveFileOut<<pageString<<endl;

//    saveFileOut.flush();
//    saveFile.close();
}
void HASPPP::MatrixInv(MatrixXi matrixIn,MatrixXi &matrixOut){
	/*Look up the GF table and calculate the inverse of the matrixIn */
    int row=matrixIn.rows();
    int col=matrixIn.cols();
    MatrixXi H,Horig;
    H.resize(row,2*col);
    H.setZero(row,2*col);

    for(int i=0;i<row;i++){
        for(int j=0;j<row;j++){
            H(i,j)=matrixIn(i,j);
            if(i==j){
               H(i,j+row)=1;
            }
        }
    }
    //change by zhangrunzhi 2023_5_31
    int jj=0;
    for(int i=0;i<row;i++){
        if(H(i,i)==0){
            Horig = H.row(i);
            for(jj=i+1;jj<row;jj++){
                if(H(jj,i)!=0){
                    for(int ii=0;ii<2*col;ii++){
                        H(i,ii) = H(jj,ii);
                        H(jj,ii) = Horig(0,ii);
                    }
                    break;
                }
            }
            if(jj==row+1){
                fateErro(std::string("DINV Error!"));
            }
        }
    }



    MatrixXi tempH=H;
    MatrixXi tronTempH;
    int tempData;
    for(int j=0;j<col;j++){
        tempData = MatrixEleInv(m_refForm,tempH(j,j));
        if(tempData == -1){
            continue;
        }
        for(int k=j;k<2*col;k++){
            tempH(j,k)=m_refForm(tempData,tempH(j,k));
        }
        for(int i=j+1;i<row;i++){
            tempData=tempH(i,j);
            tempH(i,j)=0;
            for(int k=j+1;k<2*col;k++){
                tempH(i,k)=gf256_add(tempH(i,k),m_refForm(tempH(j,k),tempData));
            }
        }

    }
    for(int i=col-1;i>0;i--){
        for(int j=i-1;j>=0;j--){
            tempData=tempH(j,i);
            tempH(j,i)=0;
            for(int k=i+1;k<2*col;k++){
                tempH(j,k)=gf256_add(tempH(j,k),m_refForm(tempH(i,k),tempData));
            }
        }
    }
    matrixOut.resize(row,col);
    matrixOut.setZero(row,col);
    for(int i=0;i<row;i++){
        for(int j=col;j<2*col;j++)
            matrixOut(i,j-col)=tempH(i,j);
    }

}
//void HASPPP::testHASExa1(){
//    QString pathHead="/home/david/Documents/math/gf256-master/pid1/",path;
//    QDir Dir(pathHead);
//    Dir.setFilter(QDir::Files);
//    int j=0;
//    QStringList list = Dir.entryList(QDir::Files);
//    MatrixXi pid,index;
//    int pidLen;
//    int n_pid[list.length()];
//    index.resize(1,list.length());
//    index.setZero(1,list.length());
//    pid.resize(list.length(),53);
//    pid.setZero(list.length(),53);
//    int dataIn;
//    QString tempLine;
//    QStringList tempList;
//    QString pageString;
//    for(int i=0;i<list.length();i++){
//        path=pathHead+list.at(i);
//        QFile e6bInfo(path);
//        if(!e6bInfo.open(QFile::ReadOnly)){
//            return ;
//        }
//        j=0;
//        while(!e6bInfo.atEnd()){
//            tempLine=e6bInfo.readLine();
//            tempList=tempLine.split('\r');
//            pid(i,j)=tempList.at(0).toUInt();
//            j++;
//        }
//        index(0,i)=list.at(i).mid(3,3).toInt();
//    }
//    pidLen = list.length();
//    for(int i=0;i<pidLen;i++){
//        n_pid[i]=index(0,i);
//    }

//    decodeRs(pid,n_pid,pidLen,pageString);

//    path="/home/david/Documents/math/gf256-master/hasDecode1.txt";
//    QFile Decodefile(path);
//    if(!Decodefile.open(QFile::ReadOnly)){
//        return ;
//    }
//    j=0;
//    QString decodeString;
//    while(!Decodefile.atEnd()){
//        tempLine=Decodefile.readLine();
//        if(j!=19)
//            decodeString=decodeString+tempLine.mid(0,81);
//        else
//            decodeString=decodeString+tempLine.mid(0,51);
//        j++;
//    }
//    Decodefile.close();
//    QVector <int>my;
//    my.clear();
//    for(int jj=0;jj<212;jj++){
//        if(QString::compare(decodeString.mid(jj,1),pageString.mid(jj,1))!=0)
//            my.append(jj);
//    }
//    qDebug()<<"decode RS :"<<my.length()<<endl;
//    hassat_t hasSat;
//    double date[6]={2022,12,2,0,0,0};
//    gtime_t gpsTimeTemp=epoch2time(date);
//    if(decodePageChange(pageString,pidLen,gpsTimeTemp,m_hasSat,hasSat)){
//        m_hasSat = hasSat;
//        updateSSR(hasSat,m_navs);
//    }
//}
//void HASPPP::testHASExa(){
//    QString pathHead="/home/david/Documents/math/gf256-master/pid/",path;
//    QDir Dir(pathHead);
//    Dir.setFilter(QDir::Files);
//    int j=0;
//    QStringList list = Dir.entryList(QDir::Files);
//    MatrixXi pid,index;
//    int pidLen;
//    int n_pid[list.length()];
//    index.resize(1,list.length());
//    index.setZero(1,list.length());
//    pid.resize(list.length(),53);
//    pid.setZero(list.length(),53);
//    int dataIn;
//    QString tempLine;
//    QStringList tempList;
//    QString pageString;
//    for(int i=0;i<list.length();i++){
//        path=pathHead+list.at(i);
//        QFile e6bInfo(path);
//        if(!e6bInfo.open(QFile::ReadOnly)){
//            return ;
//        }
//        j=0;
//        while(!e6bInfo.atEnd()){
//            tempLine=e6bInfo.readLine();
//            tempList=tempLine.split('\r');
//            pid(i,j)=tempList.at(0).toUInt();
//            j++;
//        }
//        index(0,i)=list.at(i).mid(3,3).toInt();
//    }
//    pidLen = list.length();
//    for(int i=0;i<pidLen;i++){
//        n_pid[i]=index(0,i);
//    }

//    decodeRs(pid,n_pid,pidLen,pageString);

//    path="/home/david/Documents/math/gf256-master/hasDecode.txt";
//    QFile Decodefile(path);
//    if(!Decodefile.open(QFile::ReadOnly)){
//        return ;
//    }
//    j=0;
//    QString decodeString;
//    while(!Decodefile.atEnd()){
//        tempLine=Decodefile.readLine();
//        if(j!=19)
//            decodeString=decodeString+tempLine.mid(0,81);
//        else
//            decodeString=decodeString+tempLine.mid(0,51);
//        j++;
//    }
//    Decodefile.close();
//    QVector <int>my;
//    my.clear();
//    for(int jj=0;jj<decodeString.length();jj++){
//        if(QString::compare(decodeString.mid(jj,1),pageString.mid(jj,1))!=0)
//            my.append(jj);
//    }
//    qDebug()<<"decode RS :"<<my.length()<<endl;
//    hassat_t hasSat;
//    double date[6]={2022,12,2,0,0,0};
//    gtime_t gpsTimeTemp=epoch2time(date);
//    if(decodePageChange(pageString,pidLen,gpsTimeTemp,m_hasSat,hasSat)){
//        m_hasSat = hasSat;
//        updateSSR(hasSat,m_navs);
//    }


//}
void HASPPP::decodeRs(MatrixXi pageIn,int *pid,int pidLen,std::string& OutString){
	/* Decode RS*/

    MatrixXi D;
    D.resize(pidLen,pidLen);
    D.setZero(pidLen,pidLen);

    for(int i=0;i<pidLen;i++){
        for(int k=0;k<pidLen;k++){
            D(i,k)=m_gMatrix(pid[i]-1,k);
        }
        //cout<<D<<endl;
    }

    MatrixXi DInv;
    MatrixXi Dtemp=D;
    MatrixXi Ad = D.adjoint();
    MatrixInv(Dtemp,DInv);

    /* test */
    /*QString matchResults = "/home/david/Documents/rtklib_b34_my/HASNovAtel/data/DZhang.txt";
    QFile saveFile(matchResults);//Saved file
    if (!saveFile.open(QFile::WriteOnly|QFile::Text))
    {
        return;
    }
    QTextStream saveFileOut(&saveFile);
    for(int k = 0;k<D.rows();k++){
        for(int m = 0;m < D.cols()-1;m++){
            saveFileOut.setFieldWidth(4);
            saveFileOut<<QString::number(D(k,m));
            saveFileOut.setFieldWidth(1);
            saveFileOut<<" ";
        }
        saveFileOut.setFieldWidth(4);
        saveFileOut<<QString::number(D(k,D.cols()-1));
        saveFileOut.setFieldWidth(1);
        saveFileOut<<endl;
    }
    saveFileOut.flush();
    saveFile.close();

    matchResults = "/home/david/Documents/rtklib_b34_my/HASNovAtel/data/DInvZhang.txt";
    QFile saveFile1(matchResults);//Saved file
    if (!saveFile1.open(QFile::WriteOnly|QFile::Text))
    {
        return;
    }
    QTextStream saveFileOut1(&saveFile1);
    for(int k = 0;k<DInv.rows();k++){
        for(int m = 0;m < DInv.cols()-1;m++){
            saveFileOut1.setFieldWidth(4);
            saveFileOut1<<QString::number(DInv(k,m));
            saveFileOut1.setFieldWidth(1);
            saveFileOut1<<" ";
        }
        saveFileOut1.setFieldWidth(4);
        saveFileOut1<<QString::number(DInv(k,DInv.cols()-1));
        saveFileOut1.setFieldWidth(1);
        saveFileOut1<<endl;
    }
    saveFileOut.flush();
    saveFile.close();*/





    MatrixXi w;
    w.resize(1,pidLen);
    w.setZero(1,pidLen);
    MatrixXi mk;
    mk.resize(pidLen,53);
    mk.setZero(pidLen,53);
    int DRow=DInv.rows();
    int wd;
    for(int jj=0;jj<53;jj++){
        for(int k=0;k<pidLen;k++){
            w(0,k)=pageIn(k,jj);
        }
        for(int i=0;i<DRow;i++){
            for(int n=0;n<DRow;n++){
                wd=m_refForm(DInv(i,n),w(0,n));
                mk(i,jj)=gf256_add(mk(i,jj),wd);
            }
        }

    }

    /*QString matchResults = "/home/david/Documents/rtklib_b34_my/HASNovAtel/data/MatrixZhang.txt";
    QFile saveFile(matchResults);//Saved file
    if (!saveFile.open(QFile::WriteOnly|QFile::Text))
    {
        OutString = "";
        return;
    }
    QTextStream saveFileOut(&saveFile);
    for(int k = 0;k<pageIn.rows();k++){
        for(int m = 0;m < pageIn.cols()-1;m++){
            saveFileOut.setFieldWidth(4);
            saveFileOut<<QString::number(pageIn(k,m));
            saveFileOut.setFieldWidth(1);
            saveFileOut<<" ";
        }
        saveFileOut.setFieldWidth(4);
        saveFileOut<<QString::number(pageIn(k,pageIn.cols()-1));
        saveFileOut.setFieldWidth(1);
        saveFileOut<<endl;
    }
    saveFileOut.flush();
    saveFile.close();
    matchResults = "/home/david/Documents/rtklib_b34_my/HASNovAtel/data/ResZhang.txt";
    QFile saveFile1(matchResults);//Saved file
    if (!saveFile1.open(QFile::WriteOnly|QFile::Text))
    {
        OutString = "";
        return;
    }
    QTextStream saveFileOut1(&saveFile1);
    for(int k = 0;k<mk.rows();k++){
        for(int m = 0;m < mk.cols()-1;m++){
            saveFileOut1.setFieldWidth(4);
            saveFileOut1<<QString::number(mk(k,m));
            saveFileOut1.setFieldWidth(1);
            saveFileOut1<<" ";
        }
        saveFileOut1.setFieldWidth(4);
        saveFileOut1<<QString::number(mk(k,mk.cols()-1));
        saveFileOut1.setFieldWidth(1);
        saveFileOut1<<endl;
    }
    saveFileOut1.flush();
    saveFile1.close();*/

    std::string Out,tempOut;
    int mkint;
    char hexmk;


    for(int jj=0;jj<DRow;jj++){
        for(int k=0;k<53;k++){
            mkint = mk(jj,k);
            memcpy(&hexmk,&mkint,sizeof(char));
            Out=Out+hex2string(&hexmk, 1);
        }
    }
//    cout<<Out<<endl;
    OutString = Out;

}
void HASPPP::testClock(int sat,double ck,gtime_t timeUpdate,gtime_t timeVai){
#ifdef TRACECLOCK
    char test_clock_char[1024];
    std::string matchResults = m_workContent+FILEPATHSEP+"debugFiles"+FILEPATHSEP+"clock.txt";
    memcpy(test_clock_char,matchResults.data(), matchResults.size()*sizeof(char)); //change 2024 2 22, 1024-> matchResults.size()
	test_clock_char[matchResults.size()] = '\0';
    std::ofstream file_ofs(test_clock_char,std::ios::app);
    if(!file_ofs.is_open()){
        cout<<"testClock file open error!"<<endl;
        return;
    }

//    QFile saveFile(matchResults);//Saved file
//    if (!saveFile.open(QFile::WriteOnly|QFile::Text|QFile::Append))
//    {
//        return;
//    }
//    QTextStream saveFileOut(&saveFile);
    char prn[4];
    satno2id(sat,prn);
    double timeBe[6];
    double timeVal[6];
    char buff1[1024],buff2[1024],buff3[1024];
    time2epoch(m_obsGTime,timeBe);//change timeUpdate -> m_obsGTime
    time2epoch(timeVai,timeVal);
    std::sprintf(buff1,"%.0f:%.0f:%.2f",timeBe[3],timeBe[4],timeBe[5]);
    std::sprintf(buff2,"%.0f:%.0f:%.2f",timeVal[3],timeVal[4],timeVal[5]);
    std::sprintf(buff3,"  clock Offser: %.4f",ck);

    file_ofs <<"prn: "<<prn<<" update Time: "<<std::string(buff1)<<" timeEnd: "<<std::string(buff2)<<std::string(buff3)<<endl;
    file_ofs.close();
//    saveFileOut<<"prn: "<<prn<<" update Time: "<<timeBe[3]<<":"<<timeBe[4]<<":"<<timeBe[5]<<" timeEnd: "<<timeVal[3]<<":"<<timeVal[4]<<":"<<timeVal[5]<<"  clock Offser: "<<ck<<endl;

    //int a=0;
    //if(n_pid!=2&&n_pid!=16){
    //    a=a+1;
    //}

//    saveFileOut.flush();
//    saveFile.close();
#endif
}
void HASPPP::comEU(std::string pageString){
#ifdef TRACEBNC
    char outfile[1024];
    std::string matchResults = m_workContent+FILEPATHSEP+"debugFiles"+FILEPATHSEP+"SSRA.bnc";
    memcpy(outfile,matchResults.data(), matchResults.size()*sizeof(char)); //change 2024 2 22, 1024-> matchResults.size()
	outfile[matchResults.size()] = '\0';
    std::ofstream file_ofs(outfile,std::ios::app);
    if(!file_ofs.is_open()){
        cout<<"comEU file open error!"<<endl;
        return;
    }
    file_ofs << pageString<<endl;
    file_ofs.close();

//    QFile saveFile(matchResults);//Saved file
//    if (!saveFile.open(QFile::WriteOnly|QFile::Text|QFile::Append))
//    {
//        return;
//    }
//    QTextStream saveFileOut(&saveFile);
//    saveFileOut<<pageString<<endl;

//    saveFileOut.flush();
//    saveFile.close();
#endif
}
void HASPPP::comIOD(std::string pageString){
    char com_iod_char[1024];
    std:;string matchResults = m_workContent+FILEPATHSEP+"debugFiles"+FILEPATHSEP+"IOD.txt";
    memcpy(com_iod_char,matchResults.data(), matchResults.size()*sizeof(char)); //change 2024 2 22, 1024->  matchResults.size()
	com_iod_char[matchResults.size()] = '\0';
    std::ofstream file_ofs(com_iod_char,std::ios::app);
    if(!file_ofs.is_open()){
        cout<<"comIOD file open error!"<<endl;
        return;
    }
    file_ofs << pageString<<endl;
    file_ofs.close();
//    QFile saveFile(matchResults);//Saved file
//    if (!saveFile.open(QFile::WriteOnly|QFile::Text|QFile::Append))
//    {
//        return;
//    }
//    QTextStream saveFileOut(&saveFile);
//    saveFileOut<<pageString<<endl;

//    saveFileOut.flush();
//    saveFile.close();
}
void HASPPP::testPid(gtime_t timeTe,int MS,int MID,int n_pid,int*pid,MatrixXi page,std::string pageString){
#ifdef TRACEPID
    /* test */
    double timeDou[6];
    char test_pid_char[1024];
    char buffer[4096];
    time2epoch(timeTe,timeDou);

    std::string matchResults = m_workContent+FILEPATHSEP+"debugFiles"+FILEPATHSEP+"pid.txt";
    memcpy(test_pid_char,matchResults.data(), matchResults.size()*sizeof(char));//change 2024 2 22,1024-> matchResults.size()
	test_pid_char[matchResults.size()] = '\0';
    std::ofstream file_ofs(test_pid_char,std::ios::app);
    if(!file_ofs.is_open()){
        cout<<"testPid file open error!"<<endl;
        return;
    }

//    QFile saveFile(matchResults);//Saved file
//    if (!saveFile.open(QFile::WriteOnly|QFile::Text|QFile::Append))
//    {
//        return;
//    }
//    QTextStream saveFileOut(&saveFile);

    std::sprintf(buffer,"Time: %.0f-%.0f-%.0f %.0f:%.0f:%.2f MID: %d  MS: %d   n_pid: %d",timeDou[0],timeDou[1],timeDou[2],timeDou[3],timeDou[4],timeDou[5],MID,MS,n_pid);

    file_ofs<<std::string(buffer);
    for(int i =0;i<n_pid;i++){
        std::sprintf(buffer," %d",pid[i]);
        file_ofs<<std::string(buffer);
    }
    file_ofs<<endl;
    file_ofs<<"page: "<<pageString<<endl;
    file_ofs.close();

    //int a=0;
    //if(n_pid!=2&&n_pid!=16){
    //    a=a+1;
    //}

//    saveFileOut.flush();
//    saveFile.close();
#endif
}
void HASPPP::testPage(MatrixXi pageIn,int *pid,int pidLen,gtime_t timeTe,int MS,int MID){
    MatrixXi finPage;
    char buffstr[1024];
    std::string pageString;
//    int pidIn[1];
//    for(int i=0;i<pidLen;i++){
//        finPage = pageIn.row(i);
//        pidIn[0] = pid[i];
//        decodeRs(finPage,pidIn,1,pageString);
//        testPid(timeTe,MS,MID,1,pidIn,finPage,pageString);
//    }
    for(int i=0;i<pidLen;i++){
        finPage = pageIn.row(i);
        std::sprintf(buffstr,"%d ",pid[i]);
        pageString=std::string(buffstr);
        for(int j=0;j<53;j++){
            std::sprintf(buffstr,"%d ",finPage(0,j));
            pageString =pageString+std::string(buffstr);
        }
        testPid(timeTe,MS,MID,1,pid+i,finPage,pageString);
    }
}
//void HASPPP::testFastSlow(){
//    QString fileTemp = "/home/david/Documents/rtklib_b34_my/SINOHAS/rtkPPPHas/debugFiles/pidA.txt";
//    QString lineTemp;
//    QFile hasInfo(fileTemp);
//    int pidLen=0;
//    int pid[12];
//    MatrixXi finPage;
//    QString pageString;
//    finPage.resize(12,53);
//    finPage.setZero(12,53);
//    QStringList navHead;
//    if(!hasInfo.open(QFile::ReadOnly)){
//        qDebug() << "open file path " <<fileTemp << "is failed";
//        return;
//    }
//    while (!hasInfo.atEnd()) {
//        lineTemp = hasInfo.readLine();
//        lineTemp.replace(QRegExp("[\\s]+"), " ");
//        navHead = lineTemp.trimmed().split(" ");
//        pid[pidLen] = navHead.at(0).toInt();
//        for(int i=0;i<53;i++){
//            finPage(pidLen,i) = navHead.at(i+1).toInt();
//        }
//        pidLen++;
//    }
//    decodeRs(finPage,pid,pidLen,pageString);
//    testPid({0},0,0,pidLen,pid,finPage,pageString);



//    hasInfo.close();
//}

void HASPPP::sortUnique(const int arr[], int n, int result[], int& resultSize) {

    std::set<int> uniqueSet(arr, arr + n);

    resultSize = 0;
    for (int elem : uniqueSet) {
        result[resultSize++] = elem;
    }
}
int HASPPP::sortpid(const galhas galhas_i,int &n_pid,int *pid,MatrixXi &finPage){

finPage.resize(galhas_i.ms ,53);
#ifdef BUBBLE
    sortUnique(galhas_i.pid, galhas_i.n_pid, pid, n_pid);
    if(n_pid!=galhas_i.ms){
        return 0;
    }
    for (int j=0;j<n_pid;j++){
        finPage.row(j) = galhas_i.page.row(pid[j]-1);
    }
#else
    for(int j = 0;j<255;j++){
        for(int ip=0;ip<32;ip++){
            if(galhas_i.page(j,ip)!=0){
                finPage.row(n_pid) = galhas_i.page.row(j);
                pid[n_pid] = j+1;
                n_pid++;
                break;
            }
        }
    }
    if(n_pid!=galhas_i.ms){
        return 0;
    }
#endif
    return 1;
}
void HASPPP::processCNAV(uint8_t *dataUChar,gtime_t gpstFrame,int cnavoffset){
    //gtime_t timeDiff[255];
    //memset(timeDiff,0,255*sizeof(gtime_t));
    //testFrameTime(gpstFrame);

    MatrixXi finPage;
    char buffstr[1024];
    std:string errorline;


    int HASS=getbitu(dataUChar, 14+cnavoffset,2);
    int MT=getbitu(dataUChar, 18+cnavoffset,2);
    int MS,PID,MID;
    MID=getbitu(dataUChar,20+cnavoffset,5);
    //qDebug()<<MID<<endl;
    MS=getbitu(dataUChar, 25+cnavoffset,5);
//    if(MS<5){
//        return;
//    }
    //qDebug()<<MS<<endl;

    PID=getbitu(dataUChar,30+cnavoffset,8);
    //qDebug()<<PID<<endl;
    std::string pageString;
    //if(HASS == 2 || MT != 1 || MS == 1){
    std::sprintf(buffstr,"MID: %d PID: %d MS: %d HASS: %d MT: %d",MID,PID,MS,HASS,MT);
    errorline = std::string(buffstr);
    traces(1,gpstFrame,errorline);
    if(HASS == 2 || MT != 1){

        return;
    }
    int a = 0;
    double testEp[6] = {2023,5,29,0,42,2};
    hassat_t *hasSat = (hassat_t*)malloc(sizeof(hassat_t));//add by zrz 2024.4.27
    gtime_t testTime = epoch2time(testEp);
    if(gpstFrame.time>=testTime.time){
        a=a+1;
    }

    if(m_typeErr.errorMid == MID &&gpstFrame.time-m_typeErr.errorTime.time<ERRORINTER&&gpstFrame.time-m_typeErr.errorTime.time>0){
        m_typeErr.errorNum++;
    }
    if(m_galhas.mid == -1 && m_galhasSpare.mid != MID){
        m_galhas.mid = MID;
        m_galhas.ms = MS + 1;
        m_galhas.n_pid = 1;
        m_galhas.pid[0] = PID;
        //qDebug()<<"m"<<m_galhas.pid[0]<<endl;
        m_galhas.page.resize(255,53);
        m_galhas.page.setZero(255,53);
        for(int pos = 0;pos<53;pos++){
            m_galhas.page(PID-1,pos) = getbitu(dataUChar,38+pos*8+cnavoffset,8);
        }
        m_galhas.updateTime[m_galhas.n_pid-1] = gpstFrame;
        if(m_galhas.n_pid == m_galhas.ms){
            m_galhas.mid = -1;
            //for(int ln = 0;ln < m_galhas.n_pid-1;ln++){
            //    timeDiff[ln].time = gpstFrame.time - m_galhas.updateTime[ln].time;
            //}
            for(int pos = 0;pos<53;pos++){
                m_galhas.page(PID-1,pos) = getbitu(dataUChar,38+pos*8+cnavoffset,8);
            }
            finPage.resize(m_galhas.ms ,53);
            int n_pid = 0;
            int pid[255];
            // change by zrz 2024.4.28
            if(sortpid(m_galhas,n_pid,pid,finPage)==0){
                free(hasSat);
                return;
            }
            decodeRs(finPage,pid,n_pid,pageString);
            testPid(gpstFrame,MS,MID,n_pid,pid,finPage,pageString);
//            testPage(finPage,pid,n_pid,gpstFrame,MS,MID);


            /*for(int j = 0;j<m_galhas.n_pid;j++){
                finPage.row(j) = m_galhas.page.row(m_galhas.pid[j]-1);
            }
            decodeRs(finPage,m_galhas.pid,m_galhas.n_pid);*/

            /* DECODE PAGE */
            if(pageString.length()==0){
                free(hasSat);
                return;
            }

            if(decodePageChange(pageString,n_pid,gpstFrame,&m_hasSat,hasSat)){
                m_hasSat = *hasSat;
                updateSSR(*hasSat,m_navs);
            }
        }

    }
    else if(m_galhas.mid == MID && m_galhas.n_pid ==  m_galhas.ms -1){
        for(int j=0;j<m_galhas.n_pid;j++){
            if(m_galhas.pid[j]==PID){
                free(hasSat);
                return;
            }
        }
        m_galhas.pid[m_galhas.n_pid] = PID;
        m_galhas.mid = -1;
        //for(int ln = 0;ln < m_galhas.n_pid-1;ln++){
        //    timeDiff[ln].time = gpstFrame.time - m_galhas.updateTime[ln].time;
        //}
        for(int pos = 0;pos<53;pos++){
            m_galhas.page(PID-1,pos) = getbitu(dataUChar,38+pos*8+cnavoffset,8);
        }
        m_galhas.n_pid++;
        finPage.resize(m_galhas.ms ,53);
        int n_pid = 0;
        int pid[255];
        // change by zrz 2024.4.28
        if(sortpid(m_galhas,n_pid,pid,finPage)==0){
            free(hasSat);
            return;
        }

        decodeRs(finPage,pid,n_pid,pageString);
        testPid(gpstFrame,MS,MID,n_pid,pid,finPage,pageString);
//        testPage(finPage,pid,n_pid,gpstFrame,MS,MID);


        /*for(int j = 0;j<m_galhas.n_pid;j++){
            finPage.row(j) = m_galhas.page.row(m_galhas.pid[j]-1);
        }
        decodeRs(finPage,m_galhas.pid,m_galhas.n_pid);*/

        /* DECODE PAGE */
        if(pageString.length()==0){
            free(hasSat);
            return;
        }

        if(decodePageChange(pageString,n_pid,gpstFrame,&m_hasSat,hasSat)){
            m_hasSat = *hasSat;
            updateSSR(*hasSat,m_navs);
        }
    }
    else if(m_galhas.mid == MID){
        for(int j=0;j<m_galhas.n_pid;j++){
            if(m_galhas.pid[j]==PID){
                free(hasSat);
                return;
            }
        }
        m_galhas.pid[m_galhas.n_pid] = PID;
        for(int pos = 0;pos<53;pos++){
            m_galhas.page(PID-1,pos) = getbitu(dataUChar,38+pos*8+cnavoffset,8);
        }
        m_galhas.n_pid++;
        m_galhas.updateTime[m_galhas.n_pid-1] = gpstFrame;
        if(m_galhas.n_pid ==m_galhas.ms){
            m_galhas.mid = -1;
            //for(int ln = 0;ln < m_galhas.n_pid-1;ln++){
            //    timeDiff[ln].time = gpstFrame.time - m_galhas.updateTime[ln].time;
            //}
            for(int pos = 0;pos<53;pos++){
                m_galhas.page(PID-1,pos) = getbitu(dataUChar,38+pos*8+cnavoffset,8);
            }
            finPage.resize(m_galhas.ms ,53);
            int n_pid = 0;
            int pid[255];
            // change by zrz 2024.4.28
            if(sortpid(m_galhas,n_pid,pid,finPage)==0){
                free(hasSat);
                return;
            }
            decodeRs(finPage,pid,n_pid,pageString);
            testPid(gpstFrame,MS,MID,n_pid,pid,finPage,pageString);
//            testPage(finPage,pid,n_pid,gpstFrame,MS,MID);


            /*for(int j = 0;j<m_galhas.n_pid;j++){
                finPage.row(j) = m_galhas.page.row(m_galhas.pid[j]-1);
            }
            decodeRs(finPage,m_galhas.pid,m_galhas.n_pid);*/

            /* DECODE PAGE */
            if(pageString.length()==0){
                free(hasSat);
                return;
            }

            if(decodePageChange(pageString,n_pid,gpstFrame,&m_hasSat,hasSat)){
                m_hasSat = *hasSat;
                updateSSR(*hasSat,m_navs);
            }
        }
    }
    else if(m_galhasSpare.mid == MID){
        galhas hasTemp;
        hasTemp = m_galhas;
        m_galhas = m_galhasSpare;
        m_galhasSpare = hasTemp;
        if(m_galhas.n_pid ==  m_galhas.ms -1){
            for(int j=0;j<m_galhas.n_pid;j++){
                if(m_galhas.pid[j]==PID){
                    free(hasSat);
                    return;
                }
            }
            m_galhas.pid[m_galhas.n_pid] = PID;
            m_galhas.mid = -1;
            for(int pos = 0;pos<53;pos++){
                m_galhas.page(PID-1,pos) = getbitu(dataUChar,38+pos*8+cnavoffset,8);
            }
            m_galhas.n_pid++;
            finPage.resize(m_galhas.ms ,53);
            int n_pid = 0;
            int pid[255];
            // change by zrz 2024.4.28
            if(sortpid(m_galhas,n_pid,pid,finPage)==0){
                free(hasSat);
                return;
            }
            decodeRs(finPage,pid,n_pid,pageString);
            if(n_pid!=2&&n_pid!=16){
                 a=a+1;
            }
            testPid(gpstFrame,MS,MID,n_pid,pid,finPage,pageString);
//            testPage(finPage,pid,n_pid,gpstFrame,MS,MID);

            /*for(int j = 0;j<m_galhas.n_pid;j++){
                finPage.row(j) = m_galhas.page.row(m_galhas.pid[j]-1);
            }
            decodeRs(finPage,m_galhas.pid,m_galhas.n_pid);*/

            /* DECODE PAGE */
            if(pageString.length()==0){
                free(hasSat);
                return;
            }

            if(decodePageChange(pageString,n_pid,gpstFrame,&m_hasSat,hasSat)){
                m_hasSat = *hasSat;
                updateSSR(*hasSat,m_navs);
            }
        }
        else{
            for(int j=0;j<m_galhas.n_pid;j++){
                if(m_galhas.pid[j]==PID){
                    free(hasSat);
                    return;
                }
            }
            m_galhas.pid[m_galhas.n_pid] = PID;
            for(int pos = 0;pos<53;pos++){
                m_galhas.page(PID-1,pos) = getbitu(dataUChar,38+pos*8+cnavoffset,8);
            }
            m_galhas.n_pid++;
            m_galhas.updateTime[m_galhas.n_pid-1] = gpstFrame;
        }
    }
    else if( m_galhasSpare.mid == -1){
        m_galhasSpare.mid = MID;
        m_galhasSpare.ms = MS + 1;
        m_galhasSpare.n_pid = 1;
        m_galhasSpare.pid[0] = PID;
        m_galhasSpare.page.resize(255,53);
        m_galhasSpare.page.setZero(255,53);
        for(int pos = 0;pos<53;pos++){
            m_galhasSpare.page(PID-1,pos) = getbitu(dataUChar,38+pos*8+cnavoffset,8);
        }
        m_galhasSpare.updateTime[m_galhasSpare.n_pid-1] = gpstFrame;

        if(m_galhasSpare.n_pid == m_galhasSpare.ms){
            m_galhasSpare.mid =-1;
            finPage.resize(m_galhasSpare.ms ,53);
            int n_pid = 0;
            int pid[255];
            // change by zrz 2024.4.28
            if(sortpid(m_galhasSpare,n_pid,pid,finPage)==0){
                free(hasSat);
                return;
            }

            decodeRs(finPage,pid,n_pid,pageString);
            if(n_pid!=2&&n_pid!=16){
                 a=a+1;
            }
            testPid(gpstFrame,MS,MID,n_pid,pid,finPage,pageString);
//            testPage(finPage,pid,n_pid,gpstFrame,MS,MID);

            /*for(int j = 0;j<m_galhas.n_pid;j++){
                finPage.row(j) = m_galhas.page.row(m_galhas.pid[j]-1);
            }
            decodeRs(finPage,m_galhas.pid,m_galhas.n_pid);*/

            /* DECODE PAGE */
            if(pageString.length()==0){
                free(hasSat);
                return;
            }

            if(decodePageChange(pageString,n_pid,gpstFrame,&m_hasSat,hasSat)){
                m_hasSat = *hasSat;
                updateSSR(*hasSat,m_navs);
            }
        }

    }
    //change by zhangrunzhi 120 -> 5
    else if(m_galhasSpare.mid != -1){
        if(gpstFrame.time - m_galhasSpare.updateTime[m_galhasSpare.n_pid-1].time >= 5){
            initHas(m_galhasSpare);
        }
        else {
            m_typeErr.errorMid =  m_galhasSpare.mid;
            m_typeErr.errorTime = m_galhasSpare.updateTime[m_galhasSpare.n_pid-1];
            cout<<"def "<<MS<<MID<<endl;
        }
        m_galhasSpare.mid = MID;
        m_galhasSpare.ms = MS + 1;
        m_galhasSpare.n_pid = 1;
        m_galhasSpare.pid[0] = PID;
        m_galhasSpare.page.resize(255,53);
        m_galhasSpare.page.setZero(255,53);
        for(int pos = 0;pos<53;pos++){
            m_galhasSpare.page(PID-1,pos) = getbitu(dataUChar,38+pos*8+cnavoffset,8);
        }
        m_galhasSpare.updateTime[m_galhasSpare.n_pid-1] = gpstFrame;
        //change 2023 5 27
        if(m_galhasSpare.n_pid == m_galhasSpare.ms){
            m_galhasSpare.mid =-1;
            finPage.resize(m_galhasSpare.ms ,53);
            int n_pid = 0;
            int pid[255];
            if(sortpid(m_galhasSpare,n_pid,pid,finPage)==0){
                free(hasSat);
                return;
            }
            decodeRs(finPage,pid,n_pid,pageString);
            if(n_pid!=2&&n_pid!=16){
                 a=a+1;
            }
            testPid(gpstFrame,MS,MID,n_pid,pid,finPage,pageString);
//            testPage(finPage,pid,n_pid,gpstFrame,MS,MID);

            /*for(int j = 0;j<m_galhas.n_pid;j++){
                finPage.row(j) = m_galhas.page.row(m_galhas.pid[j]-1);
            }
            decodeRs(finPage,m_galhas.pid,m_galhas.n_pid);*/

            /* DECODE PAGE */
            if(pageString.length()==0){
                free(hasSat);
                return;
            }

            if(decodePageChange(pageString,n_pid,gpstFrame,&m_hasSat,hasSat)){
                m_hasSat = *hasSat;
                updateSSR(*hasSat,m_navs);
            }
        }

    }
    else{
        ErrorMsgPos(errorline);
        traces(1,gpstFrame,errorline);
    }
    free(hasSat);
}
void HASPPP::testFrameTime(gtime_t timeFrame){
    double timeDou[6];
    char buffer[1024];
    time2epoch(timeFrame,timeDou);
    char test_frame_time_char[1024];
    std::string matchResults = m_workContent+FILEPATHSEP+"debugFiles"+FILEPATHSEP+"FrameTime.txt";
    memcpy(test_frame_time_char,matchResults.data(), matchResults.size()*sizeof(char));//change 2024 2 22, 1024-> matchResults.size()
	test_frame_time_char[matchResults.size()] = '\0';
    std::ofstream file_ofs(test_frame_time_char,std::ios::app);
    if(!file_ofs.is_open()){
        cout<<"testFrameTime file open error!"<<endl;
        return;
    }

//    QFile saveFile(matchResults);//Saved file
//    if (!saveFile.open(QFile::WriteOnly|QFile::Text|QFile::Append))
//    {
//        return;
//    }
//    QTextStream saveFileOut(&saveFile);

    std::sprintf(buffer,"Time: %.0f-%.0f-%.0f %.0f:%.0f:%.2f",timeDou[0],timeDou[1],timeDou[2],timeDou[3],timeDou[4],timeDou[5]);

    file_ofs<<"Time: "<<std::string(buffer)<<endl;
    file_ofs.close();
    //int a=0;
    //if(n_pid!=2&&n_pid!=16){
    //    a=a+1;
    //}

//    saveFileOut.flush();
//    saveFile.close();
}
std::string HASPPP::hex2string(char* datahex, int len){
    char tempchar[4096];
    std::string Out;
    int  jj ;

    for(jj=0;jj<len;jj++){
        std::sprintf(tempchar+jj*2,"%02x",(uint8_t)datahex[jj]);
    }
    tempchar[jj*2]='\0';
    Out = std::string(tempchar);
    return Out;
}

/* set unsigned/signed bits ----------------------------------------------------
* set unsigned/signed bits to byte data
* args   : unsigned char *buff IO byte data
*          int    pos    I      bit position from start of data (bits)
*          int    len    I      bit length (bits) (len<=32)
*         (unsigned) int I      unsigned/signed data
* return : none
*-----------------------------------------------------------------------------*/
void HASPPP:: setbitu(unsigned char *buff, int pos, int len, unsigned int data)
{
    unsigned int mask=1u<<(len-1);
    int i;
    if (len<=0||32<len) return;
    for (i=pos;i<pos+len;i++,mask>>=1) {
        if (data&mask) buff[i/8]|=1u<<(7-i%8); else buff[i/8]&=~(1u<<(7-i%8));
    }
}
int HASPPP::decodeSeptentrioHead(uint8_t navCharSina[64],gtime_t *gpstFrame,int &readOffset){
    char byteTemp[1024];
    std::string readStringTemp, readString;
    int index;
    int headLen =0,receMsgType =0,msgLen = 0,gpstMs=0,gpstWeek=0;
    uint8_t *p=NULL;
    int temp;

    if (m_hasBin.hasReadByteReady==482538 && readOffset>13000){
        temp=msgLen;
    }
    uint8_t dataUChar[80];
    uint8_t headUChar[NVTHEADBYTE];
    if(m_hasBin.hasReadByteReady + SEPTENTRIO_HEADLEN +readOffset >=m_hasBin.hasSsrByte){
        m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
        return 0;
    }
    memcpy(byteTemp, m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, SEPTENTRIO_HEADLEN*sizeof(char));
    byteTemp[SEPTENTRIO_HEADLEN] = '\0';
    readOffset = readOffset + SEPTENTRIO_HEADLEN ;
    readStringTemp = hex2string(byteTemp, SEPTENTRIO_HEADLEN);
    while (readStringTemp.find("2440") == readStringTemp.npos){
        if(m_hasBin.hasReadByteReady + SEPTENTRIO_HEADLEN +readOffset>=m_hasBin.hasSsrByte){
            m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
            return 0;
        }
        memcpy(byteTemp, m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, SEPTENTRIO_HEADLEN*sizeof(char));
        byteTemp[SEPTENTRIO_HEADLEN] = '\0';
        readOffset = readOffset + SEPTENTRIO_HEADLEN ;
        readStringTemp = hex2string(byteTemp, SEPTENTRIO_HEADLEN);

    }
    index = readStringTemp.find("2440")/2;
    readString = readStringTemp;
    if(index != 0){
        if(m_hasBin.hasReadByteReady + index +readOffset>=m_hasBin.hasSsrByte){
            m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
            return 0;
        }
        memcpy(byteTemp, m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, index*sizeof(char));
        byteTemp[index] = '\0';
        readOffset = readOffset + index ;
        readString = readStringTemp.substr(index*2,readStringTemp.size()-1)+hex2string(byteTemp, index);
    }
    memcpy(headUChar,m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset-SEPTENTRIO_HEADLEN,SEPTENTRIO_HEADLEN*sizeof(char));
    readStringTemp = readString.substr(10,2)+readString.substr(8,2);
    receMsgType = stoi(readStringTemp,0,16);
    readStringTemp = readString.substr(14,2)+readString.substr(12,2);
    msgLen = stoi(readStringTemp,0,16);

    gpstMs = U4(headUChar+8);
    gpstWeek = U2(headUChar+12);
    *gpstFrame = gpst2time(gpstWeek,gpstMs*0.001);

    if(m_hasBin.hasReadByteReady + msgLen +readOffset>=m_hasBin.hasSsrByte){
        m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
        return 0;
    }
    if(receMsgType!=GALCNAVSEP){
        return 1;
    }

    memcpy(byteTemp, m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, msgLen*sizeof(char));
    byteTemp[msgLen] = '\0';
    readOffset = readOffset + msgLen;
    memcpy(dataUChar,byteTemp+CNAVOFSSEP,(msgLen-CNAVOFSSEP)*sizeof(char));
    p = dataUChar;
    for (int i=0;i<16;i++,p+=4) {
        setbitu(navCharSina,32*i,32,U4(p));
    }
    return 2;


}
int HASPPP::decodeNovAtelHead(uint8_t navCharSina[64],gtime_t *gpstFrame,int &readOffset){
    char byteTemp[1024];
    std::string readStringTemp, readString;
    int index;
    int headLen =0,receMsgType =0,msgLen = 0,gpstMs=0,gpstWeek=0;

    uint8_t dataUChar[80];
    uint8_t headUChar[NVTHEADBYTE];
    if(m_hasBin.hasReadByteReady + NVTHEADBYTE +readOffset >=m_hasBin.hasSsrByte){
        m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
        return 0;
    }
    memcpy(byteTemp, m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, NVTHEADBYTE*sizeof(char));
    byteTemp[NVTHEADBYTE] = '\0';
    readOffset = readOffset + NVTHEADBYTE ;
    readStringTemp = hex2string(byteTemp, NVTHEADBYTE);
    while (readStringTemp.find("aa4412") == readStringTemp.npos){
        if(m_hasBin.hasReadByteReady + NVTHEADBYTE +readOffset>=m_hasBin.hasSsrByte){
            m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
            return 0;
        }
        memcpy(byteTemp, m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, NVTHEADBYTE*sizeof(char));
        byteTemp[NVTHEADBYTE] = '\0';
        readOffset = readOffset + NVTHEADBYTE ;
        readStringTemp = hex2string(byteTemp, NVTHEADBYTE);

    }
    index = readStringTemp.find("aa4412")/2;
    readString = readStringTemp;
    if(index != 0){
        if(m_hasBin.hasReadByteReady + index +readOffset>=m_hasBin.hasSsrByte){
            m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
            return 0;
        }
        memcpy(byteTemp, m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, index*sizeof(char));
        byteTemp[index] = '\0';
        readOffset = readOffset + index ;
        readString = readStringTemp.substr(index*2,readStringTemp.size()-1)+hex2string(byteTemp, index);
    }
    readStringTemp = readString.substr(6,2);
    headLen = stoi(readStringTemp,0,16);
    if(headLen != NVTHEADBYTE){
        cout << "headLen = "<<headLen<<"headLen erro"<<endl;
        return 1;
    }

    memcpy(headUChar,m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset-NVTHEADBYTE,NVTHEADBYTE*sizeof(char));
    readStringTemp = readString.substr(10,2)+readString.substr(8,2);
    receMsgType = stoi(readStringTemp,0,16);
    readStringTemp = readString.substr(18,2)+readString.substr(16,2);
    msgLen = stoi(readStringTemp,0,16);

    gpstMs = U4(headUChar+16);
    gpstWeek = U2(headUChar+14);
    *gpstFrame = gpst2time(gpstWeek,gpstMs*0.001);

    if(m_hasBin.hasReadByteReady + msgLen +readOffset>=m_hasBin.hasSsrByte){
        m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
        return 0;
    }
    if(receMsgType!=GALCNAVNOVATEL){
        return 1;
    }
    memcpy(byteTemp, m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, msgLen*sizeof(char));
    byteTemp[msgLen] = '\0';
    readOffset = readOffset + msgLen;



    memcpy(dataUChar,byteTemp+CNAVOFSNOVATEL,(msgLen-CNAVOFSNOVATEL)*sizeof(char));
    memcpy(navCharSina,dataUChar,58*sizeof(char));
    return 2;
}
int HASPPP::decodeSINOHead(uint8_t navCharSina[64],gtime_t *gpstFrame,int &readOffset){
    char byteTemp[1024];
    std::string readStringTemp, readString;
    int index;
    int headLen =0,receMsgType =0,msgLen = 0,gpstMs=0,gpstWeek=0;

    uint8_t dataUChar[64];
    uint8_t headUChar[NVTHEADBYTE];
    if(m_hasBin.hasReadByteReady + NVTHEADBYTE +readOffset >=m_hasBin.hasSsrByte){
        m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
        return 0;
    }
//        byteTemp.resize(NVTHEADBYTE*sizeof(char));
//        memcpy(byteTemp.data(), m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, NVTHEADBYTE*sizeof(char));
    memcpy(byteTemp, m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, NVTHEADBYTE*sizeof(char));
    byteTemp[NVTHEADBYTE] = '\0';
    readOffset = readOffset + NVTHEADBYTE ;
    readStringTemp = hex2string(byteTemp, NVTHEADBYTE);
//        readStringTemp = byteTemp.toHex();
    while (readStringTemp.find("aa4412") == readStringTemp.npos){
        if(m_hasBin.hasReadByteReady + NVTHEADBYTE +readOffset>=m_hasBin.hasSsrByte){
            m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
            return 0;
        }
        memcpy(byteTemp, m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, NVTHEADBYTE*sizeof(char));
        byteTemp[NVTHEADBYTE] = '\0';
        readOffset = readOffset + NVTHEADBYTE ;
        readStringTemp = hex2string(byteTemp, NVTHEADBYTE);

    }
//        while(readStringTemp.indexOf("aa4412",Qt::CaseInsensitive) == -1){
//            if(m_hasBin.hasReadByteReady + NVTHEADBYTE +readOffset>=m_hasBin.hasSsrByte){
//                m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
//                return;
//            }
//            byteTemp.resize(NVTHEADBYTE*sizeof(char));
//            memcpy(byteTemp.data(), m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, NVTHEADBYTE*sizeof(char));
//            readOffset= readOffset + NVTHEADBYTE;
//            readStringTemp = byteTemp.toHex();
//        }
    index = readStringTemp.find("aa4412")/2;
    readString = readStringTemp; //if index ==  0,readString = readStringTemp
    if(index != 0){
        if(m_hasBin.hasReadByteReady + index +readOffset>=m_hasBin.hasSsrByte){
            m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
            return 0;
        }
        memcpy(byteTemp, m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, index*sizeof(char));
        byteTemp[index] = '\0';
        readOffset = readOffset + index ;
        readString = readStringTemp.substr(index*2,readStringTemp.size()-1)+hex2string(byteTemp, index);

//            byteTemp.resize(index*sizeof(char));
//            memcpy(byteTemp.data(), m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, index*sizeof(char));
//            readOffset= readOffset + index;
//            readString = readStringTemp.mid(2*index) + byteTemp.toHex();
    }
    readStringTemp = readString.substr(6,2);
    headLen = stoi(readStringTemp,0,16);
    if(headLen != NVTHEADBYTE){
        cout << "headLen = "<<headLen<<"headLen erro"<<endl;
        return 1;
    }

    memcpy(headUChar,m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset-NVTHEADBYTE,NVTHEADBYTE*sizeof(char));
    readStringTemp = readString.substr(10,2)+readString.substr(8,2);
    receMsgType = stoi(readStringTemp,0,16);
    readStringTemp = readString.substr(18,2)+readString.substr(16,2);
    msgLen = stoi(readStringTemp,0,16);

    //readStringTemp = readString.mid(38,2)+readString.mid(36,2)+readString.mid(34,2)+readString.mid(32,2);
   // gpstMs = readStringTemp.toInt(&ok,16);
    gpstMs = U4(headUChar+16);
    //readStringTemp = readString.mid(30,2)+readString.mid(28,2);
    //gpstWeek = readStringTemp.toInt(&ok,16);
    gpstWeek = U2(headUChar+14);
    *gpstFrame = gpst2time(gpstWeek,gpstMs*0.001);

    if(m_hasBin.hasReadByteReady + msgLen +readOffset>=m_hasBin.hasSsrByte){
        m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
        return 0;
    }
    if(receMsgType!=GALCNAVSINO){
        return 1;
    }
//        byteTemp.resize(msgLen*sizeof(char));
//        memcpy(byteTemp.data(), m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, msgLen*sizeof(char));
    memcpy(byteTemp, m_hasBin.binBuff+m_hasBin.hasReadByteReady+readOffset, msgLen*sizeof(char));
    byteTemp[msgLen] = '\0';
    readOffset = readOffset + msgLen;
//        qDebug()<<"message ID: "<<receMsgType<<endl;

//        satellite = U4(dataUChar+GALCNAVSate);
    memcpy(dataUChar,byteTemp,64*sizeof(char));
//        QString tr = QString("E")+QString::number(satellite)+QString(", Receiver receive message!");
//        traces(2,gpstFrame,tr);

    //change by zrz  2024.4.28
//    uint lsbMsb = 0;
//    for(int o = 0;o<64;o++){
//        if(o == 0){
//            lsbMsb = getbitu(dataUChar+o,2,6);
//            continue;
//        }
//        navCharSina[o-1] = (uint8_t)((lsbMsb<<2) + getbitu(dataUChar+o,0,2));
//        lsbMsb = getbitu(dataUChar+o,2,6);
//    }
    memcpy(navCharSina,dataUChar,64*sizeof(char));
    return 2;
}
void HASPPP::updateHasCorrection(double *obsTime){
	/* Decoding function, obsTime is the time of observation*/
//    testFastSlow();
    if(m_hasBin.hasReadByteReady>= m_hasBin.hasSsrByte){
        return ;
    }
    char buffer[4096];
    //double obs[6];
    //obs[0] = 2023;
    //obs[1] = 11;
    //obs[2] = 8;
    //obs[3] = 22;
    //obs[4] = 28;
    //obs[5] = 40.00;
    //obsTime = &obs[0];
    gtime_t obsGtime = epoch2time(obsTime);
    /* debug */
    //QString matchResults = "/home/david/Documents/rtklib_b34_my/Gao/rtkPPPHas/data/pidQString.txt";
    //QFile saveFile(matchResults);//Saved file
    //if (!saveFile.open(QFile::WriteOnly|QFile::Text))
    //{
    //     return;
    //}
    //saveFile.close();


//    QByteArray byteTemp;
//    QString readStringTemp,readString;

    int readOffset = 0,cnavoffset;
    gtime_t gpstFrame;
    uint8_t navCharSina[64];
    int satellite;
    int res;


    if(obsTime[3]==6&&obsTime[4]==23&&obsTime[5]==30){
        satellite=1;
    }
    //change by zhangrunzhi 2023 .2.2
    m_obsGTime = obsGtime;

    std::sprintf(buffer,"obsTime: %.0f-%.0f-%.0f %.0f:%.0f:%.2f",obsTime[0],obsTime[1],obsTime[2],obsTime[3],obsTime[4],obsTime[5]);
    std::string obsString = std::string(buffer);
    comEU(obsString);
    while(timediff(m_updatedGPSTime,obsGtime)< -DTTOL){
        switch (m_hasFormat){
            case HAS_SINO:
                res = decodeSINOHead(navCharSina,&gpstFrame,readOffset);
                if (res==0){
                    return ;
                }
                else if(res==1){
                    continue;
                }
                else if(res==2){
                    cnavoffset = CNAVOFSSINO;
                }
                else {
                    cout<<"SINO receiver head error!"<<endl;
                    continue;
                }
                break;
            case HAS_NOVATEL:
                res = decodeNovAtelHead(navCharSina,&gpstFrame,readOffset);
                if (res==0){
                    return ;
                }
                else if(res==1){
                    continue;
                }
                else if(res==2){
                    cnavoffset = 0;
                }
                else {
                    cout<<"NovAtel receiver head error!"<<endl;
                    continue;
                }
                break;
            case HAS_SEPTENTRIO:
                res = decodeSeptentrioHead(navCharSina,&gpstFrame,readOffset);
                if (res==0){
                    return ;
                }
                else if(res==1){
                    continue;
                }
                else if(res==2){
                    cnavoffset = 0;
                }
                else {
                    cout<<"Septentrio receiver head error!"<<endl;
                    continue;
                }
                break;
            default:
                cout<< "no supporting HAS receiver!"<<endl;
                break;
        }

        processCNAV(navCharSina,gpstFrame,cnavoffset);

        m_updatedGPSTime = gpstFrame;

    }
    m_hasBin.hasReadByteReady  = m_hasBin.hasReadByteReady  + readOffset;
}
/*  only used in second-level  */
gtime_t HASPPP::gpst2gstM(gtime_t gpst){
    return timeadd(gpst,0.0);
}
gtime_t HASPPP::gst2gpstM(gtime_t gst){
    return timeadd(gst,-0.0);
}
void HASPPP::updateSSR(hassat_t hasSat,nav_t &nav){
    int sat;
    for(int i = 0;i<hasSat.nsat;i++){
        sat = hasSat.sat[i].sat;
        nav.ssr[sat- 1] = hasSat.sat[i].ssr;
    }

}
ssr_t *HASPPP:: getSsr(){
    return m_navs.ssr;
}
bool HASPPP::decodePageChange(std::string page,int pidLen,gtime_t gpstTime,const hassat_t *hasSat_i,hassat_t *hasSat_o){
	/* As of junuary 1,2024, the two main types of combinations were obtained for \
	 * complete HAS product:IOD:110010 (orbit and code bias) and 001000 (clock offset)*/
    int charLen=pidLen*53;
    char buffstr[1024];
    //charLen=pidLen*53;
    gtime_t gstTimeLocal,gstTime;
    uint8_t* dataUChar = new uint8_t[charLen];//uint8_t dataUChar[charLen];
	//uint8_t dataUChar[10000];
    double dateEPH[6];
    bool ok;
    gstTimeLocal = gpst2gstM(gpstTime);
    std::string errorLine;
    int Hr = (int)((int)gstTimeLocal.time/3600);
    for(int i = 0;i<charLen;i++){
//        dataUChar[i] = (uint8_t)(page.mid(i*2,2).toInt(&ok,16));
        dataUChar[i] = (uint8_t)stoi(page.substr(i*2,2),0,16);
    }
    uint8_t* buffer = new uint8_t[charLen];
	//uint8_t buffer[10000];
    int iod[6];
    int toh;
    int maskId;
    int iodId;
    bool okDecode;
    int index;
    //add by zhangrunzhi 2022 12.07
    hassat_t hasSat = *hasSat_i;
    //initHassat_t(hasSat);
    initHassat_t(*hasSat_o);
        memcpy(buffer,dataUChar,charLen*sizeof(char));
        decodePageHead(buffer,iod);
        toh = getbitu(buffer, 0,12);
        int HrT = Hr*3600 + toh;
        if(HrT <= gstTimeLocal.time + gstTimeLocal.sec){
            gstTime.sec = 0;
            gstTime.time = HrT;
        }
        else{
            gstTime.sec = 0;
            gstTime.time = HrT -3600;
        }
        //hasSat.iodTime[0] = gpst2gstM(gstTime);
        hasSat.iodTime[0] = gstTime;
        hasSat.iodTime[5] = gpstTime;
        maskId = getbitu(buffer,22,5);
        iodId = getbitu(buffer,27,5);
        uint8_t *p = buffer + 4;
        index = 0;

        //memcpy(hasSat.iod,iod,6*sizeof(int));
        //add by zhangrunzhi 2022.12.4
        //change 2022 12 07

        //change 2023 01 31 iod 110010
        //if(iod[0]&&iod[1]&&!iod[2]&&!iod[3]&&iod[4]&&iod[5]){
        if(iod[0]&&iod[1]&&!iod[2]&&!iod[3]&&iod[4]){
            hasSat.iod[0] = iodId;
        }
        else if(!iod[0]&&!iod[1]&&iod[2]&&!iod[3]&&!iod[4]&&!iod[5]){
            hasSat.iod[1] = iodId;
        }
        //add by ZRZ 2023 12 26
        else if (iod[0]&&iod[1]&&!iod[2]&&!iod[3]&&iod[4]&&iod[5]){
            time2epoch(gpstTime,dateEPH);
            std::sprintf(buffstr,"%.0f-%.0f-%.0f %.0f:%.0f%.2f iod: %d%d%d%d%d%d",dateEPH[0],dateEPH[1],dateEPH[2],dateEPH[3],dateEPH[4],dateEPH[5],iod[0],iod[1],iod[2],iod[3],iod[4],iod[5]);
            errorLine = std::string(buffstr);

            comIOD(errorLine);
        }
        else{
            time2epoch(gpstTime,dateEPH);
            std::sprintf(buffstr,"%.0f-%.0f-%.0f %.0f:%.0f%.2f iod: %d%d%d%d%d%d",dateEPH[0],dateEPH[1],dateEPH[2],dateEPH[3],dateEPH[4],dateEPH[5],iod[0],iod[1],iod[2],iod[3],iod[4],iod[5]);
            errorLine = std::string(buffstr);

            comIOD(errorLine);
			delete []buffer;
			delete []dataUChar;
            return false;
        }
        if(iod[0]){
            okDecode = decodeMask(p,hasSat,&index);
            if(!okDecode){
                std::sprintf(buffstr,"%d",gpstTime.time);
                errorLine = std::string(buffstr)+std::string("  Decode Mask failed");
                traces(1,gpstTime,errorLine);
				delete[]buffer;
				delete[]dataUChar;
                return false;
            }
            hasSat.maskId = maskId;
        }
        else{
            if(maskId == hasSat.maskId){

            }
            else{
                std::sprintf(buffstr,"MaskId changed,maskId = %d,  hasSat_i.maskId = %d",maskId,hasSat_i->maskId);
                errorLine = std::string(buffstr);
                traces(1,gpstTime,errorLine);
				delete[]buffer;
				delete[]dataUChar;
                return false;
            }
        }
        //add by zhangrunzhi 2022.12.4
        //if(iod[0]&&iod[1]&&!iod[2]&&!iod[3]&&!iod[4]&&!iod[5]){
            //qDebug()<<"success iod:"<<iod[0]<<iod[1]<<iod[2]<<iod[3]<<iod[4]<<iod[5]<<endl;
        //}
        //if(!iod[0]&&!iod[1]&&!iod[2]&&iod[3]&&!iod[4]&&!iod[5]){
            //qDebug()<<"success iod:"<<iod[0]<<iod[1]<<iod[2]<<iod[3]<<iod[4]<<iod[5]<<endl;
        //}
        if(index > charLen*8){
            ErrorMsgPos(errorLine);
            std::sprintf(buffstr,"index: %d   decodeLen: %d  iod:  %d%d%d%d%d%d",index,charLen*8,iod[0],iod[1],iod[2],iod[3],iod[4],iod[5]);
            errorLine = std::string(buffstr);
            traces(1,gpstTime,errorLine);
			delete[]buffer;
			delete[]dataUChar;
            return false;
        }
        if(iod[1]){
            okDecode = decodeOrbit(p,hasSat,&index);
            if(!okDecode){
                errorLine = std::string("Decode Orbit failed");
                traces(1,gpstTime,errorLine);
				delete[]buffer;
				delete[]dataUChar;
                return false;
            }
        }
        else{
            if(maskId == hasSat.maskId && iodId == hasSat.iodSet){
                //hasSat = hasSat_i;
            }
            //add by zhangrunzhi 2022 12.4
            else{
                //qDebug()<<"maskId:"<<maskId<<"  maskIdLast:"<<hasSat_i.maskId<<"iodSet:"<<iodId<<"  iodSetLast:"<<hasSat_i.iodSet<<endl;
                ErrorMsgPos(errorLine);
                std::sprintf(buffstr,"iodSetOld: %d iodSetNew: %d maskIdOld: %d maskIdNew: %d iod:  %d%d%d%d%d%d",hasSat.iodSet,iodId,hasSat.maskId,maskId,iod[0],iod[1],iod[2],iod[3],iod[4],iod[5]);
                errorLine = std::string(buffstr);
                traces(1,gpstTime,errorLine);
				delete[]buffer;
				delete[]dataUChar;
                return false;
            }
        }
        if(index > charLen*8){
            ErrorMsgPos(errorLine);
            std::sprintf(buffstr,"index: %d,   decodeLen: %d iod:  %d%d%d%d%d%d",index,charLen*8,iod[0],iod[1],iod[2],iod[3],iod[4],iod[5]);
            errorLine = std::string(buffstr);
            traces(1,gpstTime,errorLine);
			delete[]buffer;
			delete[]dataUChar;
            return false;
        }
        if(iod[0] == 0 && iod[2] && maskId == hasSat_i->maskId){
            okDecode = decodeCF(p,hasSat,&index);
            if(!okDecode){
                errorLine = std::string("Decode Clock Full failed");
                traces(1,gpstTime,errorLine);
				delete[]buffer;
				delete[]dataUChar;
                return false;
            }
        }
        //else if(iod[0] == 0 && iod[2] && (maskId != hasSat_i.maskId)){
        //    errorLine = QString("CF failed, iod:  ")+QString::number(iod[0])+QString::number(iod[1])+QString::number(iod[2])+QString::number(iod[3])+QString::number(iod[4])+QString::number(iod[5]);
        //    traces(1,errorLine);
        //    return false;
        //}
        //else if(iod[0] == 0&& iod[2] == 1){
        //    okDecode = decodeCF(p,hasSat,&index);
         //   if(!okDecode){
         //       errorLine = QString("Decode Clock Full failed");
         //       traces(1,errorLine);
         //       return false;
         //   }
       // }
        //add by zhangrunzhi 2022 12 07
        //else if(iod[0] && !iod[2] && maskId == hasSat_i.maskId && iodId == hasSat_i.iodSet){
         //   for(int j = 0;j<hasSat.nsat;j++){
         //       hasSat.sat[j].ssr.dclk[0] = hasSat_i.sat[j].ssr.dclk[0];
         //       //qDebug()<<dcc * 0.0025<<prn<<endl;
         //       hasSat.sat[j].ssr.t0[1] = hasSat_i.sat[j].ssr.t0[1];
           // }
        //}
        if(index > charLen*8){
            ErrorMsgPos(errorLine);
            std::sprintf(buffstr,"index: %d,   decodeLen: %d iod:  %d%d%d%d%d%d",index,charLen*8,iod[0],iod[1],iod[2],iod[3],iod[4],iod[5]);
            errorLine = std::string(buffstr);
            traces(1,gpstTime,errorLine);
			delete[]buffer;
			delete[]dataUChar;
            return false;
        }
        if(iod[3]){
            okDecode = decodeCS(p,hasSat,&index);
            if(!okDecode){
                errorLine = std::string("Decode Clock Subset failed");
                traces(1,gpstTime,errorLine);
				delete[]buffer;
				delete[]dataUChar;
                return false;
            }
        }
        if(index > charLen*8){
            ErrorMsgPos(errorLine);
            std::sprintf(buffstr,"index: %d,   decodeLen: %d iod:  %d%d%d%d%d%d",index,charLen*8,iod[0],iod[1],iod[2],iod[3],iod[4],iod[5]);
            errorLine = std::string(buffstr);
            traces(1,gpstTime,errorLine);
			delete[]buffer;
			delete[]dataUChar;
            return false;
        }
        if(iod[4]){
            okDecode = decodeCodeBias(p,hasSat,&index);
            if(!okDecode){
                errorLine = std::string("Decode Code Bias failed");
                traces(1,gpstTime,errorLine);
				delete[]buffer;
				delete[]dataUChar;
                return false;
            }
        }
        if(index > charLen*8){
            ErrorMsgPos(errorLine);
            std::sprintf(buffstr,"index: %d,   decodeLen: %d iod:  %d%d%d%d%d%d",index,charLen*8,iod[0],iod[1],iod[2],iod[3],iod[4],iod[5]);
            errorLine = std::string(buffstr);
            traces(1,gpstTime,errorLine);
			delete[]buffer;
			delete[]dataUChar;
            return false;
        }
        if(iod[5]){
            okDecode = decodePhaseBias(p,hasSat,&index);
            if(!okDecode){
                errorLine = std::string("Decode Phase Bias failed");
                traces(1,gpstTime,errorLine);
				delete[]buffer;
				delete[]dataUChar;
                return false;
            }
        }
        if(index > charLen*8){
            ErrorMsgPos(errorLine);
            std::sprintf(buffstr,"index: %d,   decodeLen: %d iod:  %d%d%d%d%d%d",index,charLen*8,iod[0],iod[1],iod[2],iod[3],iod[4],iod[5]);
            errorLine = std::string(buffstr);
            traces(1,gpstTime,errorLine);
			delete[]buffer;
			delete[]dataUChar;
            return false;
        }
        bool decodeBool;
        decodeBool = adjustPos(p,index,(charLen - 4)*8);
        if(decodeBool && iod[2]==1){
            decodeBool =true;
        }
        else if(iod[2]==1 && !decodeBool){
            decodeBool =false;
        }
        if(decodeBool){
            *hasSat_o = hasSat;
            hasSat_o->iodSet = iodId;
        }
        else{
            std::sprintf(buffstr,"Decode length Error,iod:  %d%d%d%d%d%d",iod[0],iod[1],iod[2],iod[3],iod[4],iod[5]);
            errorLine = std::string(buffstr);
            traces(1,gpstTime,errorLine);
			delete[]buffer;
			delete[]dataUChar;
            return false;
        }
		delete[]buffer;
		delete[]dataUChar;
    return true;
    //}


}
/*bool HASPPP::decodePage(QString page,int pidLen,gtime_t gpstTime,hassat_t hasSat_i,hassat_t &hasSat_o){
    int charLen=pidLen*53;
    //charLen=pidLen*53;
    gtime_t gstTimeLocal,gstTime;
    uint8_t dataUChar[charLen];
    bool ok;
    gstTimeLocal = gpst2gstM(gpstTime);
    QString errorLine;
    int Hr = (int)((int)gstTimeLocal.time/3600);
    for(int i = 0;i<charLen;i++){
        dataUChar[i] = (uint8_t)(page.mid(i*2,2).toInt(&ok,16));
    }
    uint8_t buffer[charLen];
    //uint8_t buffer[charLen];
    int iod[6];
    int toh;
    int maskId;
    int iodId;
    bool okDecode;
    int index;
    //add by zhangrunzhi 2022 12.07
    hassat_t hasSat;
    initHassat_t(hasSat);
    initHassat_t(hasSat_o);
    //for(int i = 0;i<pidLen;i++){
        //memcpy(buffer,dataUChar+i*53,53*sizeof(char));
        memcpy(buffer,dataUChar,charLen*sizeof(char));
        decodePageHead(buffer,iod);
        toh = getbitu(buffer, 0,12);
        int HrT = Hr*3600 + toh;
        if(HrT <= gstTimeLocal.time + gstTimeLocal.sec){
            gstTime.sec = 0;
            gstTime.time = HrT;
        }
        else{
            gstTime.sec = 0;
            gstTime.time = HrT -3600;
        }
        //hasSat.iodTime[0] = gpst2gstM(gstTime);
        hasSat.iodTime[0] = gstTime;
        hasSat.iodTime[5] = gpstTime;
        maskId = getbitu(buffer,22,5);
        iodId = getbitu(buffer,27,5);
        uint8_t *p = buffer + 4;
        index = 0;

        memcpy(hasSat.iod,iod,6*sizeof(int));
        //add by zhangrunzhi 2022.12.4
        //change 2022 12 07
        if(iod[0]&&iod[1]&&!iod[2]&&!iod[3]&&iod[4]&&iod[5]){

        }
        else if(!iod[0]&&!iod[1]&&iod[2]&&!iod[3]&&!iod[4]&&!iod[5]){

        }
        else{
            return false;
            //qDebug()<<"iod:"<<iod[0]<<iod[1]<<iod[2]<<iod[3]<<iod[4]<<iod[5]<<endl;
        }
        if(iod[0]){
            okDecode = decodeMask(p,hasSat,&index);
            if(!okDecode){
                errorLine = QString::number(gpstTime.time)+QString("  Decode Mask failed");
                traces(1,errorLine);
                return false;
            }
            hasSat.maskId = maskId;
        }
        else{
            if(maskId == hasSat_i.maskId){
                hasSat.nsat = hasSat_i.nsat;
                hasSat.nsys = hasSat_i.nsys;
                for(int n = 0;n<hasSat.nsat;n++){
                    hasSat.sat[n].sat = hasSat_i.sat[n].sat;
                    memcpy(hasSat.sat[n].SigM,hasSat.sat[n].SigM,MAXSIG*sizeof(int));
                    hasSat.sat[n].nm = hasSat.sat[n].nm;
                }
                memcpy(hasSat.sys,hasSat_i.sys,MAXSYS*sizeof(int));
                hasSat.maskId = maskId;
                hasSat.iodTime[0] = hasSat_i.iodTime[0];
            }
            else{
                errorLine = QString("MaskId changed,maskId = ")+QString::number(maskId)+QString(",  hasSat_i.maskId = ")+QString::number(hasSat_i.maskId);
                traces(1,errorLine);
                return false;
            }
        }
        //add by zhangrunzhi 2022.12.4
        if(iod[0]&&iod[1]&&!iod[2]&&!iod[3]&&!iod[4]&&!iod[5]){
            //qDebug()<<"success iod:"<<iod[0]<<iod[1]<<iod[2]<<iod[3]<<iod[4]<<iod[5]<<endl;
        }
        if(!iod[0]&&!iod[1]&&!iod[2]&&iod[3]&&!iod[4]&&!iod[5]){
            //qDebug()<<"success iod:"<<iod[0]<<iod[1]<<iod[2]<<iod[3]<<iod[4]<<iod[5]<<endl;
        }
        if(index > charLen*8){
            ErrorMsgPos(errorLine);
            errorLine = QString("index: ")+QString::number(index)+QString("   decodeLen: ")+QString::number(charLen*8) +errorLine +QString("iod:  ")+QString::number(iod[0])+QString::number(iod[1])+QString::number(iod[2])+QString::number(iod[3])+QString::number(iod[4])+QString::number(iod[5]);
            traces(1,errorLine);
            return false;
        }
        if(iod[1]){
            okDecode = decodeOrbit(p,hasSat,&index);
            if(!okDecode){
                errorLine = QString("Decode Orbit failed");
                traces(1,errorLine);
                return false;
            }
        }
        else{
            if(maskId == hasSat_i.maskId && iodId == hasSat_i.iodSet){
                hasSat = hasSat_i;
            }
            //add by zhangrunzhi 2022 12.4
            else{
                //qDebug()<<"maskId:"<<maskId<<"  maskIdLast:"<<hasSat_i.maskId<<"iodSet:"<<iodId<<"  iodSetLast:"<<hasSat_i.iodSet<<endl;
                return false;
            }
        }
        if(index > charLen*8){
            ErrorMsgPos(errorLine);
            errorLine = QString("index: ")+QString::number(index)+QString("   decodeLen: ")+QString::number(charLen*8) +errorLine +QString("iod:  ")+QString::number(iod[0])+QString::number(iod[1])+QString::number(iod[2])+QString::number(iod[3])+QString::number(iod[4])+QString::number(iod[5]);
            traces(1,errorLine);
            return false;
        }
        if(iod[0] == 0 && iod[2] && maskId == hasSat_i.maskId){
            okDecode = decodeCF(p,hasSat,&index);
            if(!okDecode){
                errorLine = QString("Decode Clock Full failed");
                traces(1,errorLine);
                return false;
            }
        }
        else if(iod[0] == 0 && iod[2] && (maskId != hasSat_i.maskId)){
            errorLine = QString("CF failed, iod:  ")+QString::number(iod[0])+QString::number(iod[1])+QString::number(iod[2])+QString::number(iod[3])+QString::number(iod[4])+QString::number(iod[5]);
            traces(1,errorLine);
            return false;
        }
        else if(iod[0] == 0&& iod[2] == 1){
            okDecode = decodeCF(p,hasSat,&index);
            if(!okDecode){
                errorLine = QString("Decode Clock Full failed");
                traces(1,errorLine);
                return false;
            }
        }
        //add by zhangrunzhi 2022 12 07
        else if(iod[0] && !iod[2] && maskId == hasSat_i.maskId && iodId == hasSat_i.iodSet){
            for(int j = 0;j<hasSat.nsat;j++){
                hasSat.sat[j].ssr.dclk[0] = hasSat_i.sat[j].ssr.dclk[0];
                //qDebug()<<dcc * 0.0025<<prn<<endl;
                hasSat.sat[j].ssr.t0[1] = hasSat_i.sat[j].ssr.t0[1];
            }
        }
        if(index > charLen*8){
            ErrorMsgPos(errorLine);
            errorLine = QString("index: ")+QString::number(index)+QString("   decodeLen: ")+QString::number(charLen*8) +errorLine +QString("iod:  ")+QString::number(iod[0])+QString::number(iod[1])+QString::number(iod[2])+QString::number(iod[3])+QString::number(iod[4])+QString::number(iod[5]);
            traces(1,errorLine);
            return false;
        }
        if(iod[3]){
            okDecode = decodeCS(p,hasSat,&index);
            if(!okDecode){
                errorLine = QString("Decode Clock Subset failed");
                traces(1,errorLine);
                return false;
            }
        }
        if(index > charLen*8){
            ErrorMsgPos(errorLine);
            errorLine = QString("index: ")+QString::number(index)+QString("   decodeLen: ")+QString::number(charLen*8) +errorLine +QString("iod:  ")+QString::number(iod[0])+QString::number(iod[1])+QString::number(iod[2])+QString::number(iod[3])+QString::number(iod[4])+QString::number(iod[5]);
            traces(1,errorLine);
            return false;
        }
        if(iod[4]){
            okDecode = decodeCodeBias(p,hasSat,&index);
            if(!okDecode){
                errorLine = QString("Decode Code Bias failed");
                traces(1,errorLine);
                return false;
            }
        }
        if(index > charLen*8){
            ErrorMsgPos(errorLine);
            errorLine = QString("index: ")+QString::number(index)+QString("   decodeLen: ")+QString::number(charLen*8) +errorLine +QString("iod:  ")+QString::number(iod[0])+QString::number(iod[1])+QString::number(iod[2])+QString::number(iod[3])+QString::number(iod[4])+QString::number(iod[5]);
            traces(1,errorLine);
            return false;
        }
        if(iod[5]){
            okDecode = decodePhaseBias(p,hasSat,&index);
            if(!okDecode){
                errorLine = QString("Decode Phase Bias failed");
                traces(1,errorLine);
                return false;
            }
        }
        if(index > charLen*8){
            ErrorMsgPos(errorLine);
            errorLine = QString("index: ")+QString::number(index)+QString("   decodeLen: ")+QString::number(charLen*8) +errorLine +QString("iod:  ")+QString::number(iod[0])+QString::number(iod[1])+QString::number(iod[2])+QString::number(iod[3])+QString::number(iod[4])+QString::number(iod[5]);
            traces(1,errorLine);
            return false;
        }
        bool decodeBool;
        decodeBool = adjustPos(p,index,(charLen - 4)*8);
        if(decodeBool && iod[2]==1){
            decodeBool =true;
        }
        else if(iod[2]==1 && !decodeBool){
            decodeBool =false;
        }
        if(decodeBool){
            hasSat_o = hasSat;
            hasSat_o.iodSet = iodId;
        }
        else{
            errorLine = QString("Decode length Error,")+QString("iod:  ")+QString::number(iod[0])+QString::number(iod[1])+QString::number(iod[2])+QString::number(iod[3])+QString::number(iod[4])+QString::number(iod[5]);
            traces(1,errorLine);
            return false;
        }
    return true;
    //}


}*/
bool HASPPP::adjustPos(uint8_t *dataChar,int index,int indexFin){
	/* Due to the data of long combination, it is necessary to determine whether \
	   the decoding is correct by length*/
    uint8_t table[4] = {85,85,85,85};
    int len = ((indexFin - index)>= 32)?32:(indexFin - index);
    int table32 = getbitu(table,0,len);
    int data32  = getbitu(dataChar,index,len);
    int table4  = getbitu(table,0,4);
    int data4   = getbitu(dataChar,index-4,4);
    /*if(table32 == data32 && table4 != data4){
        return true;
    }
    else{
        return false;
    }*/
    /*    */
    if(table32 == data32){
        return true;
    }
    else{
        return false;
    }

}
bool HASPPP::decodePhaseBias(uint8_t *p,hassat_t &hasSat,int *indexF){
    int index = *indexF,code;
    double pbias;
    int pdi;
    int intervalOrder = getbitu(p,index,4),interval;
    int valTable[16]={5,10,15,20,30,60,90,120,180,240,300,600,900,1800,3600,-1};
    index = index + 4;
    if(intervalOrder != 15){
        interval = valTable[intervalOrder];
    }
    traces(1,hasSat.iodTime[0],"decodePhaseBias!");
    hasSat.iodTime[5] = hasSat.iodTime[0];
    hasSat.iodTime[5].time = hasSat.iodTime[5].time + interval;
    for(int i = 0;i<hasSat.nsat;i++){
        for(int j = 0;j<16;j++){
            if(hasSat.sat[i].SigM[j] == 1){
                pbias = getbits(p,index,11); index = index + 11;
                pdi = getbitu(p,index,2); index = index + 2;
                //qDebug()<<pdi<<endl;
                if(pbias != -1024){
                    code=cbiasCode2Index(hasSat.sat[i].sat,j);
                    hasSat.sat[i].ssr.pbias[code-1]=pbias * 0.01;
                    //qDebug()<<hasSat.sat[i].ssr.pbias[code-1]<<endl;
                    hasSat.sat[i].ssr.t0[3] = gst2gpstM(hasSat.iodTime[5]);
                }

            }
        }
    }
    *indexF = index;
    return true;
}
int HASPPP::cbiasCode2Index(int sat,int cbindex) {
    int code=-1,k,sys;
    char prn[4];
    satno2id(sat,prn);
    if(prn[0] == 'G'){
        sys = 0;
    }
    else if(prn[0] == 'E'){
        sys = 1;
    }
    else{
        return -1;
    }
    for(k=0;k<16;k++) {
        if(*cbiascodes[sys][k]) {
            if(k==cbindex) {
                code=obs2code(cbiascodes[sys][k]);
                break;
            }
        }
    }
    return code;
}
bool HASPPP::decodeCodeBias(uint8_t *p,hassat_t &hasSat,int *indexF){
    int index = *indexF,code;
    char buffstr[1024];
    double cbias;
    int intervalOrder = getbitu(p,index,4),interval;
    int valTable[16]={5,10,15,20,30,60,90,120,180,240,300,600,900,1800,3600,-1};
    index = index + 4;
    if(intervalOrder != 15){
        interval = valTable[intervalOrder];
    }
    hasSat.iodTime[4] = hasSat.iodTime[0];
    hasSat.iodTime[4].time = hasSat.iodTime[4].time + interval;
    char prn[4];
    char obs[5];
    double euEp[6];
    time2epoch(hasSat.iodTime[4],euEp);
    std::sprintf(buffstr,"> CODE_BIAS %4.0f %2.0f %2.0f %2.0f %2.0f %4.1f %4d",euEp[0],euEp[1],euEp[2],euEp[3],euEp[4],euEp[5],hasSat.nsat);
    std::string EU = std::string(buffstr);
    comEU(EU);
    std::string EUC;
    int cbiasnum=0;
    for(int i = 0;i<hasSat.nsat;i++){
        satno2id(hasSat.sat[i].sat,prn);
        EUC = std::string(prn);
        EU.clear();
        cbiasnum = 0;
        for(int j = 0;j<16;j++){
            if(hasSat.sat[i].SigM[j] == 1){
                cbias = getbits(p,index,11); index = index + 11;
                //qDebug()<<QString::number(cbias* 0.02)<<endl;
                if(cbias != -1024){
                    code=cbiasCode2Index(hasSat.sat[i].sat,j);
                    memcpy(obs,code2obs(code),3*sizeof(char));
                    hasSat.sat[i].ssr.cbias[code-1]=cbias * 0.02;
                    std::sprintf(buffstr,"%11.4f",hasSat.sat[i].ssr.cbias[code-1]);
                    EU = EU+std::string("   ")+std::string(obs)+std::string(buffstr);

                    //qDebug()<<" "<<hasSat.sat[i].ssr.cbias[code-1];
                    hasSat.sat[i].ssr.t0[2] = gst2gpstM(hasSat.iodTime[4]);
                    cbiasnum++;
                }

            }

        }
        std::sprintf(buffstr,"%5d",cbiasnum);
        EU = EUC+buffstr+EU;

        comEU(EU);

        //qDebug()<<prn<<endl;
    }
    *indexF = index;
    return true;
}
bool HASPPP::decodeCS(uint8_t *p,hassat_t &hasSat,int *indexF){
    int index = *indexF;
    char buffstr[1024];
    int intervalOrder = getbitu(p,index,4),interval;
    int valTable[16]={5,10,15,20,30,60,90,120,180,240,300,600,900,1800,3600,-1};
    index = index + 4;
    if(intervalOrder != 15){
        interval = valTable[intervalOrder];
    }

    traces(1,hasSat.iodTime[0],"decodeCS!");
    hasSat.iodTime[3] = hasSat.iodTime[0];
    hasSat.iodTime[3].time = hasSat.iodTime[3].time + interval;
    int nsys,gnssId,orderB=0,orderA;
    double dcm;

    nsys = getbitu(p,index,4);index = index + 4;
    std::string errorLine;
    /*if(nsys >MAXSYS){
        errorLine = QString("nsys: ")+QString::number(nsys);
        errorLine = QString("CS");
        traces(1,errorLine);
        return false;
    }*/
    double euEp[6];
    time2epoch(hasSat.iodTime[3],euEp);
    std::sprintf(buffstr,"> CLOCK %4.0f %2.0f %2.0f %2.0f %2.0f %4.1f %4d",euEp[0],euEp[1],euEp[2],euEp[3],euEp[4],euEp[5],hasSat.nsat);
    std::string EU = std::string(buffstr);
    comEU(EU);
    for(int i = 0;i<nsys;i++){
        gnssId = getbitu(p,index,4);index = index + 4;
        dcm = getbitu(p,index,2) + 1;index = index + 2;
        if(gnssId == 0){
            orderB = 0;
            orderA = hasSat.sys[0];
        }
        else if(gnssId == 2){
            orderB = hasSat.sys[0];
            orderA = hasSat.sys[0] + hasSat.sys[1];
        }
        else{
            continue;
        }
        //int *satM = new int[orderA];
		int satM[1000];
        double dcc;
        for(int j = orderB; j<orderA;j++){
            satM[j] = getbitu(p,index,1);index = index + 1;
        }
        for(int j = orderB; j<orderA;j++){
            if(satM[j] == 0){
                continue;
            }
            dcc = getbits(p,index,13);index = index + 13;
            if(dcc == -4096 || dcc == 4095){
                continue;
            }
            else{
                hasSat.sat[j].ssr.dclk[0] = dcm * dcc * 0.0025;
                //qDebug()<<"aa"<<hasSat.sat[j].ssr.dclk[0]<<endl;
                hasSat.sat[j].ssr.t0[1] = gst2gpstM(hasSat.iodTime[3]);
                char prn[4];
                satno2id(hasSat.sat[j].sat,prn);
                std::sprintf(buffstr,"    %8d    %7.4f",hasSat.sat[j].ssr.iode,hasSat.sat[j].ssr.dclk[0]);
                EU = std::string(prn)+std::string(buffstr);
                comEU(EU);
            }
        }
		//delete[] satM;
    }
    *indexF = index;
    return true;
}
bool HASPPP::decodeCF(uint8_t *p,hassat_t &hasSat,int *indexF){
    int index = *indexF;
    char buffstr[1024];
    int intervalOrder = getbitu(p,index,4),interval;
    int valTable[16]={5,10,15,20,30,60,90,120,180,240,300,600,900,1800,3600,-1};
    index = index + 4;
    int sysoffset=0;
    int sys = sat2sys(hasSat.sat[0].sat);
    if(intervalOrder != 15){
        interval = valTable[intervalOrder];
    }

    hasSat.iodTime[2] = hasSat.iodTime[0];
    hasSat.iodTime[2].time = hasSat.iodTime[2].time + interval;
    int DCM[MAXSYS];
    //debug by zhangrunzhi 2023 6 16
    if(hasSat.nsys==1&&sys==1){
        sysoffset =1;
    }
    for(int i = 0;i<hasSat.nsys;i++){
        DCM[i+sysoffset] = getbitu(p,index,2) + 1; index = index + 2;
    }
    double dcc;
    char prn[4];
    double euEp[6];
    time2epoch(hasSat.iodTime[2],euEp);
    std::sprintf(buffstr,"> CLOCK %4.0f %2.0f %2.0f %2.0f %2.0f %4.1f %4d",euEp[0],euEp[1],euEp[2],euEp[3],euEp[4],euEp[5],hasSat.nsat);
    std::string EU = std::string(buffstr);
    comEU(EU);

    for(int j = 0;j<hasSat.nsat;j++){
        dcc = getbits(p,index,13);index = index + 13;
        if(dcc == -4096 || dcc == 4095){
            continue;
        }
        else{
            sys = sat2sys(hasSat.sat[j].sat);
            if(sys == -1){
                continue;
            }
            else{
                satno2id(hasSat.sat[j].sat,prn);
                hasSat.sat[j].ssr.dclk[0] = DCM[sys] * dcc * 0.0025;
                //qDebug()<<dcc * 0.0025<<prn<<endl;
                hasSat.sat[j].ssr.t0[1] = gst2gpstM(hasSat.iodTime[2]);
                std::sprintf(buffstr,"    %8d    %7.4f",hasSat.sat[j].ssr.iode,hasSat.sat[j].ssr.dclk[0]);
                EU = std::string(prn)+std::string(buffstr);
                comEU(EU);
                testClock(hasSat.sat[j].sat,hasSat.sat[j].ssr.dclk[0],hasSat.iodTime[0],hasSat.sat[j].ssr.t0[1]);
            }
        }

    }
    *indexF = index;
    return true;

}
int HASPPP::sat2sys(int sat){
    char prn[4];
    satno2id(sat,prn);
    if(prn[0] == 'G'){
        return 0;
    }
    else if(prn[0] == 'E'){
        return 1;
    }
    else {
        return -1;
    }
}
bool HASPPP::decodeOrbit(uint8_t *p,hassat_t &hasSat,int *indexF){
    std::string EU;
    double euEp[6];
    char buffstr[1024];
    int index = *indexF;
    int intervalOrder = getbitu(p,index,4),interval;
    int valTable[16]={5,10,15,20,30,60,90,120,180,240,300,600,900,1800,3600,-1};
    index = index + 4;

    if(intervalOrder != 15){
        interval = valTable[intervalOrder];
        //qDebug()<<"interval"<<interval<<endl;
    }
    hasSat.iodTime[1] = hasSat.iodTime[0];
    hasSat.iodTime[1].time = hasSat.iodTime[1].time + interval;
    int iodref=0;
    double dr,dit,dct;
    //QString errorLine;
    //for(int i =0;i<hasSat.nsat;)
    time2epoch(hasSat.iodTime[1],euEp);
    std::sprintf(buffstr,"> ORBIT %4.0f %2.0f %2.0f %2.0f %2.0f %4.1f %3d",euEp[0],euEp[1],euEp[2],euEp[3],euEp[4],euEp[5],hasSat.nsat);
    EU = std::string(buffstr);
    comEU(EU);
    for(int i = 0;i<hasSat.nsat;i++){
        hassatp_t satT = hasSat.sat[i];
        char prn[4];
        satno2id(satT.sat,prn);
       if(prn[0] =='G'){
            iodref = getbitu(p,index,8);
            dr = getbits(p,index+8,13);
            dit = getbits(p,index+21,12);
            dct = getbits(p,index+33,12);
            index=index+45;
            //cout<<iodref<<endl;
        }
            //cout<<dr*0.0025<<endl;
        else if(prn[0] == 'E'){
            iodref = getbitu(p,index,10);
            dr = getbits(p,index+10,13);
            dit = getbits(p,index+23,12);
            dct = getbits(p,index+35,12);
            index=index+47;
             //cout<<index<<endl;
            //cout<<iodref<<endl;
         }

       //else{
          // ErrorMsgPos(errorLine);
           // errorLine = QString("Decode orbit error system is not exist") + errorLine;
           // traces(1,errorLine);
        //}
        //cout<<summ<<endl;
        if(dr != -4096 && dr != -2048){
            satT.ssr.deph[0] = dr * 0.0025;
            satT.ssr.deph[1] = dit * 0.008;
            satT.ssr.deph[2] = dct * 0.008;
            satT.iode = iodref;
           //qDebug()<<satT.iode<<endl;
            //qDebug()<<satT.ssr.deph[0]<<endl;
            //qDebug()<<"cc"<<satT.iode<<endl;
            satT.ssr.iode = iodref;
            //qDebug()<<"prn:"<<prn<<"iodref: "<<satT.ssr.iode<<endl;
            satT.ssr.t0[0] = gst2gpstM(hasSat.iodTime[1]);
        }
        //cout<<iodref<<endl;
            else{
                 memset(satT.ssr.deph,0,3*sizeof(double));
                 continue;
             }
        //qDebug()<<"c"<<satT.ssr.iode<<endl;
             hasSat.sat[i] = satT;
             std::sprintf(buffstr,"    %8d    %7.4f    %7.4f    %7.4f",satT.iode,satT.ssr.deph[0],satT.ssr.deph[1],satT.ssr.deph[2]);
             EU = std::string(prn)+std::string(buffstr);

             comEU(EU);
             /* test */
             //double iodTIme[6],iodTIMe[6];
             //time2epoch(gst2gpstM(hasSat.iodTime[5]),iodTIme);
            // time2epoch(gst2gpstM(hasSat.iodTime[1]),iodTIMe);
             //QString matchResults = "/media/david/My Passport/GHAS/iodRef.txt";
             //QFile saveFile(matchResults);//Saved file
             //if (!saveFile.open(QFile::WriteOnly|QFile::Text|QFile::Append))
             //{
            //     continue;
             //}
             //QTextStream saveFileOut(&saveFile);
             //saveFileOut<<"GPSTtime: "<<iodTIme[0]<<"-"<<iodTIme[1]<<"-"<<iodTIme[2]<<":"<<iodTIme[3]<<":"<<iodTIme[4]<<":"<<iodTIme[5]<<"  "<<prn<<": "<<iodref<<endl;
             //saveFileOut<<"GPSTtime: "<<iodTIMe[0]<<"-"<<iodTIMe[1]<<"-"<<iodTIMe[2]<<":"<<iodTIMe[3]<<":"<<iodTIMe[4]<<":"<<iodTIMe[5]<<"  "<<prn<<": "<<iodref<<endl;


             //int a=0;
             //if(n_pid!=2&&n_pid!=16){
             //    a=a+1;
             //}

             //saveFileOut.flush();
             //saveFile.close();


             //qDebug()<<iodref<<satT.ssr.deph[0]<<satT.ssr.deph[1]<<satT.ssr.deph[2]<<prn<<endl;
    }

    *indexF = index;
    return true;

}
    // zhangrunzhi
   /*for(int i = 0;i<hasSat.nsat;i++){
        {
        hassatp_t satT = hasSat.sat[i];
        char prn[3];
        satno2id(satT.sat,prn);
        if(prn[0] == 'G'){
            iodref = getbitu(p,index,8);index = index + 8;
            //qDebug()<<iodref<<endl;
        }
        else if(prn[0] == 'E'){
            iodref = getbitu(p,index,10);
            index = index + 10;
        }
        //cout<<iodref<<endl;

       // else{
           // ErrorMsgPos(errorLine);
           // errorLine = QString("Decode orbit error system is not exist") + errorLine;
            //traces(1,errorLine);
       // }
        dr = getbits(p,index,13);index = index + 13;
        dit = getbits(p,index,12);index = index + 12;
        dct = getbits(p,index,12);index = index + 12;
        //cout<<"dr"<<dr*0.0025<<endl;
        if(dr != -4096 && dr != -2048 ){
            satT.ssr.deph[0] = dr * 0.0025;
            satT.ssr.deph[1] = dit * 0.008;
            satT.ssr.deph[2] = dct * 0.008;
            satT.iode = iodref;
            //qDebug()<<satT.ssr.deph[0]<<endl;
            //qDebug()<<"cc"<<satT.iode<<endl;
            satT.ssr.iode = iodref;
            /* GPST*/

         /* satT.ssr.t0[0] = gst2gpstM(hasSat.iodTime[1]);
        }
       else{
            memset(satT.ssr.deph,0,3*sizeof(double));
            continue;
        }
        hasSat.sat[i] = satT;
        /*
        QString te = "8000";
        uint8_t te8[2];
        bool ok;
        for(int m = 0;m<2;m++){
            te8[m] = (uint8_t)(te.mid(m*2,2).toInt(&ok,16));
        }
        double e = getbits(te8,0,12);
        if(e == -2048){
            ok =false;
        }
        */
      /* }

    }*/

   /* *indexF = index;
    return true;

}*/

bool HASPPP::decodeMask(uint8_t *p,hassat_t &hasSat,int *indexF){
    int nsys,gnssId,index,satM[40],sigM[16],nSat,nSig,cmaf,nm;
    int indexTemp;
    char buffstr[1024];
    nsys = getbitu(p,0,4);
    index = 4;
    indexTemp = index;
    int sigOrder[16];
    hasSat.nsys = nsys;
    std::string errorLine;
    if(nsys>MAXSYS){
        ErrorMsgPos(errorLine);
        std::sprintf(buffstr,"%d",nsys);
        errorLine = std::string("nsys = ") + std::string(buffstr)+std::string(" ")+ errorLine;
        traces(1,hasSat.iodTime[0],errorLine);
        return false;
    }
    hasSat.nsat = 0;
    for(int j = 0;j<nsys;j++){
        nSat = 0;nSig = 0;
        gnssId = getbitu(p,index,4);index = index + 4;
        for(int k = 0;k<40;k++){
            satM[k] = getbitu(p,index,1);
            nSat = nSat + satM[k];
            index = index + 1;
            if(satM[k] == 0){
                continue;
            }
            hasSat.nsat++;
            hasSat.sat[hasSat.nsat-1].sat = prn2sat(gnssId,k+1);
            if(gnssId == 0){
                hasSat.sys[0] = hasSat.sys[0] + 1;
            }
            else if(gnssId == 2){
                hasSat.sys[1] = hasSat.sys[1] + 1;
            }

        }

        for(int k = 0;k<16;k++){
            sigM[k] = getbitu(p,index,1);
            if(sigM[k] == 1){
                sigOrder[nSig] = k;
            }
            nSig = nSig + sigM[k];
            index = index + 1;
        }
        cmaf = getbitu(p,index,1);
        index = index + 1;
        int sat,pos,flag;
        if(cmaf == 0){
            //index = index + nSig*nSat;
            for(int i = 0;i<40;i++){
                if(satM[i] == 1){
                    sat = prn2sat(gnssId,i+1);
                    pos = satOrderPos(hasSat,sat);
                    if(pos != -1){
                        memcpy(hasSat.sat[pos].SigM,sigM,16*sizeof(int));
                    }
                }
            }
        }
        else{
            for(int i = 0;i<40;i++){
                if(satM[i] == 1){
                    int sigMSat[16];
                    sat = prn2sat(gnssId,i+1);
                    memcpy(sigMSat,sigM,16*sizeof(int));
                    for(int n = 0;n<nSig;n++){
                        flag = getbitu(p,index,1);index =index + 1;
                        if(flag == 0){
                            sigMSat[sigOrder[n]] = 0;
                        }
                    }
                    pos = satOrderPos(hasSat,sat);
                    if(pos != -1){
                        memcpy(hasSat.sat[pos].SigM,sigMSat,16*sizeof(int));
                    }
                }
                else{
                    continue;
                }
            }
        }
        nm = getbitu(p,index,3);
        if(nm!=0){
            traces(1,hasSat.iodTime[0],"MASK BLOCK FAILED,nm =%d");
        }
        index = index + 3;
        if(cmaf != 0 && index != indexTemp + 64 + nSat *nSig){
            ErrorMsgPos(errorLine);
            errorLine = std::string("cmaf error") + errorLine;
            traces(1,hasSat.iodTime[0],errorLine);
            return false;
        }
        else{
            indexTemp = index;
        }
    }
    index = index + 6;
    //qDebug()<<index<<endl;
    *indexF = index;
    return true;
}
int HASPPP::satOrderPos(hassat_t hasSat,int sat){
    int i = 0;
    for(i = 0;i<hasSat.nsat;i++){
        if(hasSat.sat[i].sat == sat){
            return i;
        }
    }
    if(i ==hasSat.nsat){
        return -1;
    }
}
int HASPPP::prn2sat(int gnssId,int svid){
    std::string sysString,prnString;
    char buffstr[1024];
    char prn[4];
    if(gnssId == 0){
        sysString = std::string("G");
    }
    else if(gnssId == 2){
        sysString = std::string("E");
    }
    else {
        sysString = std::string(" ");
    }

    std::sprintf(buffstr,"%d",svid);
    if(svid <10){
        prnString = sysString + '0' + std::string(buffstr);
    }
    else{
        prnString = sysString + std::string(buffstr);
    }
    //char *prnTemp = (prnString.toLatin1().data());
//    QByteArray prnTemp = prnString.toLatin1();
//    prnTemp = prnTemp.data();
    memcpy(prn,prnString.data(),3*sizeof(char));
    int sat = satid2no(prn);
    return sat;
}
void HASPPP::decodePageHead(uint8_t *buffer,int *iod){
    for(int i = 0;i <6;i++){
        iod[i] = getbitu(buffer, 12+i,1);
    }
    //qDebug()<<"iod1"<<iod[0]<<endl;
    //qDebug()<<"iod2"<<iod[1]<<endl;
    //qDebug()<<"iod3"<<iod[2]<<endl;
    //qDebug()<<"iod4"<<iod[3]<<endl;
    //qDebug()<<"iod5"<<iod[4]<<endl;
    //qDebug()<<"iod6"<<iod[5]<<endl;
}

/* extract unsigned/signed bits ------------------------------------------------
* extract unsigned/signed bits from byte data
* args   : uint8_t *buff    I   byte data
*          int    pos       I   bit position from start of data (bits)
*          int    len       I   bit length (bits) (len<=32)
* return : extracted unsigned/signed bits
*-----------------------------------------------------------------------------*/
uint32_t HASPPP::getbitu(const uint8_t *buff, int pos, int len)
{
    uint32_t bits=0;
    int i;
    for (i=pos;i<pos+len;i++) bits=(bits<<1)+((buff[i/8]>>(7-i%8))&1u);
    return bits;
}
int32_t HASPPP::getbits(const uint8_t *buff, int pos, int len)
{
    uint32_t bits=getbitu(buff,pos,len);
    if (len<=0||32<=len||!(bits&(1u<<(len-1)))) return (int32_t)bits;
    return (int32_t)(bits|(~0u<<len)); /* extend sign */
}
/* add time --------------------------------------------------------------------
* add time to gtime_t struct
* args   : gtime_t t        I   gtime_t struct
*          double sec       I   time to add (s)
* return : gtime_t struct (t+sec)
*-----------------------------------------------------------------------------*/
gtime_t HASPPP::timeadd(gtime_t t, double sec)
{
    double tt;

    t.sec+=sec; tt=floor(t.sec); t.time+=(int)tt; t.sec-=tt;
    return t;
}
/* time difference -------------------------------------------------------------
* difference between gtime_t structs
* args   : gtime_t t1,t2    I   gtime_t structs
* return : time difference (t1-t2) (s)
*-----------------------------------------------------------------------------*/
double HASPPP::timediff(gtime_t t1, gtime_t t2)
{
    return difftime(t1.time,t2.time)+t1.sec-t2.sec;
}

/* satellite number to satellite id --------------------------------------------
* convert satellite number to satellite id
* args   : int    sat       I   satellite number
*          char   *id       O   satellite id (Gnn,Rnn,Enn,Jnn,Cnn,Inn or nnn)
* return : none
*-----------------------------------------------------------------------------*/
void HASPPP::satno2id(int sat, char *id)
{
    int prn;
    switch (satsys(sat,&prn)) {
        case SYS_GPS: sprintf(id,"G%02d",prn-MINPRNGPS+1); return;
        case SYS_GLO: sprintf(id,"R%02d",prn-MINPRNGLO+1); return;
        case SYS_GAL: sprintf(id,"E%02d",prn-MINPRNGAL+1); return;
        case SYS_QZS: sprintf(id,"J%02d",prn-MINPRNQZS+1); return;
        case SYS_CMP: sprintf(id,"C%02d",prn-MINPRNCMP+1); return;
        case SYS_IRN: sprintf(id,"I%02d",prn-MINPRNIRN+1); return;
        case SYS_LEO: sprintf(id,"L%02d",prn-MINPRNLEO+1); return;
        case SYS_SBS: sprintf(id,"%03d" ,prn); return;
    }
    strcpy(id,"");
}
/* gps time to time ------------------------------------------------------------
* convert week and tow in gps time to gtime_t struct
* args   : int    week      I   week number in gps time
*          double sec       I   time of week in gps time (s)
* return : gtime_t struct
*-----------------------------------------------------------------------------*/
gtime_t HASPPP::gpst2time(int week, double sec)
{
    static const double gpst0[]={1980,1, 6,0,0,0}; /* gps time reference */
    gtime_t t=epoch2time(gpst0);

    if (sec<-1E9||1E9<sec) sec=0.0;
    t.time+=(time_t)86400*7*week+(int)sec;
    t.sec=sec-(int)sec;
    return t;
}
/* satellite id to satellite number --------------------------------------------
* convert satellite id to satellite number
* args   : char   *id       I   satellite id (nn,Gnn,Rnn,Enn,Jnn,Cnn,Inn or Snn)
* return : satellite number (0: error)
* notes  : 120-142 and 193-199 are also recognized as sbas and qzss
*-----------------------------------------------------------------------------*/
int HASPPP::satid2no(const char *id)
{
    int sys,prn;
    char code;

    if (sscanf(id,"%d",&prn)==1) {
        if      (MINPRNGPS<=prn&&prn<=MAXPRNGPS) sys=SYS_GPS;
        else if (MINPRNSBS<=prn&&prn<=MAXPRNSBS) sys=SYS_SBS;
        else if (MINPRNQZS<=prn&&prn<=MAXPRNQZS) sys=SYS_QZS;
        else return 0;
        return satno(sys,prn);
    }
    if (sscanf(id,"%c%d",&code,&prn)<2) return 0;

    switch (code) {
        case 'G': sys=SYS_GPS; prn+=MINPRNGPS-1; break;
        case 'R': sys=SYS_GLO; prn+=MINPRNGLO-1; break;
        case 'E': sys=SYS_GAL; prn+=MINPRNGAL-1; break;
        case 'J': sys=SYS_QZS; prn+=MINPRNQZS-1; break;
        case 'C': sys=SYS_CMP; prn+=MINPRNCMP-1; break;
        case 'I': sys=SYS_IRN; prn+=MINPRNIRN-1; break;
        case 'L': sys=SYS_LEO; prn+=MINPRNLEO-1; break;
        case 'S': sys=SYS_SBS; prn+=100; break;
        default: return 0;
    }
    return satno(sys,prn);
}

/* satellite system+prn/slot number to satellite number ------------------------
* convert satellite system+prn/slot number to satellite number
* args   : int    sys       I   satellite system (SYS_GPS,SYS_GLO,...)
*          int    prn       I   satellite prn/slot number
* return : satellite number (0:error)
*-----------------------------------------------------------------------------*/
int HASPPP::satno(int sys, int prn)
{
    if (prn<=0) return 0;
    switch (sys) {
        case SYS_GPS:
            if (prn<MINPRNGPS||MAXPRNGPS<prn) return 0;
            return prn-MINPRNGPS+1;
        case SYS_GLO:
            if (prn<MINPRNGLO||MAXPRNGLO<prn) return 0;
            return NSATGPS+prn-MINPRNGLO+1;
        case SYS_GAL:
            if (prn<MINPRNGAL||MAXPRNGAL<prn) return 0;
            return NSATGPS+NSATGLO+prn-MINPRNGAL+1;
        case SYS_QZS:
            if (prn<MINPRNQZS||MAXPRNQZS<prn) return 0;
            return NSATGPS+NSATGLO+NSATGAL+prn-MINPRNQZS+1;
        case SYS_CMP:
            if (prn<MINPRNCMP||MAXPRNCMP<prn) return 0;
            return NSATGPS+NSATGLO+NSATGAL+NSATQZS+prn-MINPRNCMP+1;
        case SYS_IRN:
            if (prn<MINPRNIRN||MAXPRNIRN<prn) return 0;
            return NSATGPS+NSATGLO+NSATGAL+NSATQZS+NSATCMP+prn-MINPRNIRN+1;
        case SYS_LEO:
            if (prn<MINPRNLEO||MAXPRNLEO<prn) return 0;
            return NSATGPS+NSATGLO+NSATGAL+NSATQZS+NSATCMP+NSATIRN+
                   prn-MINPRNLEO+1;
        case SYS_SBS:
            if (prn<MINPRNSBS||MAXPRNSBS<prn) return 0;
            return NSATGPS+NSATGLO+NSATGAL+NSATQZS+NSATCMP+NSATIRN+NSATLEO+
                   prn-MINPRNSBS+1;
    }
    return 0;
}
/* satellite number to satellite system ----------------------------------------
* convert satellite number to satellite system
* args   : int    sat       I   satellite number (1-MAXSAT)
*          int    *prn      IO  satellite prn/slot number (NULL: no output)
* return : satellite system (SYS_GPS,SYS_GLO,...)
*-----------------------------------------------------------------------------*/
int HASPPP::satsys(int sat, int *prn)
{
    int sys=SYS_NONE;
    if (sat<=0||MAXSAT<sat) sat=0;
    else if (sat<=NSATGPS) {
        sys=SYS_GPS; sat+=MINPRNGPS-1;
    }
    else if ((sat-=NSATGPS)<=NSATGLO) {
        sys=SYS_GLO; sat+=MINPRNGLO-1;
    }
    else if ((sat-=NSATGLO)<=NSATGAL) {
        sys=SYS_GAL; sat+=MINPRNGAL-1;
    }
    else if ((sat-=NSATGAL)<=NSATQZS) {
        sys=SYS_QZS; sat+=MINPRNQZS-1;
    }
    else if ((sat-=NSATQZS)<=NSATCMP) {
        sys=SYS_CMP; sat+=MINPRNCMP-1;
    }
    else if ((sat-=NSATCMP)<=NSATIRN) {
        sys=SYS_IRN; sat+=MINPRNIRN-1;
    }
    else if ((sat-=NSATIRN)<=NSATLEO) {
        sys=SYS_LEO; sat+=MINPRNLEO-1;
    }
    else if ((sat-=NSATLEO)<=NSATSBS) {
        sys=SYS_SBS; sat+=MINPRNSBS-1;
    }
    else sat=0;
    if (prn) *prn=sat;
    return sys;
}

/* obs code to obs code string -------------------------------------------------
* convert obs code to obs code string
* args   : uint8_t code     I   obs code (CODE_???)
* return : obs code string ("1C","1P","1P",...)
* notes  : obs codes are based on RINEX 3.04
*-----------------------------------------------------------------------------*/
char * HASPPP::code2obs(uint8_t code)
{
    if (code<=CODE_NONE||MAXCODE<code) return "";
    return obscodes[code];
}

/* time to calendar day/time ---------------------------------------------------
* convert gtime_t struct to calendar day/time
* args   : gtime_t t        I   gtime_t struct
*          double *ep       O   day/time {year,month,day,hour,min,sec}
* return : none
* notes  : proper in 1970-2037 or 1970-2099 (64bit time_t)
*-----------------------------------------------------------------------------*/
void HASPPP::time2epoch(gtime_t t, double *ep)
{
    const int mday[]={ /* # of days in a month */
        31,28,31,30,31,30,31,31,30,31,30,31,31,28,31,30,31,30,31,31,30,31,30,31,
        31,29,31,30,31,30,31,31,30,31,30,31,31,28,31,30,31,30,31,31,30,31,30,31
    };
    int days,sec,mon,day;

    /* leap year if year%4==0 in 1901-2099 */
    days=(int)(t.time/86400);
    sec=(int)(t.time-(time_t)days*86400);
    for (day=days%1461,mon=0;mon<48;mon++) {
        if (day>=mday[mon]) day-=mday[mon]; else break;
    }
    ep[0]=1970+days/1461*4+mon/12; ep[1]=mon%12+1; ep[2]=day+1;
    ep[3]=sec/3600; ep[4]=sec%3600/60; ep[5]=sec%60+t.sec;
}

/* obs type string to obs code -------------------------------------------------
* convert obs code type string to obs code
* args   : char   *str      I   obs code string ("1C","1P","1Y",...)
* return : obs code (CODE_???)
* notes  : obs codes are based on RINEX 3.04
*-----------------------------------------------------------------------------*/
uint8_t HASPPP::obs2code(const char *obs)
{
    int i;

    for (i=1;*obscodes[i];i++) {
        if (strcmp(obscodes[i],obs)) continue;
        return (uint8_t)i;
    }
    return CODE_NONE;
}
