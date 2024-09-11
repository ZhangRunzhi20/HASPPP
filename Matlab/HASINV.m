clc;
clear all;

D=[31 50 155; 113 18 35;204 239 127];
Dtemp=[18,127];
Dtemp=gf(Dtemp,8);
Dtemp(1,1)*Dtemp(1,2)
D=gf(D,8);
inv(D)

data=0:255;
hJ=gf(data,8);
oH=hJ'*hJ;

fid=fopen('HASInv.txt','w');
if fid==-1
    return;
end
for i=1:256
    temp=oH(i,:);
    tempDouble=double(temp.x);
    for j=1:256
        if j~=256
            fprintf(fid,'%d ',tempDouble(j));
        else
            fprintf(fid,'%d \n',tempDouble(j));
        end
    end
    
end


fclose(fid);