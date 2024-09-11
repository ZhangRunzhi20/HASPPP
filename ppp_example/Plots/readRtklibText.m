function [ spp_pos, ppp_pos,pppar_pos, pppar_q, time_vct sat] = readRtklibText( txtFilename )
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here
spp_pos = [];
ppp_pos = [];
pppar_pos = [];
pppar_q = [];
time_vct = [];
sat=[];

fid = fopen(txtFilename, 'r');
if fid == -1
    error('open file bad.');
    return ;
end

begin_hour = [];
dateTime=[];
while ~feof(fid)
    strline_t = fgetl(fid);
    if strline_t(1) == '%'
        continue;
    end
    strline = strline_t(26:end);
    vct_line = str2num(strline);
    hour = str2num(strline_t(12:13));
    min = str2num(strline_t(15:16));
    sec = str2num(strline_t(18:19));
    dateTime = [dateTime;hour*3600+min*60+sec];
    ppp_pos = [ppp_pos; vct_line(1:3)];
    time_vct =[time_vct;strline_t(1:24)];
    sat =[sat;str2num(strline_t(75:77))];
    
end
dateTimeDiff=dateTime(2:end)-dateTime(1:end-1);
row = find(dateTimeDiff~=1&abs(dateTimeDiff)~=23*3600+59*60+5);
time_vct(row,:)
fclose(fid);

end

