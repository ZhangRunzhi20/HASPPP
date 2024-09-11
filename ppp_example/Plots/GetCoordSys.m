function Cs=GetCoordSys(type)

Cs.type='1984';
Cs.A=6378137.0;
Cs.Alfa=1.0/298.257223563;
Cs.E2=0.00669437999014132;
Cs.UTM=1.0;
Cs.X0=0;Cs.Y0=0;
Cs.L0=0;
Cs.H0=0;
Cs.DN=0;
Cs.GM=0;
Cs.Omega=0;
switch type
    case '1984'
        return
    case '1969'
        Cs.type='1969';
        Cs.A=6378160.0;
        Cs.Alfa=1.0/298.25;
        Cs.E2=2*Cs.Alfa*Cs.Alfa;
        Cs.GM=0;
        Cs.Omega=0;
        Cs.UTM=0.9996;
    case '1980'
        Cs.type='1980';
        Cs.A=6378140.0;
        Cs.UTM=1.0;
        Cs.Alfa=1.0/298.257;
        Cs.GM=0;
        Cs.Omega=0;
        Cs.E2=0.006694384999588;
    case '1954'
        Cs.type='1954';
        Cs.A=6378245.0;
        Cs.Alfa=1.0/298.3;
        Cs.E2=0.00669342162296594;
        Cs.GM=0;
        Cs.Omega=0;
        Cs.UTM=1.0;
    case '2000'
        Cs.type='2000';
        Cs.A=6378137.0;
        Cs.Alfa=1.0/298.257222101;
        Cs.E2=2*Cs.Alfa*Cs.Alfa;
        Cs.GM=3.986004418*1e+14;
        Cs.Omega=7.292115*1e-5;
        Cs.UTM=0;
    case 'PZ-90'
        Cs.type='PZ-90';
        Cs.A=6378136.0;
        Cs.Alfa=1.0/298.257839303;
        Cs.E2=2*Cs.Alfa*Cs.Alfa;
        Cs.GM=3.9860044*1e+14;
        Cs.Omega=7.292115*1e-5;
        Cs.UTM=0;
    otherwise
        disp('Error');
end

