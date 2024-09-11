#include <QCoreApplication>
#include <iostream>
#include <string>
#include "../HASPPP.h"
#include <vector>

using namespace std;

void time2epoch(gtime_t t, double *ep)
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
void time2str(gtime_t t, char *s, int n)
{
    double ep[6];

    if (n<0) n=0; else if (n>12) n=12;
    if (1.0-t.sec<0.5/pow(10.0,n)) {t.time++; t.sec=0.0;};
    time2epoch(t,ep);
    sprintf(s,"%04.0f/%02.0f/%02.0f %02.0f:%02.0f:%0*.*f",ep[0],ep[1],ep[2],
            ep[3],ep[4],n<=0?2:n+3,n<=0?0:n,ep[5]);
}

char *time_str(gtime_t t, int n)
{
    static char buff[64];
    time2str(t,buff,n);
    return buff;
}

int main(int argc, char *argv[]) {

    QCoreApplication a(argc, argv);

    gtime_t ts;
    gtime_t te;
    char s[1024];
    double obsT[6];
    gtime_t obstime = { 0 };
    std::vector<std::string> fileHASlist;
    HASPPP HAS(argv[0]);
    int epoch = 0;

#ifdef PATHEXE
    /* SINO */
    //    char infile1[1024]="/home/david/Documents/QTGnss/HASPPP/data/sino/obs/HAS20230615.HAS";
    //    double times[6] = {2023,06,15,0,0,0};
    //    double timee[6] = {2023,06,16,0,0,0};

    /* Septentrio */
    //    char infile1[1024] = "/media/david/Passport/HASPPP/data/septentrio/obs/galcnav20231228_new.sbf.HAS";
    //    double times[6] = {2023,12,28,0,0,0};
    //    double timee[6] = {2023,12,29,0,0,0};

    /* NovAtel */
    char infile1[1024] = "/media/david/Passport/HASPPP/data/novatel/obs/has20230531.HAS";
    double times[6] = { 2023,5,31,0,0,0 };
    double timee[6] = { 2023,6,1,0,0,0 };

    fileHASlist.push_back(std::string(infile1));
    HAS.setHasFile(fileHASlist, 2);  //1:SINO receiver, 2:NovAtel receiver, 3:Septentrio
#else

    double timesinos[6] = { 2023,06,15,0,0,0 };
    double timesinoe[6] = { 2023,06,16,0,0,0 };

    double timeseps[6] = { 2023,12,28,0,0,0 };
    double timesepe[6] = { 2023,12,29,0,0,0 };

    double timenovs[6] = { 2023,5,31,0,0,0 };
    double timenove[6] = { 2023,6,1,0,0,0 };

    double times[6], timee[6];

    if (argc != 3) {
        std::cout << "error: argc parameter num: " << argc << std::endl;
        return 0;
    }

    switch (atoi(argv[2])) {
    case 1: memcpy(times, timesinos, 6 * sizeof(double)); \
            memcpy(timee, timesinoe, 6 * sizeof(double)); break;
    case 2: memcpy(times, timenovs, 6 * sizeof(double)); \
            memcpy(timee, timenove, 6 * sizeof(double)); break;
    case 3: memcpy(times, timeseps, 6 * sizeof(double)); \
            memcpy(timee, timesepe, 6 * sizeof(double)); break;
    default:return 0;
    }
    fileHASlist.push_back(std::string(argv[1]));
    HAS.setHasFile(fileHASlist, atoi(argv[2]));  //1:SINO receiver, 2:NovAtel receiver, 3:Septentrio
#endif
    HAS.readHasBinFiles();
    ts = HAS.epoch2time(times);
    te = HAS.epoch2time(timee);
    for (int i=ts.time;i+=30;){
        obstime.time = i;
        epoch++;
        HAS.time2epoch(obstime,obsT);
        HAS.updateHasCorrection(obsT);


        if (difftime(obstime.time,te.time)>0) {
            break;
        }
        cout << "obs: " << time_str(obstime,2) << std::endl;

        //        memcpy(m_navs.ssr,HAS.getSsr(),MAXSAT*sizeof(ssr_t));

    }
    return 1;

}




