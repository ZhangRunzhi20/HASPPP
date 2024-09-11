#include "rtklib.h"

char* Trim1(char *a){
    char *p1, *p2;
    p1 = a;
    p2 = a + strlen(a) - 1;
    while (p2 >= p1 && *p2 == ' ' || p2 >= p1 && *p2 =='\n'){
        p2--;
    }
    *(++p2) = '\0';
    return p1;

}

extern char *GetIniKeyString(char *title,char *key,char *defVal,char *filename)
{
    FILE *fp;
    int  flag = 0;
    char sTitle[32], *wTmp;
    static char sLine[1024];
    char *sLineP;

    sprintf(sTitle, "[%s]", title);
    if(NULL == (fp = fopen(filename, "r"))) {
        perror("fopen");
        return NULL;
    }

    while (NULL != fgets(sLine, 1024, fp)) {
        if (0 == strncmp("//", sLine, 2)) continue;
        if ('#' == sLine[0])              continue;
        sLineP = strstr(sLine,"##");
        if (sLineP!=NULL){
            *sLineP= '\0';
//            sLineP = Trim1(sLine);
        }
        Trim1(sLine);


        wTmp = strchr(sLine, '=');
        if ((NULL != wTmp) && (1 == flag)) {
            if(strlen(key)==wTmp-sLine){
                if (0 == strncmp(key, sLine, wTmp-sLine)) {
                    sLine[strlen(sLine)] = '\0';
                    fclose(fp);
                    return wTmp + 1;
                }
            }

        } else {
            if (0 == strncmp(sTitle, sLine, strlen(sTitle) - 1)) {
                flag = 1;
            }
        }
    }
    fclose(fp);
    return defVal;
}
extern double GetIniKeyDou(char *title,char *key,char *defVal,char *filename)
{
    char *out;
    if(out=GetIniKeyString(title, key, defVal,filename)==NULL){

    }
    return atof(GetIniKeyString(title, key, defVal,filename));
}

extern int GetIniKeyInt(char *title,char *key,char *defVal,char *filename)
{
    if(GetIniKeyString(title, key, defVal,filename)==NULL){

    }
    return atoi(GetIniKeyString(title, key, defVal,filename));
}

static int PutIniKeyStringSta(int flagIn,int *find,char *title,char *key,char *val,char *filename)
{
    FILE *fpr, *fpw;
    int  flag = 0;
    char sLine[10240], sTitle[32], *wTmp;
    char deF[10240];
    *find = 0;
//    printf("%d\n",strlen(title));

    sprintf(sTitle, "[%s]", title);
    if (NULL == (fpr = fopen(filename, "r")))
        return;
        //PRN_ERRMSG_RETURN("fopen");
    sprintf(sLine, "%s.tmp", filename);
    if (NULL == (fpw = fopen(sLine,    "w")))
        return;
        //PRN_ERRMSG_RETURN("fopen");

    while (NULL != fgets(sLine, 10240, fpr)) {
        if (2 != flag) {
            wTmp = strchr(sLine, '=');
            if ((NULL != wTmp) && (1 == flag)) {
                if(flagIn == 1){
                    sprintf(deF,"%s=%s\n",key,val);
                    fputs(deF, fpw);
                    flagIn = 0;
                }
                if(strlen(key)==wTmp-sLine){
                    if (0 == strncmp(key, sLine, wTmp-sLine)) {
                        flag = 2;
                        *find = 1;
                        sprintf(wTmp + 1, "%s\n", val);
                    }
                }

            } else {
//                printf("%d\n", strlen(sTitle)-1);
                if (0 == strncmp(sTitle, sLine, strlen(sTitle)-1)) {//delete [
                    flag = 1;
                }
            }
        }

        fputs(sLine, fpw);
    }
    fclose(fpr);
    fclose(fpw);

    sprintf(sLine, "%s.tmp", filename);
    return rename(sLine, filename);
}

extern int PutIniKeyString(char *title,char *key,char *val,char *filename){
    int find;
    if(PutIniKeyStringSta(0,&find,title,key,val,filename)){
        return 0;
    }
    if(find==0){
        if(PutIniKeyStringSta(1,&find,title,key,val,filename)){
            return 0;
        }
    }
    return  1;

}

extern int PutIniKeyInt(char *title,char *key,int val,char *filename)
{
    char sVal[32];
    sprintf(sVal, "%d", val);
    return PutIniKeyString(title, key, sVal, filename);
}

extern int PutIniKeyDou(char *title,char *key,double val,char *filename)
{
    char sVal[32];
    sprintf(sVal, "%f", val);
    return PutIniKeyString(title, key, sVal, filename);
}


