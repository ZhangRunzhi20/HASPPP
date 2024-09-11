#include <QCoreApplication>
#include <iostream>
#include <fstream>
#include <string>
#include "HASPPP.h"
#include <vector>
#include <dirent.h>


#ifdef WIN32
#include <windows.h>
#endif

#ifdef WIN32
#define FILEPATHSEP '\\'
#define FILEPATHSEPARR "\\"
#else
#define FILEPATHSEP '/'
#define FILEPATHSEPARR "/"
#endif

using namespace std;

#ifdef WIN32
int filepid(const char *folder_path,std::vector<std::string> &filelists){
    int n=0;
    DIR *dir;
    struct dirent *entry;
    if ((dir = opendir(folder_path))!=NULL){
        while ((entry = readdir(dir))!=NULL){
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            if (strstr(entry->d_name,".txt")!=NULL && strcmp(entry->d_name+strlen(entry->d_name)-4,".txt")==0){
                std::cout<<entry->d_name<<std::endl;
                filelists.push_back(entry->d_name);
                n++;
            }

        }
        closedir(dir);
        return n;
    }
    else{
        return 0;
    }
}
#else
int filepid(const char *folder_path,std::vector<std::string> &filelists){
    int n=0;
    DIR *dir;
    struct dirent *entry;
    if ((dir = opendir(folder_path))!=NULL){
        while ((entry = readdir(dir))!=NULL){
            if (entry->d_type==DT_REG){
                if (strstr(entry->d_name,".txt")!=NULL && strcmp(entry->d_name+strlen(entry->d_name)-4,".txt")==0){
                    std::cout<<entry->d_name<<std::endl;
                    filelists.push_back(entry->d_name);
                    n++;
                }
            }
        }
        closedir(dir);
        return n;
    }
    else{
        return 0;
    }
}
#endif

void readpid(std::string filepath, std::vector<std::string> filelists,MatrixXi &finPage,int n,int *pid){
    int i,order=0;
    std::string delimiter =".";
    std::string filefolder = filepath + std::string(FILEPATHSEPARR);
    for (i=0;i<n;i++){
        order=0;
        std::string filename=filelists.at(i);
        size_t pos = 0;
        std::string token;

        if((pos = filename.find(delimiter)) != std::string::npos){
            token = filename.substr(0,pos);
//            std::cout<< token<<std::endl;
            pid[i] = std::stoi(token.substr(3,pos));
        }


        std::string filefull = filefolder + filename;
        std::ifstream file(filefull);
        if (file.is_open()){
            std::string line;
            while (std::getline(file,line)){
                finPage(i,order) = std::stoi(line);
                order++;
//                std::cout <<line<<std::endl;

            }
            file.close();
        }

    }

}

int verifymess(std::string calvalue,std::string truepath){

    std::ifstream file(truepath);
    std::string line;
    if (file.is_open()){
        std::getline(file,line);
        file.close();
    }
    if (calvalue==line){
        return 1;
    }
    else {
        return 0;
    }
}

void bubblesort(int *pid,int n_pid,MatrixXi &Page){
    int i,row,col;
    MatrixXi finPage;
    row = Page.rows();
    col = Page.cols();
    finPage.resize(1,53);
    for (i=0;i<n_pid-1;++i){
        for(int j=0;j<n_pid-i-1;++j){
            if(pid[j]>pid[j+1]){
                int temp = pid[j];
                pid[j]=pid[j+1];
                pid[j+1]=temp;

                finPage = Page.row(j);
                Page.row(j) = Page.row(j+1);
                Page.row(j+1) = finPage;
            }
        }
    }
}


int main(int argc, char *argv[])
{
    int pid[255],flag,n;
    std::string pageString;
    std::vector<std::string> filelists;
    MatrixXi finPage;
    hassat_t hasSat_i;
    hassat_t *hasSat_o = (hassat_t*)malloc(sizeof(hassat_t));
    gtime_t gpstFrame;
    double ep[6]= {2024,2,22,0,0,0};

    if (argc<2){
        printf("Missing decoding file path!\n");
    }

    /* example 1 */

    /* Step1: set HAS workpath*/
    HASPPP HAS(argv[0]);

    std::string strCurPath = std::string(argv[0]);


    std::string pidpath;

    pidpath = strCurPath.substr( 0, strCurPath.find_last_of(FILEPATHSEP));
    pidpath = pidpath.substr( 0, pidpath.find_last_of(FILEPATHSEP));

    pidpath = pidpath +std::string(FILEPATHSEPARR)+ std::string("data");

    pidpath = std::string(argv[1]);

    std::cout<<pidpath<<std::endl;

    /* Step2: read all pid from HAS ICD*/

    //    char *folder_path = "/media/david/Passport/HASPPP/office_example/pid";
    //    n=filepid(folder_path,filelists);
    //    finPage.resize(n,53);
    //    std::string filepath = std::string(folder_path);

    std::cout<<"Start to find official pid files"<<std::endl;
    n=filepid(pidpath.c_str(),filelists);
    if (n==0){
        printf("Missing decoding file: %d\n",n);
    }
    finPage.resize(n,53);
    std::string filepath = pidpath;

    std::cout<<"All official pid files: "<<n<<std::endl;
    readpid(filepath,filelists,finPage,n,pid);

    /* Step3: sort by n_pid */
    bubblesort(pid,n,finPage);

    /* HAS decoded message */
    std::cout<<"Start to decode RS"<<std::endl;
    HAS.decodeRs(finPage,pid,n,pageString);

    /* Verify if the result is correct */
    std::cout<< "Verify decoded results"<<std::endl;
    std::string decoded_message = pidpath + std::string(FILEPATHSEPARR)+std::string("office_decoded_message.rs");
    flag = verifymess(pageString,decoded_message);

    if (flag==0){
        std::cout<< "DecodeMessage Fail!" <<std::endl;
        free(hasSat_o);
        return 0;
    }
    std::cout<< "DecodeMessage Success!" <<std::endl;

    HAS.initHassat_t(hasSat_i);
    gpstFrame = HAS.epoch2time(ep);

    std::cout<<"Start to recovery HAS corrections"<<std::endl;
    HAS.decodePageChange(pageString,n,gpstFrame,&hasSat_i,hasSat_o);
    free(hasSat_o);
    std::cout<<"Finished!"<<std::endl;





    return 1;

}



