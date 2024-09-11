#include<QCoreApplication>
#include "HASPPP.h"
#include "postMain.h"
#include "iostream"
using namespace std;

char debugwork[1024];
int m_inputhasformat; //1:SINO;2:NovAtel; 3:Septentrio

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    if (argc<2){
        std::cout<<"error: input parameter "<<argc<<std::endl;
        return 0;
    }

//    WIN32_FIND_DATA file;
//    HANDLE h;
//    char dir[1024]="",*p;
//    char path[1024] = "G:\\HASPPP\\ppp_example\\data\\*.HAS";
////    char pathTemp[1024] = TEXT("G:\\HASPPP\\ppp_example\\data\\*.HAS");

////    if ((h=FindFirstFile(TEXT(path),&file))==INVALID_HANDLE_VALUE) {
////        return 1;
////    }

////    QString str = QString::fromUtf8(path);

//    // 将 QString 转换为 wchar_t*（Unicode 字符串）
////    const wchar_t *unicodeString = reinterpret_cast<const wchar_t *>(str.utf16());


//    int size = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
//    wchar_t unicodeString[size] ;

//    MultiByteToWideChar(CP_UTF8, 0, path, -1, unicodeString, size);

//    if ((h=FindFirstFile(unicodeString,&file))==INVALID_HANDLE_VALUE) {
//        return 1;
//    }

    memcpy(debugwork,argv[0],1024*sizeof(char));
    m_inputhasformat = atoi(argv[1]);
    //QHasPPP temp;
    //temp.testHASExa();
    //temp.testHASExa1();

    //bin test
//    double obs[6];
//    obs[0] = 2023;
//    obs[1] = 11;
//    obs[2] = 8;
//    obs[3] = 22;
//    obs[4] = 28;
//    obs[5] = 40.00;
//    QHasPPP temp;
//    QStringList paths;
//    paths.append(QString("/home/david/Documents/rtklib_b34_my/Gao/rtkPPPHas/data/20230223.23HAS"));
//    temp.setHasFile(paths);
//    temp.readHasBinFiles();
//    temp.updateHasCorrection(obs);

    postMain temp;
    temp.ExecProc();
    return 1;
   // QHasPPP has;
    //has.readHasFiles();

//    return a.exec();
}
