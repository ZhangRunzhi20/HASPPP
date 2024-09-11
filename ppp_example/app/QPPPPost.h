#ifndef QPPPPOST_H
#define QPPPPOST_H
#include "./rtklib.h"
#include "QStringList"
#include "HASPPP.h"


#define MIN(x,y)    ((x)<(y)?(x):(y))
#define SQRT(x)     ((x)<=0.0||(x)!=(x)?0.0:sqrt(x))

#define MAXPRCDAYS  100          /* max days of continuous processing */
#define MAXINFILE   1000         /* max number of input files */





class QPPPPost
{
public:
    QPPPPost();
    ~QPPPPost();
    int postpos(gtime_t ts, gtime_t te, double ti, double tu,
                       const prcopt_t *popt, const solopt_t *sopt,
                       const filopt_t *fopt, char **infile, int n, char *outfile,
                       const char *rov, const char *base);
private:
    int openses(const prcopt_t *popt, const solopt_t *sopt,
                       const filopt_t *fopt, nav_t *nav, pcvs_t *pcvs, pcvs_t *pcvr);
    void closeses(nav_t *nav, pcvs_t *pcvs, pcvs_t *pcvr);
    int execses_b(gtime_t ts, gtime_t te, double ti, const prcopt_t *popt,
                         const solopt_t *sopt, const filopt_t *fopt, int flag,
                         char **infile, const int *index, int n, char *outfile,
                         const char *rov, const char *base);
    int execses_r(gtime_t ts, gtime_t te, double ti, const prcopt_t *popt,
                         const solopt_t *sopt, const filopt_t *fopt, int flag,
                         char **infile, const int *index, int n, char *outfile,
                         const char *rov);
    int execses(gtime_t ts, gtime_t te, double ti, const prcopt_t *popt,
                       const solopt_t *sopt, const filopt_t *fopt, int flag,
                       char **infile, const int *index, int n, char *outfile);
    void readpreceph(char **infile, int n, const prcopt_t *prcopt,
                            nav_t *nav, sbs_t *sbs);

    void freepreceph(nav_t *nav, sbs_t *sbs);
    int readobsnav(gtime_t ts, gtime_t te, double ti, char **infile,
                          const int *index, int n, const prcopt_t *prcopt,
                          obs_t *obs, nav_t *nav, sta_t *sta);
    void setpcv(gtime_t time, prcopt_t *popt, nav_t *nav, const pcvs_t *pcvs,
                       const pcvs_t *pcvr, const sta_t *sta);

    void readotl(prcopt_t *popt, const char *file, const sta_t *sta);
    void freeobsnav(obs_t *obs, nav_t *nav);
    int antpos(prcopt_t *opt, int rcvno, const obs_t *obs, const nav_t *nav,
                      const sta_t *sta, const char *posfile);
    int outhead(const char *outfile, char **infile, int n,
                       const prcopt_t *popt, const solopt_t *sopt);
    void outheader(FILE *fp, char **file, int n, const prcopt_t *popt,
                          const solopt_t *sopt);
    void outrpos(FILE *fp, const double *r, const solopt_t *opt);
    FILE *openfile(const char *outfile);
    void procpos(FILE *fp, const prcopt_t *popt, const solopt_t *sopt,
                        int mode);

    void combres(FILE *fp, const prcopt_t *popt, const solopt_t *sopt);
    int valcomb(const sol_t *solf, const sol_t *solb);
    int inputobs(obsd_t *obs, int solq, const prcopt_t *popt);
    int nextobsf(const obs_t *obs, int *i, int rcv);
    int nextobsb(const obs_t *obs, int *i, int rcv);
    void update_rtcm_ssr(gtime_t time);
    void corr_phase_bias_ssr(obsd_t *obs, int n, const nav_t *nav);
    int  avepos(double *ra, int rcv, const obs_t *obs, const nav_t *nav,
                      const prcopt_t *opt);
    int getstapos(const char *file, char *name, double *r);


    int checkbrk(const char *format, ...);


private:
    nav_t  m_navs;
    pcvs_t m_pcvss;
    pcvs_t m_pcvsr;
    obs_t  m_obss;
    sbs_t  m_sbss;
    sta_t  m_stas[MAXRCV];      /* station infomation */
    int    m_nepoch;            /* number of observation epochs */
    int    m_iobsu;            /* current rover observation data index */
    int    m_iobsr;            /* current reference observation data index */
    int    m_isbs;            /* current sbas message index */
    int    m_revs;            /* analysis direction (0:forward,1:backward) */
    int    m_aborts;            /* abort status */
    sol_t  *m_solf;             /* forward solutions */
    sol_t  *m_solb;             /* backward solutions */
    double *m_rbf;             /* forward base positions */
    double *m_rbb;             /* backward base positions */
    int    m_isolf;             /* current forward solutions index */
    int    m_isolb;             /* current backward solutions index */
    char   m_proc_rov [64];   /* rover for current processing */
    char   m_proc_base[64];   /* base station for current processing */
    char   m_rtcm_file[1024]; /* rtcm data file */
    char   m_rtcm_path[1024]; /* rtcm data path */
    rtcm_t m_rtcm;             /* rtcm control struct */
    FILE   *m_fp_rtcm;      /* rtcm data file pointer */

    HASPPP m_hasPPP;
};

#endif // QPPPPOST_H
