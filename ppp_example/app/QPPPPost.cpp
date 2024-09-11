#include "QPPPPost.h"
extern char debugwork[1024];
extern int m_inputhasformat;

QPPPPost::QPPPPost():m_hasPPP(debugwork)
{
    memset(&m_navs,0,sizeof(nav_t));
    memset(&m_pcvsr,0,sizeof(pcvs_t));
    memset(&m_pcvss,0,sizeof(pcvs_t));
    memset(&m_obss,0,sizeof(obs_t));
    memset(&m_sbss,0,sizeof(sbs_t));
    memset(&m_stas,0,MAXRCV*sizeof(sta_t));

    m_nepoch = 0;
    m_iobsu = 0;
    m_iobsr = 0;
    m_isbs  = 0;
    m_revs  = 0;
    m_aborts = 0;

    m_solf = NULL;
    m_solb = NULL;
    m_rbf  = NULL;
    m_rbb  = NULL;
    m_isolf = 0;
    m_isolb = 0;


    memset(&m_proc_rov,0,sizeof(m_proc_rov));
    memset(&m_proc_base,0,sizeof(m_proc_base));
    memset(&m_rtcm_file,0,sizeof(m_rtcm_file));
    memset(&m_rtcm_path,0,sizeof(m_rtcm_path));
    memset(&m_rtcm,0,sizeof(rtcm_t));
    m_fp_rtcm = NULL;

}
QPPPPost::~QPPPPost(){
    freeobsnav(&m_obss,&m_navs);
    if(m_solf) free(m_solf);
    if(m_solb) free(m_solb);
    if(m_rbf)  free(m_rbf);
    if(m_rbb)  free(m_rbb);
    if(m_fp_rtcm) fclose(m_fp_rtcm);
    freepreceph(&m_navs,&m_sbss);
    closeses(&m_navs,&m_pcvss,&m_pcvsr);
}
int QPPPPost::checkbrk(const char *format, ...)
{
    va_list arg;
    char buff[1024],*p=buff;
    if (!*format) return showmsg("");
    va_start(arg,format);
    p+=vsprintf(p,format,arg);
    va_end(arg);
    if (*m_proc_rov&&*m_proc_base) sprintf(p," (%s-%s)",m_proc_rov,m_proc_base);
    else if (*m_proc_rov ) sprintf(p," (%s)",m_proc_rov );
    else if (*m_proc_base) sprintf(p," (%s)",m_proc_base);
    return showmsg(buff);
}
int QPPPPost::postpos(gtime_t ts, gtime_t te, double ti, double tu,
                   const prcopt_t *popt, const solopt_t *sopt,
                   const filopt_t *fopt, char **infile, int n, char *outfile,
                   const char *rov, const char *base)
{
    gtime_t tts,tte,ttte;
    double tunit,tss;
    int i,j,k,nf,stat=0,week,flag=1,index[MAXINFILE]={0};
    char *ifile[MAXINFILE],ofile[1024],*ext;

    trace(3,"postpos : ti=%.0f tu=%.0f n=%d outfile=%s\n",ti,tu,n,outfile);

    /* open processing session */
    if (!openses(popt,sopt,fopt,&m_navs,&m_pcvss,&m_pcvsr)) return -1;

    if (ts.time!=0&&te.time!=0&&tu>=0.0) {
        if (timediff(te,ts)<0.0) {
            showmsg("error : no period");
            closeses(&m_navs,&m_pcvss,&m_pcvsr);
            return 0;
        }
        for (i=0;i<MAXINFILE;i++) {
            if (!(ifile[i]=(char *)malloc(1024))) {
                for (;i>=0;i--) free(ifile[i]);
                closeses(&m_navs,&m_pcvss,&m_pcvsr);
                return -1;
            }
        }
        if (tu==0.0||tu>86400.0*MAXPRCDAYS) tu=86400.0*MAXPRCDAYS;
        //settspan(ts,te);
        tunit=tu<86400.0?tu:86400.0;
        tss=tunit*(int)floor(time2gpst(ts,&week)/tunit);

        for (i=0;;i++) { /* for each periods */
            tts=gpst2time(week,tss+i*tu);
            tte=timeadd(tts,tu-DTTOL);
            if (timediff(tts,te)>0.0) break;
            if (timediff(tts,ts)<0.0) tts=ts;
            if (timediff(tte,te)>0.0) tte=te;

            strcpy(m_proc_rov ,"");
            strcpy(m_proc_base,"");
            if (checkbrk("reading    : %s",time_str(tts,0))) {
                stat=1;
                break;
            }
            for (j=k=nf=0;j<n;j++) {

                ext=strrchr(infile[j],'.');

                if (ext&&(!strcmp(ext,".rtcm3")||!strcmp(ext,".RTCM3"))) {
                    strcpy(ifile[nf++],infile[j]);
                }
                else {
                    /* include next day precise ephemeris or rinex brdc nav */
                    ttte=tte;
                    if (ext&&(!strcmp(ext,".sp3")||!strcmp(ext,".SP3")||
                              !strcmp(ext,".eph")||!strcmp(ext,".EPH"))) {
                        ttte=timeadd(ttte,3600.0);
                    }
                    else if (strstr(infile[j],"brdc")) {
                        ttte=timeadd(ttte,7200.0);
                    }
                    nf+=reppaths(infile[j],ifile+nf,MAXINFILE-nf,tts,ttte,"","");
                }
                while (k<nf) index[k++]=j;

                if (nf>=MAXINFILE) {
                    trace(2,"too many input files. trancated\n");
                    break;
                }
            }
            if (!reppath(outfile,ofile,tts,"","")&&i>0) flag=0;

            /* execute processing session */
            stat=execses_b(tts,tte,ti,popt,sopt,fopt,flag,ifile,index,nf,ofile,
                           rov,base);

            if (stat==1) break;
        }
        for (i=0;i<MAXINFILE;i++) free(ifile[i]);
    }
    else if (ts.time!=0) {
        for (i=0;i<n&&i<MAXINFILE;i++) {
            if (!(ifile[i]=(char *)malloc(1024))) {
                for (;i>=0;i--) free(ifile[i]);
                return -1;
            }
            reppath(infile[i],ifile[i],ts,"","");
            index[i]=i;
        }
        reppath(outfile,ofile,ts,"","");

        /* execute processing session */
        stat=execses_b(ts,te,ti,popt,sopt,fopt,1,ifile,index,n,ofile,rov,
                       base);

        for (i=0;i<n&&i<MAXINFILE;i++) free(ifile[i]);
    }
    else {
        for (i=0;i<n;i++) index[i]=i;

        /* execute processing session */
        stat=execses_b(ts,te,ti,popt,sopt,fopt,1,infile,index,n,outfile,rov,
                       base);
    }
    /* close processing session */
    closeses(&m_navs,&m_pcvss,&m_pcvsr);

    return stat;
}
void QPPPPost::readpreceph(char **infile, int n, const prcopt_t *prcopt,
                        nav_t *nav, sbs_t *sbs)
{
    seph_t seph0={0};
    int i,j;
    char *ext;
    int nHas=0;
    char *files[MAXEXFILE];
    QStringList fileQstringlist;
    std::vector <std::string>fileHASlist;

    trace(2,"readpreceph: n=%d\n",n);

    nav->ne=nav->nemax=0;
    nav->nc=nav->ncmax=0;
    sbs->n =sbs->nmax =0;

    /* read precise ephemeris files */
    for (i=0;i<n;i++) {
        if (strstr(infile[i],"%r")||strstr(infile[i],"%b")) continue;
        readsp3(infile[i],nav,0);
    }
    /* read precise clock files */
    for (i=0;i<n;i++) {
        if (strstr(infile[i],"%r")||strstr(infile[i],"%b")) continue;
        readrnxc(infile[i],nav);
    }
    /* read sbas message files */
    for (i=0;i<n;i++) {
        if (strstr(infile[i],"%r")||strstr(infile[i],"%b")) continue;
        sbsreadmsg(infile[i],prcopt->sbassatsel,sbs);
    }

    /* debug by zhangrunzhi */
    for (i=0;i<MAXEXFILE;i++) {
        if (!(files[i]=(char *)malloc(1024))) {
            for (i--;i>=0;i--) free(files[i]);
            return;
        }
    }

    for(i =0;i<n;i++){
        if(!strstr(infile[i],".HAS"))
            continue;

        nHas=expath(infile[i],files,MAXEXFILE);
        for(j=0;j<nHas;j++){
            fileQstringlist.append(files[j]);
            fileHASlist.push_back(std::string(files[j]));
        }


        m_hasPPP.setHasFile(fileHASlist,m_inputhasformat);
        //m_hasPPP.readHasFiles();
        m_hasPPP.readHasBinFiles();
    }
    /* allocate sbas ephemeris */
    nav->ns=nav->nsmax=NSATSBS*2;
    if (!(nav->seph=(seph_t *)malloc(sizeof(seph_t)*nav->ns))) {
         showmsg("error : sbas ephem memory allocation");
         trace(1,"error : sbas ephem memory allocation");
         return;
    }
    for (i=0;i<nav->ns;i++) nav->seph[i]=seph0;

    /* set rtcm file and initialize rtcm struct */
    m_rtcm_file[0]=m_rtcm_path[0]='\0'; m_fp_rtcm=NULL;

    for (i=0;i<n;i++) {
        if ((ext=strrchr(infile[i],'.'))&&
            (!strcmp(ext,".rtcm3")||!strcmp(ext,".RTCM3"))) {
            strcpy(m_rtcm_file,infile[i]);
            init_rtcm(&m_rtcm);
            break;
        }
    }

    for (i=0;i<MAXEXFILE;i++) free(files[i]);
}
void QPPPPost::freepreceph(nav_t *nav, sbs_t *sbs)
{
    int i;

    trace(3,"freepreceph:\n");

    free(nav->peph); nav->peph=NULL; nav->ne=nav->nemax=0;
    free(nav->pclk); nav->pclk=NULL; nav->nc=nav->ncmax=0;
    free(nav->seph); nav->seph=NULL; nav->ns=nav->nsmax=0;
    free(sbs->msgs); sbs->msgs=NULL; sbs->n =sbs->nmax =0;
    for (i=0;i<nav->nt;i++) {
        free(nav->tec[i].data);
        free(nav->tec[i].rms );
    }
    free(nav->tec ); nav->tec =NULL; nav->nt=nav->ntmax=0;

    if (m_fp_rtcm) fclose(m_fp_rtcm);
    free_rtcm(&m_rtcm);
}
int QPPPPost::execses_r(gtime_t ts, gtime_t te, double ti, const prcopt_t *popt,
                     const solopt_t *sopt, const filopt_t *fopt, int flag,
                     char **infile, const int *index, int n, char *outfile,
                     const char *rov)
{
    gtime_t t0={0};
    int i,stat=0;
    char *ifile[MAXINFILE],ofile[1024],*rov_,*p,*q,s[64]="";

    trace(3,"execses_r: n=%d outfile=%s\n",n,outfile);

    for (i=0;i<n;i++) if (strstr(infile[i],"%r")) break;

    if (i<n) { /* include rover keywords */
        if (!(rov_=(char *)malloc(strlen(rov)+1))) return 0;
        strcpy(rov_,rov);

        for (i=0;i<n;i++) {
            if (!(ifile[i]=(char *)malloc(1024))) {
                free(rov_); for (;i>=0;i--) free(ifile[i]);
                return 0;
            }
        }
        for (p=rov_;;p=q+1) { /* for each rover */
            if ((q=strchr(p,' '))) *q='\0';

            if (*p) {
                strcpy(m_proc_rov,p);
                if (ts.time) time2str(ts,s,0); else *s='\0';
                if (checkbrk("reading    : %s",s)) {
                    stat=1;
                    break;
                }
                for (i=0;i<n;i++) reppath(infile[i],ifile[i],t0,p,"");
                reppath(outfile,ofile,t0,p,"");

                /* execute processing session */
                stat=execses(ts,te,ti,popt,sopt,fopt,flag,ifile,index,n,ofile);
            }
            if (stat==1||!q) break;
        }
        free(rov_); for (i=0;i<n;i++) free(ifile[i]);
    }
    else {
        /* execute processing session */
        stat=execses(ts,te,ti,popt,sopt,fopt,flag,infile,index,n,outfile);
    }
    return stat;
}
int QPPPPost::readobsnav(gtime_t ts, gtime_t te, double ti, char **infile,
                      const int *index, int n, const prcopt_t *prcopt,
                      obs_t *obs, nav_t *nav, sta_t *sta)
{
    int i,j,ind=0,nobs=0,rcv=1;

    trace(3,"readobsnav: ts=%s n=%d\n",time_str(ts,0),n);

    obs->data=NULL; obs->n =obs->nmax =0;
    nav->eph =NULL; nav->n =nav->nmax =0;
    nav->geph=NULL; nav->ng=nav->ngmax=0;
    nav->seph=NULL; nav->ns=nav->nsmax=0;
    m_nepoch=0;

    for (i=0;i<n;i++) {
        if (checkbrk("")) return 0;

        if (index[i]!=ind) {
            if (obs->n>nobs) rcv++;
            ind=index[i]; nobs=obs->n;
        }
        /* read rinex obs and nav file */
        if (readrnxt(infile[i],rcv,ts,te,ti,prcopt->rnxopt[rcv<=1?0:1],obs,nav,
                     rcv<=2?sta+rcv-1:NULL)<0) {
            checkbrk("error : insufficient memory");
            trace(1,"insufficient memory\n");
            return 0;
        }
    }
    if (obs->n<=0) {
        checkbrk("error : no obs data");
        trace(1,"\n");
        return 0;
    }
    if (nav->n<=0&&nav->ng<=0&&nav->ns<=0) {
        checkbrk("error : no nav data");
        trace(1,"\n");
        return 0;
    }
    /* sort observation data */
    m_nepoch=sortobs(obs);

    /* delete duplicated ephemeris */
    uniqnav(nav);

    /* set time span for progress display */
   /* if (ts.time==0||te.time==0) {
        for (i=0;   i<obs->n;i++) if (obs->data[i].rcv==1) break;
        for (j=obs->n-1;j>=0;j--) if (obs->data[j].rcv==1) break;
        if (i<j) {
            if (ts.time==0) ts=obs->data[i].time;
            if (te.time==0) te=obs->data[j].time;
            settspan(ts,te);
        }
    }*/
    return 1;
}
void QPPPPost::setpcv(gtime_t time, prcopt_t *popt, nav_t *nav, const pcvs_t *pcvs,
                   const pcvs_t *pcvr, const sta_t *sta)
{
    pcv_t *pcv,pcv0={0};
    double pos[3],del[3];
    int i,j,mode=PMODE_DGPS<=popt->mode&&popt->mode<=PMODE_FIXED;
    char id[64];

    /* set satellite antenna parameters */
    for (i=0;i<MAXSAT;i++) {
        nav->pcvs[i]=pcv0;
        if (!(satsys(i+1,NULL)&popt->navsys)) continue;
        if (!(pcv=searchpcv(i+1,"",time,pcvs))) {
            satno2id(i+1,id);
            trace(3,"no satellite antenna pcv: %s\n",id);
            continue;
        }
        nav->pcvs[i]=*pcv;
    }
    for (i=0;i<(mode?2:1);i++) {
        popt->pcvr[i]=pcv0;
        if (!strcmp(popt->anttype[i],"*")) { /* set by station parameters */
            strcpy(popt->anttype[i],sta[i].antdes);
            if (sta[i].deltype==1) { /* xyz */
                if (norm(sta[i].pos,3)>0.0) {
                    ecef2pos(sta[i].pos,pos);
                    ecef2enu(pos,sta[i].del,del);
                    for (j=0;j<3;j++) popt->antdel[i][j]=del[j];
                }
            }
            else { /* enu */
                for (j=0;j<3;j++) popt->antdel[i][j]=m_stas[i].del[j];
            }
        }
        if (!(pcv=searchpcv(0,popt->anttype[i],time,pcvr))) {
            trace(2,"no receiver antenna pcv: %s\n",popt->anttype[i]);
            *popt->anttype[i]='\0';
            continue;
        }
        strcpy(popt->anttype[i],pcv->type);
        popt->pcvr[i]=*pcv;
    }
}
void QPPPPost::outheader(FILE *fp, char **file, int n, const prcopt_t *popt,
                      const solopt_t *sopt)
{
    const char *s1[]={"GPST","UTC","JST"};
    gtime_t ts,te;
    double t1,t2;
    int i,j,w1,w2;
    char s2[32],s3[32];

    trace(3,"outheader: n=%d\n",n);

    if (sopt->posf==SOLF_NMEA||sopt->posf==SOLF_STAT) {
        return;
    }
    if (sopt->outhead) {
        if (!*sopt->prog) {
            fprintf(fp,"%s program   : RTKLIB ver.%s\n",COMMENTH,VER_RTKLIB);
        }
        else {
            fprintf(fp,"%s program   : %s\n",COMMENTH,sopt->prog);
        }
        for (i=0;i<n;i++) {
            fprintf(fp,"%s inp file  : %s\n",COMMENTH,file[i]);
        }
        for (i=0;i<m_obss.n;i++)    if (m_obss.data[i].rcv==1) break;
        for (j=m_obss.n-1;j>=0;j--) if (m_obss.data[j].rcv==1) break;
        if (j<i) {fprintf(fp,"\n%s no rover obs data\n",COMMENTH); return;}
        ts=m_obss.data[i].time;
        te=m_obss.data[j].time;
        t1=time2gpst(ts,&w1);
        t2=time2gpst(te,&w2);
        if (sopt->times>=1) ts=gpst2utc(ts);
        if (sopt->times>=1) te=gpst2utc(te);
        if (sopt->times==2) ts=timeadd(ts,9*3600.0);
        if (sopt->times==2) te=timeadd(te,9*3600.0);
        time2str(ts,s2,1);
        time2str(te,s3,1);
        fprintf(fp,"%s obs start : %s %s (week%04d %8.1fs)\n",COMMENTH,s2,s1[sopt->times],w1,t1);
        fprintf(fp,"%s obs end   : %s %s (week%04d %8.1fs)\n",COMMENTH,s3,s1[sopt->times],w2,t2);
    }
    if (sopt->outopt) {
        outprcopt(fp,popt);
    }
    if (PMODE_DGPS<=popt->mode&&popt->mode<=PMODE_FIXED&&popt->mode!=PMODE_MOVEB) {
        fprintf(fp,"%s ref pos   :",COMMENTH);
        outrpos(fp,popt->rb,sopt);
        fprintf(fp,"\n");
    }
    if (sopt->outhead||sopt->outopt) fprintf(fp,"%s\n",COMMENTH);

    outsolhead(fp,sopt);
}
void QPPPPost::outrpos(FILE *fp, const double *r, const solopt_t *opt)
{
    double pos[3],dms1[3],dms2[3];
    const char *sep=opt->sep;

    trace(3,"outrpos :\n");

    if (opt->posf==SOLF_LLH||opt->posf==SOLF_ENU) {
        ecef2pos(r,pos);
        if (opt->degf) {
            deg2dms(pos[0]*R2D,dms1,5);
            deg2dms(pos[1]*R2D,dms2,5);
            fprintf(fp,"%3.0f%s%02.0f%s%08.5f%s%4.0f%s%02.0f%s%08.5f%s%10.4f",
                    dms1[0],sep,dms1[1],sep,dms1[2],sep,dms2[0],sep,dms2[1],
                    sep,dms2[2],sep,pos[2]);
        }
        else {
            fprintf(fp,"%13.9f%s%14.9f%s%10.4f",pos[0]*R2D,sep,pos[1]*R2D,
                    sep,pos[2]);
        }
    }
    else if (opt->posf==SOLF_XYZ) {
        fprintf(fp,"%14.4f%s%14.4f%s%14.4f",r[0],sep,r[1],sep,r[2]);
    }
}
int QPPPPost::outhead(const char *outfile, char **infile, int n,
                   const prcopt_t *popt, const solopt_t *sopt)
{
    FILE *fp=stdout;

    trace(3,"outhead: outfile=%s n=%d\n",outfile,n);

    if (*outfile) {
        createdir(outfile);

        if (!(fp=fopen(outfile,"wb"))) {
            showmsg("error : open output file %s",outfile);
            return 0;
        }
    }
    /* output header */
    outheader(fp,infile,n,popt,sopt);

    if (*outfile) fclose(fp);

    return 1;
}
FILE * QPPPPost::openfile(const char *outfile)
{
    trace(3,"openfile: outfile=%s\n",outfile);

    return !*outfile?stdout:fopen(outfile,"ab");
}
int QPPPPost::valcomb(const sol_t *solf, const sol_t *solb)
{
    double dr[3],var[3];
    int i;
    char tstr[32];

    trace(3,"valcomb :\n");

    /* compare forward and backward solution */
    for (i=0;i<3;i++) {
        dr[i]=solf->rr[i]-solb->rr[i];
        var[i]=solf->qr[i]+solb->qr[i];
    }
    for (i=0;i<3;i++) {
        if (dr[i]*dr[i]<=16.0*var[i]) continue; /* ok if in 4-sigma */

        time2str(solf->time,tstr,2);
        trace(2,"degrade fix to float: %s dr=%.3f %.3f %.3f std=%.3f %.3f %.3f\n",
              tstr+11,dr[0],dr[1],dr[2],SQRT(var[0]),SQRT(var[1]),SQRT(var[2]));
        return 0;
    }
    return 1;
}
void QPPPPost::combres(FILE *fp, const prcopt_t *popt, const solopt_t *sopt)
{
    gtime_t time={0};
    sol_t sols={{0}},sol={{0}};
    double tt,Qf[9],Qb[9],Qs[9],rbs[3]={0},rb[3]={0},rr_f[3],rr_b[3],rr_s[3];
    int i,j,k,solstatic,pri[]={0,1,2,3,4,5,1,6};

    trace(3,"combres : isolf=%d isolb=%d\n",m_isolf,m_isolb);

    solstatic=sopt->solstatic&&
              (popt->mode==PMODE_STATIC||popt->mode==PMODE_PPP_STATIC);

    for (i=0,j=m_isolb-1;i<m_isolf&&j>=0;i++,j--) {

        if ((tt=timediff(m_solf[i].time,m_solb[j].time))<-DTTOL) {
            sols=m_solf[i];
            for (k=0;k<3;k++) rbs[k]=m_rbf[k+i*3];
            j++;
        }
        else if (tt>DTTOL) {
            sols=m_solb[j];
            for (k=0;k<3;k++) rbs[k]=m_rbb[k+j*3];
            i--;
        }
        else if (m_solf[i].stat<m_solb[j].stat) {
            sols=m_solf[i];
            for (k=0;k<3;k++) rbs[k]=m_rbf[k+i*3];
        }
        else if (m_solf[i].stat>m_solb[j].stat) {
            sols=m_solb[j];
            for (k=0;k<3;k++) rbs[k]=m_rbb[k+j*3];
        }
        else {
            sols=m_solf[i];
            sols.time=timeadd(sols.time,-tt/2.0);

            if ((popt->mode==PMODE_KINEMA||popt->mode==PMODE_MOVEB)&&
                sols.stat==SOLQ_FIX) {

                /* degrade fix to float if validation failed */
                if (!valcomb(m_solf+i,m_solb+j)) sols.stat=SOLQ_FLOAT;
            }
            for (k=0;k<3;k++) {
                Qf[k+k*3]=m_solf[i].qr[k];
                Qb[k+k*3]=m_solb[j].qr[k];
            }
            Qf[1]=Qf[3]=m_solf[i].qr[3];
            Qf[5]=Qf[7]=m_solf[i].qr[4];
            Qf[2]=Qf[6]=m_solf[i].qr[5];
            Qb[1]=Qb[3]=m_solb[j].qr[3];
            Qb[5]=Qb[7]=m_solb[j].qr[4];
            Qb[2]=Qb[6]=m_solb[j].qr[5];

            if (popt->mode==PMODE_MOVEB) {
                for (k=0;k<3;k++) rr_f[k]=m_solf[i].rr[k]-m_rbf[k+i*3];
                for (k=0;k<3;k++) rr_b[k]=m_solb[j].rr[k]-m_rbb[k+j*3];
                if (smoother(rr_f,Qf,rr_b,Qb,3,rr_s,Qs)) continue;
                for (k=0;k<3;k++) sols.rr[k]=rbs[k]+rr_s[k];
            }
            else {
                if (smoother(m_solf[i].rr,Qf,m_solb[j].rr,Qb,3,sols.rr,Qs)) continue;
            }
            sols.qr[0]=(float)Qs[0];
            sols.qr[1]=(float)Qs[4];
            sols.qr[2]=(float)Qs[8];
            sols.qr[3]=(float)Qs[1];
            sols.qr[4]=(float)Qs[5];
            sols.qr[5]=(float)Qs[2];

            /* smoother for velocity solution */
            if (popt->dynamics) {
                for (k=0;k<3;k++) {
                    Qf[k+k*3]=m_solf[i].qv[k];
                    Qb[k+k*3]=m_solb[j].qv[k];
                }
                Qf[1]=Qf[3]=m_solf[i].qv[3];
                Qf[5]=Qf[7]=m_solf[i].qv[4];
                Qf[2]=Qf[6]=m_solf[i].qv[5];
                Qb[1]=Qb[3]=m_solb[j].qv[3];
                Qb[5]=Qb[7]=m_solb[j].qv[4];
                Qb[2]=Qb[6]=m_solb[j].qv[5];
                if (smoother(m_solf[i].rr+3,Qf,m_solb[j].rr+3,Qb,3,sols.rr+3,Qs)) continue;
                sols.qv[0]=(float)Qs[0];
                sols.qv[1]=(float)Qs[4];
                sols.qv[2]=(float)Qs[8];
                sols.qv[3]=(float)Qs[1];
                sols.qv[4]=(float)Qs[5];
                sols.qv[5]=(float)Qs[2];
            }
        }
        if (!solstatic) {
            outsol(fp,&sols,rbs,sopt);
        }
        else if (time.time==0||pri[sols.stat]<=pri[sol.stat]) {
            sol=sols;
            for (k=0;k<3;k++) rb[k]=rbs[k];
            if (time.time==0||timediff(sols.time,time)<0.0) {
                time=sols.time;
            }
        }
    }
    if (solstatic&&time.time!=0.0) {
        sol.time=time;
        outsol(fp,&sol,rb,sopt);
    }
}
int QPPPPost::execses(gtime_t ts, gtime_t te, double ti, const prcopt_t *popt,
                   const solopt_t *sopt, const filopt_t *fopt, int flag,
                   char **infile, const int *index, int n, char *outfile)
{
    FILE *fp;
    prcopt_t popt_=*popt;
    char tracefile[1024],statfile[1024],path[1024],*ext;

    trace(3,"execses : n=%d outfile=%s\n",n,outfile);

    /* open debug trace */
    if (flag&&sopt->trace>0) {
        if (*outfile) {
            strcpy(tracefile,outfile);
            strcat(tracefile,".trace");
        }
        else {
            strcpy(tracefile,fopt->trace);
        }
        traceclose();
        traceopen(tracefile);
        tracelevel(sopt->trace);
    }
    /* read ionosphere data file */
    /* debug by zhangrunzhi*/
    /*if (*fopt->iono&&(ext=strrchr(fopt->iono,'.'))) {
        if (strlen(ext)==4&&(ext[3]=='i'||ext[3]=='I')) {
            reppath(fopt->iono,path,ts,"","");
            readtec(path,&m_navs,1);
        }
    }*/
    /* read erp data */
    if (*fopt->eop) {
        free(m_navs.erp.data); m_navs.erp.data=NULL; m_navs.erp.n=m_navs.erp.nmax=0;
        reppath(fopt->eop,path,ts,"","");
        if (!readerp(path,&m_navs.erp)) {
            showmsg("error : no erp data %s",path);
            trace(2,"no erp data %s\n",path);
        }
    }
    /* read obs and nav data */
    if (!readobsnav(ts,te,ti,infile,index,n,&popt_,&m_obss,&m_navs,m_stas)) return 0;
    /* debug by zhangrunzhi */
    //memcpy(popt_.ru,m_stas[0].pos,3*sizeof(double));
//    memcpy(popt_.anttype,m_stas[0].antdes,MAXANT*sizeof(char));



    /* read dcb parameters */
    if (*fopt->dcb) {
        reppath(fopt->dcb,path,ts,"","");
        readdcb(path,&m_navs,m_stas);
    }
    /* set antenna paramters */
    if (popt_.mode!=PMODE_SINGLE) {
        setpcv(m_obss.n>0?m_obss.data[0].time:timeget(),&popt_,&m_navs,&m_pcvss,&m_pcvsr,
               m_stas);
    }
    /* read ocean tide loading parameters */
    if (popt_.mode>PMODE_SINGLE&&*fopt->blq) {
        readotl(&popt_,fopt->blq,m_stas);
    }
    /* rover/reference fixed position */
    if (popt_.mode==PMODE_FIXED) {
        if (!antpos(&popt_,1,&m_obss,&m_navs,m_stas,fopt->stapos)) {
            freeobsnav(&m_obss,&m_navs);
            return 0;
        }
    }
    else if (PMODE_DGPS<=popt_.mode&&popt_.mode<=PMODE_STATIC) {
        if (!antpos(&popt_,2,&m_obss,&m_navs,m_stas,fopt->stapos)) {
            freeobsnav(&m_obss,&m_navs);
            return 0;
        }
    }
    /* open solution statistics */
    if (flag&&sopt->sstat>0) {
        strcpy(statfile,outfile);
        strcat(statfile,".stat");
        rtkclosestat();
        rtkopenstat(statfile,sopt->sstat);
    }
    /* write header to output file */
    if (flag&&!outhead(outfile,infile,n,&popt_,sopt)) {
        freeobsnav(&m_obss,&m_navs);
        return 0;
    }
    m_iobsu=m_iobsr=m_isbs=m_revs=m_aborts=0;

    if (popt_.mode==PMODE_SINGLE||popt_.soltype==0) {
        if ((fp=openfile(outfile))) {
            procpos(fp,&popt_,sopt,0); /* forward */
            fclose(fp);
        }
    }
    else if (popt_.soltype==1) {
        if ((fp=openfile(outfile))) {
            m_revs=1; m_iobsu=m_iobsr=m_obss.n-1; m_isbs=m_sbss.n-1;
            procpos(fp,&popt_,sopt,0); /* backward */
            fclose(fp);
        }
    }
    else { /* combined */
        m_solf=(sol_t *)malloc(sizeof(sol_t)*m_nepoch);
        m_solb=(sol_t *)malloc(sizeof(sol_t)*m_nepoch);
        m_rbf=(double *)malloc(sizeof(double)*m_nepoch*3);
        m_rbb=(double *)malloc(sizeof(double)*m_nepoch*3);

        if (m_solf&&m_solb) {
            m_isolf=m_isolb=0;
            procpos(NULL,&popt_,sopt,1); /* forward */
            m_revs=1; m_iobsu=m_iobsr=m_obss.n-1; m_isbs=m_sbss.n-1;
            procpos(NULL,&popt_,sopt,1); /* backward */

            /* combine forward/backward solutions */
            if (!m_aborts&&(fp=openfile(outfile))) {
                combres(fp,&popt_,sopt);
                fclose(fp);
            }
        }
        else showmsg("error : memory allocation");
        free(m_solf);
        free(m_solb);
        free(m_rbf);
        free(m_rbb);
    }
    /* free obs and nav data */
    freeobsnav(&m_obss,&m_navs);

    return m_aborts?1:0;
}
int QPPPPost::nextobsf(const obs_t *obs, int *i, int rcv)
{
    double tt;
    int n;

    for (;*i<obs->n;(*i)++) if (obs->data[*i].rcv==rcv) break;
    for (n=0;*i+n<obs->n;n++) {
        tt=timediff(obs->data[*i+n].time,obs->data[*i].time);
        if (obs->data[*i+n].rcv!=rcv||tt>DTTOL) break;
    }
    return n;
}
int QPPPPost::nextobsb(const obs_t *obs, int *i, int rcv)
{
    double tt;
    int n;

    for (;*i>=0;(*i)--) if (obs->data[*i].rcv==rcv) break;
    for (n=0;*i-n>=0;n++) {
        tt=timediff(obs->data[*i-n].time,obs->data[*i].time);
        if (obs->data[*i-n].rcv!=rcv||tt<-DTTOL) break;
    }
    return n;
}
int QPPPPost::inputobs(obsd_t *obs, int solq, const prcopt_t *popt)
{
    gtime_t time={0};
    int i,nu,nr,n=0;

    trace(3,"infunc  : revs=%d iobsu=%d iobsr=%d isbs=%d\n",m_revs,m_iobsu,m_iobsr,m_isbs);

    if (0<=m_iobsu&&m_iobsu<m_obss.n) {
       // settime((time=obss.data[iobsu].time));
        if (checkbrk("processing : %s Q=%d",time_str(time,0),solq)) {
            m_aborts=1; showmsg("aborted"); return -1;
        }
    }
    if (!m_revs) { /* input forward data */
        if ((nu=nextobsf(&m_obss,&m_iobsu,1))<=0) return -1;
        if (popt->intpref) {
            for (;(nr=nextobsf(&m_obss,&m_iobsr,2))>0;m_iobsr+=nr)
                if (timediff(m_obss.data[m_iobsr].time,m_obss.data[m_iobsu].time)>-DTTOL) break;
        }
        else {
            for (i=m_iobsr;(nr=nextobsf(&m_obss,&i,2))>0;m_iobsr=i,i+=nr)
                if (timediff(m_obss.data[i].time,m_obss.data[m_iobsu].time)>DTTOL) break;
        }
        nr=nextobsf(&m_obss,&m_iobsr,2);
        if (nr<=0) {
            nr=nextobsf(&m_obss,&m_iobsr,2);
        }
        for (i=0;i<nu&&n<MAXOBS*2;i++) obs[n++]=m_obss.data[m_iobsu+i];
        for (i=0;i<nr&&n<MAXOBS*2;i++) obs[n++]=m_obss.data[m_iobsr+i];
        m_iobsu+=nu;

        /* update sbas corrections */
        while (m_isbs<m_sbss.n) {
            time=gpst2time(m_sbss.msgs[m_isbs].week,m_sbss.msgs[m_isbs].tow);

            if (getbitu(m_sbss.msgs[m_isbs].msg,8,6)!=9) { /* except for geo nav */
                sbsupdatecorr(m_sbss.msgs+m_isbs,&m_navs);
            }
            if (timediff(time,obs[0].time)>-1.0-DTTOL) break;
            m_isbs++;
        }
        /* update rtcm ssr corrections */
        if (*m_rtcm_file) {
            update_rtcm_ssr(obs[0].time);
        }
    }
    else { /* input backward data */
        if ((nu=nextobsb(&m_obss,&m_iobsu,1))<=0) return -1;
        if (popt->intpref) {
            for (;(nr=nextobsb(&m_obss,&m_iobsr,2))>0;m_iobsr-=nr)
                if (timediff(m_obss.data[m_iobsr].time,m_obss.data[m_iobsu].time)<DTTOL) break;
        }
        else {
            for (i=m_iobsr;(nr=nextobsb(&m_obss,&i,2))>0;m_iobsr=i,i-=nr)
                if (timediff(m_obss.data[i].time,m_obss.data[m_iobsu].time)<-DTTOL) break;
        }
        nr=nextobsb(&m_obss,&m_iobsr,2);
        for (i=0;i<nu&&n<MAXOBS*2;i++) obs[n++]=m_obss.data[m_iobsu-nu+1+i];
        for (i=0;i<nr&&n<MAXOBS*2;i++) obs[n++]=m_obss.data[m_iobsr-nr+1+i];
        m_iobsu-=nu;

        /* update sbas corrections */
        while (m_isbs>=0) {
            time=gpst2time(m_sbss.msgs[m_isbs].week,m_sbss.msgs[m_isbs].tow);

            if (getbitu(m_sbss.msgs[m_isbs].msg,8,6)!=9) { /* except for geo nav */
                sbsupdatecorr(m_sbss.msgs+m_isbs,&m_navs);
            }
            if (timediff(time,obs[0].time)<1.0+DTTOL) break;
            m_isbs--;
        }
    }
    return n;
}
void QPPPPost::update_rtcm_ssr(gtime_t time)
{
    char path[1024];
    int i;

    /* open or swap rtcm file */
    reppath(m_rtcm_file,path,time,"","");

    if (strcmp(path,m_rtcm_path)) {
        strcpy(m_rtcm_path,path);

        if (m_fp_rtcm) fclose(m_fp_rtcm);
        m_fp_rtcm=fopen(path,"rb");
        if (m_fp_rtcm) {
            m_rtcm.time=time;
            input_rtcm3f(&m_rtcm,m_fp_rtcm);
            trace(2,"rtcm file open: %s\n",path);
        }
    }
    if (!m_fp_rtcm) return;

    /* read rtcm file until current time */
    while (timediff(m_rtcm.time,time)<1E-3) {
        if (input_rtcm3f(&m_rtcm,m_fp_rtcm)<-1) break;

        /* update ssr corrections */
        for (i=0;i<MAXSAT;i++) {
            if (!m_rtcm.ssr[i].update||
                m_rtcm.ssr[i].iod[0]!=m_rtcm.ssr[i].iod[1]||
                timediff(time,m_rtcm.ssr[i].t0[0])<-1E-3) continue;
            m_navs.ssr[i]=m_rtcm.ssr[i];
            m_rtcm.ssr[i].update=0;
        }
    }
}
void QPPPPost::corr_phase_bias_ssr(obsd_t *obs, int n, const nav_t *nav)
{
    double freq;
    uint8_t code;
    int i,j;

    for (i=0;i<n;i++) for (j=0;j<NFREQ;j++) {
        code=obs[i].code[j];

        if ((freq=sat2freq(obs[i].sat,code,nav))==0.0) continue;

        /* correct phase bias (cyc) */
        obs[i].L[j]-=nav->ssr[obs[i].sat-1].pbias[code-1]*freq/CLIGHT;
    }
}
void QPPPPost::procpos(FILE *fp, const prcopt_t *popt, const solopt_t *sopt,
                    int mode)
{
    gtime_t time={0};
    sol_t sol={{0}};
    rtk_t rtk;
    obsd_t obs[MAXOBS*2]; /* for rover and base */
    double rb[3]={0};
    int i,nobs,n,solstatic,pri[]={6,1,2,3,4,5,1,6};
    double obsStr[6]= {0};
    double tempObs=0;
    double timeEpoch[6];


    trace(3,"procpos : mode=%d\n",mode);

    solstatic=sopt->solstatic&&
              (popt->mode==PMODE_STATIC||popt->mode==PMODE_PPP_STATIC);

    rtkinit(&rtk,popt);
    m_rtcm_path[0]='\0';
    /*debug by zhangrunzhi*/
    memcpy(rtk.sol.rr,rtk.opt.ru,3*sizeof(double));


    while ((nobs=inputobs(obs,rtk.sol.stat,popt))>=0) {

        /* exclude satellites */
        for (i=n=0;i<nobs;i++) {
            if ((satsys(obs[i].sat,NULL)&popt->navsys)&&
                popt->exsats[obs[i].sat-1]!=1) obs[n++]=obs[i];
        }
        if (n<=0) continue;
        time2epoch(obs[0].time,obsStr);
        m_hasPPP.updateHasCorrection(obsStr);
        memcpy(m_navs.ssr,m_hasPPP.getSsr(),MAXSAT*sizeof(ssr_t));
        /* carrier-phase bias correction */
        if (!strstr(popt->pppopt,"-ENA_FCB")) {
            corr_phase_bias_ssr(obs,n,&m_navs);
        }
        if(obsStr[3]==12&&obsStr[4]==59&&obsStr[30]==0){
            tempObs = 0;
        }
        time2epoch(obs[0].time,timeEpoch);
        //if(((int)timeEpoch[3]%4)==0&&timeEpoch[4]==0&&timeEpoch[5]==0){
        //    rtkinit(&rtk,popt);
         //   memcpy(rtk.sol.rr,rtk.opt.ru,3*sizeof(double));

        //}
        if((int)timeEpoch[3]==7&& (int)timeEpoch[4]==35&& (int)timeEpoch[5]==30){
            tempObs = 0;

        }
        if (!rtkpos(&rtk,obs,n,&m_navs)) continue;

        if (mode==0) { /* forward/backward */
            if (!solstatic) {
                outsol(fp,&rtk.sol,rtk.rb,sopt);
                if(rtk.sol.stat==6){
                    trace(1,"#Clk,%s,%3d,%10.10f,%10.10f,%10.10f,%10.10f,\n",
                          time_str(obs[0].time,2),rtk.sol.stat,rtk.x[3],rtk.x[4],rtk.x[5],rtk.x[6]);
//                    printf("#Clk,%s,%3d,%10.10f,%10.10f,%10.10f,%10.10f,\n",
//                          time_str(obs[0].time,2),rtk.sol.stat,rtk.x[3],rtk.x[4],rtk.x[5],rtk.x[6]);
                }
            }
            else if (time.time==0||pri[rtk.sol.stat]<=pri[sol.stat]) {
                sol=rtk.sol;
                for (i=0;i<3;i++) rb[i]=rtk.rb[i];
                if (time.time==0||timediff(rtk.sol.time,time)<0.0) {
                    time=rtk.sol.time;
                }
            }
        }
        else if (!m_revs) { /* combined-forward */
            if (m_isolf>=m_nepoch) return;
            m_solf[m_isolf]=rtk.sol;
            for (i=0;i<3;i++) m_rbf[i+m_isolf*3]=rtk.rb[i];
            m_isolf++;
        }
        else { /* combined-backward */
            if (m_isolb>=m_nepoch) return;
            m_solb[m_isolb]=rtk.sol;
            for (i=0;i<3;i++) m_rbb[i+m_isolb*3]=rtk.rb[i];
            m_isolb++;
        }
    }
    if (mode==0&&solstatic&&time.time!=0.0) {
        sol.time=time;
        outsol(fp,&sol,rb,sopt);
    }
    rtkfree(&rtk);
}
int QPPPPost::avepos(double *ra, int rcv, const obs_t *obs, const nav_t *nav,
                  const prcopt_t *opt)
{
    obsd_t data[MAXOBS];
    gtime_t ts={0};
    sol_t sol={{0}};
    int i,j,n=0,m,iobs;
    char msg[128];

    trace(3,"avepos: rcv=%d obs.n=%d\n",rcv,obs->n);

    for (i=0;i<3;i++) ra[i]=0.0;

    for (iobs=0;(m=nextobsf(obs,&iobs,rcv))>0;iobs+=m) {

        for (i=j=0;i<m&&i<MAXOBS;i++) {
            data[j]=obs->data[iobs+i];
            if ((satsys(data[j].sat,NULL)&opt->navsys)&&
                opt->exsats[data[j].sat-1]!=1) j++;
        }
        if (j<=0||!screent(data[0].time,ts,ts,1.0)) continue; /* only 1 hz */

        if (!pntpos(data,j,nav,opt,&sol,NULL,NULL,msg)) continue;

        for (i=0;i<3;i++) ra[i]+=sol.rr[i];
        n++;
    }
    if (n<=0) {
        trace(1,"no average of base station position\n");
        return 0;
    }
    for (i=0;i<3;i++) ra[i]/=n;
    return 1;
}
int QPPPPost::getstapos(const char *file, char *name, double *r)
{
    FILE *fp;
    char buff[256],sname[256],*p,*q;
    double pos[3];

    trace(3,"getstapos: file=%s name=%s\n",file,name);

    if (!(fp=fopen(file,"r"))) {
        trace(1,"station position file open error: %s\n",file);
        return 0;
    }
    while (fgets(buff,sizeof(buff),fp)) {
        if ((p=strchr(buff,'%'))) *p='\0';

        if (sscanf(buff,"%lf %lf %lf %s",pos,pos+1,pos+2,sname)<4) continue;

        for (p=sname,q=name;*p&&*q;p++,q++) {
            if (toupper((int)*p)!=toupper((int)*q)) break;
        }
        if (!*p) {
            pos[0]*=D2R;
            pos[1]*=D2R;
            pos2ecef(pos,r);
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    trace(1,"no station position: %s %s\n",name,file);
    return 0;
}
int QPPPPost::antpos(prcopt_t *opt, int rcvno, const obs_t *obs, const nav_t *nav,
                  const sta_t *sta, const char *posfile)
{
    double *rr=rcvno==1?opt->ru:opt->rb,del[3],pos[3],dr[3]={0};
    int i,postype=rcvno==1?opt->rovpos:opt->refpos;
    char *name;

    trace(3,"antpos  : rcvno=%d\n",rcvno);

    if (postype==POSOPT_SINGLE) { /* average of single position */
        if (!avepos(rr,rcvno,obs,nav,opt)) {
            showmsg("error : station pos computation");
            return 0;
        }
    }
    else if (postype==POSOPT_FILE) { /* read from position file */
        name=m_stas[rcvno==1?0:1].name;
        if (!getstapos(posfile,name,rr)) {
            showmsg("error : no position of %s in %s",name,posfile);
            return 0;
        }
    }
    else if (postype==POSOPT_RINEX) { /* get from rinex header */
        if (norm(m_stas[rcvno==1?0:1].pos,3)<=0.0) {
            showmsg("error : no position in rinex header");
            trace(1,"no position position in rinex header\n");
            return 0;
        }
        /* antenna delta */
        if (m_stas[rcvno==1?0:1].deltype==0) { /* enu */
            for (i=0;i<3;i++) del[i]=m_stas[rcvno==1?0:1].del[i];
            del[2]+=m_stas[rcvno==1?0:1].hgt;
            ecef2pos(m_stas[rcvno==1?0:1].pos,pos);
            enu2ecef(pos,del,dr);
        }
        else { /* xyz */
            for (i=0;i<3;i++) dr[i]=m_stas[rcvno==1?0:1].del[i];
        }
        for (i=0;i<3;i++) rr[i]=m_stas[rcvno==1?0:1].pos[i]+dr[i];
    }
    return 1;
}
void QPPPPost::freeobsnav(obs_t *obs, nav_t *nav)
{
    trace(3,"freeobsnav:\n");

    free(obs->data); obs->data=NULL; obs->n =obs->nmax =0;
    free(nav->eph ); nav->eph =NULL; nav->n =nav->nmax =0;
    free(nav->geph); nav->geph=NULL; nav->ng=nav->ngmax=0;
    free(nav->seph); nav->seph=NULL; nav->ns=nav->nsmax=0;
}
void QPPPPost::readotl(prcopt_t *popt, const char *file, const sta_t *sta)
{
    int i,mode=PMODE_DGPS<=popt->mode&&popt->mode<=PMODE_FIXED;

    for (i=0;i<(mode?2:1);i++) {
        readblq(file,sta[i].name,popt->odisp[i]);
    }
}
int QPPPPost::execses_b(gtime_t ts, gtime_t te, double ti, const prcopt_t *popt,
                     const solopt_t *sopt, const filopt_t *fopt, int flag,
                     char **infile, const int *index, int n, char *outfile,
                     const char *rov, const char *base)
{
    gtime_t t0={0};
    int i,stat=0;
    char *ifile[MAXINFILE],ofile[1024],*base_,*p,*q,s[64];

    trace(3,"execses_b: n=%d outfile=%s\n",n,outfile);

    /* read prec ephemeris and sbas data */
    readpreceph(infile,n,popt,&m_navs,&m_sbss);

    for (i=0;i<n;i++) if (strstr(infile[i],"%b")) break;

    if (i<n) { /* include base station keywords */
        if (!(base_=(char *)malloc(strlen(base)+1))) {
            freepreceph(&m_navs,&m_sbss);
            return 0;
        }
        strcpy(base_,base);

        for (i=0;i<n;i++) {
            if (!(ifile[i]=(char *)malloc(1024))) {
                free(base_); for (;i>=0;i--) free(ifile[i]);
                freepreceph(&m_navs,&m_sbss);
                return 0;
            }
        }
        for (p=base_;;p=q+1) { /* for each base station */
            if ((q=strchr(p,' '))) *q='\0';

            if (*p) {
                strcpy(m_proc_base,p);
                if (ts.time) time2str(ts,s,0); else *s='\0';
                if (checkbrk("reading    : %s",s)) {
                    stat=1;
                    break;
                }
                for (i=0;i<n;i++) reppath(infile[i],ifile[i],t0,"",p);
                reppath(outfile,ofile,t0,"",p);

                stat=execses_r(ts,te,ti,popt,sopt,fopt,flag,ifile,index,n,ofile,rov);
            }
            if (stat==1||!q) break;
        }
        free(base_); for (i=0;i<n;i++) free(ifile[i]);
    }
    else {
        stat=execses_r(ts,te,ti,popt,sopt,fopt,flag,infile,index,n,outfile,rov);
    }
    /* free prec ephemeris and sbas data */
    freepreceph(&m_navs,&m_sbss);

    return stat;
}
int QPPPPost::openses(const prcopt_t *popt, const solopt_t *sopt,
                   const filopt_t *fopt, nav_t *nav, pcvs_t *pcvs, pcvs_t *pcvr)
{
    trace(3,"openses :\n");

    /* read satellite antenna parameters */
    if (*fopt->satantp&&!(readpcv(fopt->satantp,pcvs))) {
        showmsg("error : no sat ant pcv in %s",fopt->satantp);
        trace(1,"sat antenna pcv read error: %s\n",fopt->satantp);
        return 0;
    }
    /* read receiver antenna parameters */
    if (*fopt->rcvantp&&!(readpcv(fopt->rcvantp,pcvr))) {
        showmsg("error : no rec ant pcv in %s",fopt->rcvantp);
        trace(1,"rec antenna pcv read error: %s\n",fopt->rcvantp);
        return 0;
    }
    /* open geoid data */
    if (sopt->geoid>0&&*fopt->geoid) {
        if (!opengeoid(sopt->geoid,fopt->geoid)) {
            showmsg("error : no geoid data %s",fopt->geoid);
            trace(2,"no geoid data %s\n",fopt->geoid);
        }
    }
    return 1;
}
void QPPPPost::closeses(nav_t *nav, pcvs_t *pcvs, pcvs_t *pcvr)
{
    trace(3,"closeses:\n");

    /* free antenna parameters */
    free(pcvs->pcv); pcvs->pcv=NULL; pcvs->n=pcvs->nmax=0;
    free(pcvr->pcv); pcvr->pcv=NULL; pcvr->n=pcvr->nmax=0;

    /* close geoid data */
    closegeoid();

    /* free erp data */
    free(nav->erp.data); nav->erp.data=NULL; nav->erp.n=nav->erp.nmax=0;

    /* close solution statistics and debug trace */
    rtkclosestat();
    traceclose();
}
