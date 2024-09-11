%% This script mainly draws the PPP calculation error curve
clear
clc
clear all

%%NOTE
%: The positioning result filename must start with the station 
% in order to search for true values from the SNX file using the station name.

if ispc %Windows
    file_path = 'G:\\HASPPP\\ppp_example\\data\\sino\\ptbb.pos' % positioning result file
    [allStations]=readsnx('G:\\HASPPP\\ppp_example\\Plots\\SNX\\IGS0OPSSNX_20231660000_01D_01D_CRD.SNX'); % true coordinates
elseif isunix %Linux
    file_path = '/media/david/Passport/HASPPP/ppp_example/data/sino/ptbb1660_g.pos';
    [allStations]=readsnx('/media/david/Passport/HASPPP/ppp_example/Plots/SNX/IGS0OPSSNX_20231660000_01D_01D_CRD.SNX');
end


  
if ispc
    lastSlashIndex = max(strfind(file_path, '\'));
elseif isunix
    lastSlashIndex = max(strfind(file_path, '/'));
end

station_name = file_path(lastSlashIndex+1:lastSlashIndex+4);
[ true_pos ] = search_snx( allStations, station_name ); 
%Please check that the true_pos value is not 0
 
  
[ spp_pos1, ppp_pos1,pppar_pos1, pppar_q1, time_vct1 sat ] = readRtklibText( file_path );


 
% ppp_pos = rtklibpos; ylim([-6 6])



time_vctback = time_vct1;
sol_PPP = ppp_pos1;
bad_flag = find(sol_PPP(:,1)==0);
sol_PPP(bad_flag,:) = [];
time_vct1(bad_flag,:) = [];
data_len = size(sol_PPP,1);
NEU1 = [];
for nk = 1:data_len
    dNEU1 = XYZ_NEU(sol_PPP(nk,:) , true_pos - sol_PPP(nk,:));
    NEU1 = [NEU1;dNEU1'];
end

NEU2=NEU1;
time_vct2=time_vct1;

Nrms=rms(NEU1(10:end,1))
Erms=rms(NEU1(10:end,2))
Urms=rms(NEU1(10:end,3))




%% plot data
X = 1:size(NEU1,1);
figure;
hold on
plot(X, NEU1(:,1), 'r.')
plot(X, NEU1(:,2), 'g.')
plot(X, NEU1(:,3), 'b.')
title('GPS/Galileo static results ')
legend('N', 'E', 'U')
xlabel('epoch(30s)')
ylabel('Error(m)')
ylim([-0.2, 0.2]);
grid on
legend('N', 'E', 'U')
return

%% save error
doylist2023 = [31 28 31 30 31 30 31 31 30 31 30 31];
savepeth = [file_path '.txt'];
fid = fopen(savepeth, 'w');
if fid == -1
    error('open file bad.');
    return ;
end
if size(NEU1,1)~=size(time_vct1)
    error('date length erro.');
end

fprintf(fid,'************************************************\r\n');
fprintf(fid,'nrms= %1.3f    erms= %1.3f   urms= %1.3f\r\n',Nrms,Erms,Urms);

fprintf(fid,'************************************************\r\n');
for i=1:size(time_vct2,1)
    temp =time_vct2(i,:);
    day=str2num(temp(9:10));
    month=str2num(temp(6:7));
    if month>1
        doy= day+sum(doylist2023(1:month-1));
    else
        doy= day;
    end
    
    date=[num2str(doy) ':' temp(12:end)];
    
    fprintf(fid,'%s  %1.3f   %1.3f   %1.3f\r\n',date,NEU1(i,1),NEU1(i,2),NEU1(i,3));
end

fclose(fid);

return




