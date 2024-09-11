clc
clear all;

data_Temp=textread('has.txt','%d');
data=data_Temp(224:-1:1);
%data=data_Temp;


hJ=gf(data,8);


hJ=[zeros(1,31),hJ'];
G=[];
for i=1:32
    G=[G;hJ];
    hJ=[hJ(2:end),hJ(1)];
end
G=G';
GT=G(1:32,1:32);


GN=G(33:255,1:32);
G=[eye(32);GN*inv(GT)];




fid=fopen('GMatrix.txt','w');
if fid==-1
    return;
end
for i=1:255
    temp=G(i,:);
    fprintf(fid,'%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d \n',double(temp.x));
end

fclose(fid);