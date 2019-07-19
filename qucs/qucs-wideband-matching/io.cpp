/*
 * io.cpp - Input/output operations class implementation
 *
 * copyright (C) 2016 Andres Martinez-Mera <andresmartinezmera@gmail.com>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 *
 */




#include "io.h"

IO::IO()
{
    Nsamples = 30;
    fmatching_min = -1;
    fmatching_max = -1;
    CopyToClipboard=true;
}

IO::~IO()
{}

// Converts std::string to double
double string_to_double( const std::string& s )
{
    std::istringstream i(s);
    double x;
    if (!(i >> x))
        return 0;
    return x;
}


//Export data according to the GNUplot data file format
int IO::exportGNUplot(GRABIM_Result Res, string folderpath, bool LocalOptimizer)
{
    ofstream GNUplotExport;
    if (folderpath.empty()) folderpath = this->tmp_path;
    string datapath = folderpath+"/data";
    string scriptpath = folderpath+"/plotscript";
    GNUplotExport.open (datapath, std::ofstream::out);
    if(!GNUplotExport.is_open()) return -1;
    GNUplotExport << "#GRABIM data" << endl;
    if (LocalOptimizer)
    {
       GNUplotExport << "#freq S11(Grid search) S11(Local optimiser) S21(Grid search) S21(Local optimiser) " << endl;
       for (unsigned int i = 0; i < Res.freq.size(); i++)
       {//Writes frequency S11_grid[db] S11_optimiser[db] and S21[dB]
         GNUplotExport << Res.freq.at(i)*1e-9 << " " << 20*log10(abs(Res.S11_gridsearch.at(i))) << " "
                      << 20*log10(abs(Res.S21_gridsearch.at(i))) << " " << 20*log10(abs(Res.S11_nlopt.at(i)))
                      << " " << 20*log10(abs(Res.S21_nlopt.at(i))) << " " << real(Res.S11_nlopt.at(i))
                      << " " << imag(Res.S11_nlopt.at(i)) <<endl;
       }
    }
    else
    {//The local optimizer step was skipped
       GNUplotExport << "#freq S11(Grid search) S11(Local optimiser) S21(Grid search) S21(Local optimiser) " << endl;
       for (unsigned int i = 0; i < Res.freq.size(); i++)
       {//Writes frequency S11_grid[db] S11_optimiser[db] and S21[dB]
         GNUplotExport << Res.freq.at(i)*1e-9 << " " << 20*log10(abs(Res.S11_gridsearch.at(i))) << " "
                      << 20*log10(abs(Res.S21_gridsearch.at(i))) << " " << -1
                      << " " << -1 << " " << real(Res.S11_gridsearch.at(i))
                      << " " << imag(Res.S11_gridsearch.at(i)) <<endl;
       }
    }
    GNUplotExport.close();
    generateGNUplotScript(datapath, scriptpath, LocalOptimizer);
    string gnuplot_command = "gnuplot " + scriptpath + " -p";
    system(gnuplot_command.c_str());
    system("\n");
return 0;
}


void IO::generateGNUplotScript(string datapath, string scriptpath, bool LocalOptimizer)
{
   string script;
   script = "source = \"" + datapath + "\"\n";
   script += "# S21, S11 [dB]\n"
              "set term x11 0\n"
              "set title \"Network response\"\n"
              "set xlabel \"Frequency (GHz)\"\n"
              "set grid ytics lt 0 lw 1 lc rgb \"#bbbbbb\"\n"
              "set grid xtics lt 0 lw 1 lc rgb \"#bbbbbb\"\n";

   if (LocalOptimizer)
   {
   script += "plot source using 1:5 with lines title \"S21 (dB) (refined)\" linecolor rgb \"red\" linewidth 2, source using 1:4 with lines title \"S11 (dB) (refined)\" linecolor rgb \"blue\" linewidth 2;\n";
   }
   else
   {
    script += "plot source using 1:3 with lines title \"S21 (dB) Grid search\" linecolor rgb \"red\" linewidth 2, source using 1:2 with lines title \"S11 (dB) Grid search\" linecolor rgb \"blue\" linewidth 2;\n";
   }

   script +=  "#Smith chart plot\n"
              "#Source: http://swigerco.com/gnuradio/phase/vna_comp/\n"
              "set title \"S11\"\n"
              "set term x11 2\n"
              "set size square\n"
              "set clip\n"
              "set xtics axis nomirror\n"
              "set ytics axis nomirror\n"
              "unset grid\n"
              "unset polar\n"
              "unset key\n"
              "set para\n"
              "set rrange [-0 : 10]\n"
              "set trange [-pi : pi]\n"
              "set xrange [-1:1]\n"
              "set yrange [-1:1]\n"
              "set label \"-10dB\" at 0.22,0.22\n"
              "set label \"-15dB\" at 0.1,0.1\n"
              "tv(t,r) = sin(t)/(1+r)\n"
              "tu(t,r) = (cos(t) +r)/(1+r)\n"
              "cu(t,x) = 1 + cos(t)/x\n"
              "cv(t,x) = (1+ sin(t))/x\n"
              "plot cu(t,.1) linecolor rgb \"black\" linewidth 0.2 ,cv(t,.1) linecolor rgb \"black\" linewidth 0.2 ,cu(t,.1) linecolor rgb \"black\" linewidth 0.2 ,-cv(t,.1) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "cu(t,1) linecolor rgb \"black\" linewidth 0.2 ,cv(t,1) linecolor rgb \"black\" linewidth 0.2 ,cu(t,1) linecolor rgb \"black\" linewidth 0.2 ,-cv(t,1) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "cu(t,10) linecolor rgb \"black\" linewidth 0.2 ,cv(t,10) linecolor rgb \"black\" linewidth 0.2 ,cu(t,10) linecolor rgb \"black\" linewidth 0.2 ,-cv(t,10) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,.1) linecolor rgb \"black\" linewidth 0.2, tv(t,.1) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,.2) linecolor rgb \"black\" linewidth 0.2 ,tv(t,.2) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,.3) linecolor rgb \"black\" linewidth 0.2 ,tv(t,.3) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,.4) linecolor rgb \"black\" linewidth 0.2 ,tv(t,.4) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,.5) linecolor rgb \"black\" linewidth 0.2 ,tv(t,.5) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,.6) linecolor rgb \"black\" linewidth 0.2 ,tv(t,.6) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,.7) linecolor rgb \"black\" linewidth 0.2 ,tv(t,.7) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,.8) linecolor rgb \"black\" linewidth 0.2 ,tv(t,.8) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,.9) linecolor rgb \"black\" linewidth 0.2 ,tv(t,.9) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,1) linecolor rgb \"black\" linewidth 0.2 ,tv(t,1) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,2) linecolor rgb \"black\" linewidth 0.2 ,tv(t,2) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,5) linecolor rgb \"black\" linewidth 0.2 ,tv(t,5) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,10) linecolor rgb \"black\" linewidth 0.2 ,tv(t,10) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "cu(t,.5) linecolor rgb \"black\" linewidth 0.2 ,cv(t,.5) linecolor rgb \"black\" linewidth 0.2 ,cu(t,.5) linecolor rgb \"black\" linewidth 0.2 ,-cv(t,.5) linecolor rgb \"black\" linewidth 0.2 ,\\\n"
              "tu(t,0) linecolor rgb \"black\" linewidth 0.6 ,tv(t,0) linecolor rgb \"black\" linewidth 0.6 , source using 6:7 linecolor rgb \"red\" with lines,0.335*sin(t),0.335*cos(t) linecolor rgb \"green\",0.175*sin(t),0.175*cos(t) linecolor rgb \"green\"\n";
    ofstream GNUplotScript;
    GNUplotScript.open (scriptpath, std::ofstream::out);
    GNUplotScript << script;
    GNUplotScript.close();
}

// Extends std::tolower(int c) capability to st::string arguments
string tolower(string str)
{
    char c;
    for (unsigned int i =0; i < str.length();i++)
    {
        c=str.at(i);
        str.at(i) = tolower(c);
    }
    return str;
}

//Removes consecutive blank spaces in a std::string
string RemoveBlankSpaces(string line)
{
    //Remove consecutive repeated blank spaces and space at the beginning
    //Sometimes, the fields may be separated by \t...
    for (unsigned int i = 0; i< line.length(); i++)
    {
        if (i == 0)//Remove first space
        {
            if((!line.substr(0,1).compare(" "))||(!line.substr(0,1).compare("\t")))
            {
                line.erase(0, 1);
                i--;
            }
            continue;
        }
        if (((!line.substr(i-1, 1).compare(" "))||(!line.substr(i-1,1).compare("\t")))&&((!line.substr(i, 1).compare(" "))||(!line.substr(i,1).compare("\t"))))
        {
            line.erase(i, 1);
            i--;
        }
    }
    return line;
}



// Loads impedance data from a s1p file
int IO::loadS1Pdata(std::string filepath, terminal Port)
{
    std::ifstream s2pfile(filepath.c_str());//Tries to open the data file.
    if(!s2pfile.is_open())//The data file cannot be opened => error
    {
        return -1;
    }

    std::string line;
    double freq_scale = 1;
    double Zref = 50;

    std::getline(s2pfile, line);
    while(line.compare(0, 1, "#"))//Looking for # field
    {
        std::getline(s2pfile, line);
    }

    line = tolower(line);
    freq_scale = getS2PfreqScale(line);


    //Get the impedance at which the S params were measured

    int Rindex = line.find_last_of("r");
    Rindex = line.find_first_not_of(" ", Rindex);
    Zref = atof(line.substr(Rindex+1).c_str());

    bool is_indB = (line.find("db") != string::npos);
    bool RI = (line.find("ma") == string::npos);
    bool isS_par = (line.find(" s ") != string::npos);
    bool isZ_par = (line.find(" z ") != string::npos);


    while( getline(s2pfile, line) )
    {//Looking for the start of the raw data

        line = RemoveBlankSpaces(line);

        if ((!line.compare(0,1, "!"))|| (line.length() == 1)) continue;
        else break;


    }

    //DATA beginning.
    //At this point, the number of frequency samples is not known, so it's better to
    //push data into queues and then arrange it into armadillo structures
    std::queue <double> frequency, S11M, S11A;
    unsigned int qsize=0;

    do
    {
        line = RemoveBlankSpaces(line);
        if (line.empty()|| (line.length()==1))break;
        if (line.at(0) == '!') break;//Comment

        //Frequency
        int index = line.find_first_of(" ");
        if (index == -1)index = line.find_first_of("\t");
        frequency.push(string_to_double(line.substr(0,index)));
        line.erase(0, index+1);


        index = line.find_first_of(" ");
        if (index == -1)index = line.find_first_of("\t");
        S11M.push(string_to_double(line.substr(0,index)));
        line.erase(0, index+1);

        index = line.find_first_of(" ");
        if (index == -1)index = line.find_first_of("\t");
        S11A.push(string_to_double(line.substr(0,index)));

        qsize++;
    }while (std::getline(s2pfile, line));
    s2pfile.close();
    vector<double> freq(qsize);
    vector<complex<double>> S(qsize);
    vector<complex<double>> Z(qsize);

    double phi, S11m, S11a;
    for (unsigned int i = 0; i < qsize; i++)
    {
        freq[i] = freq_scale*frequency.front();
        frequency.pop();

        S11m = S11M.front();
        S11a = S11A.front();
        S11M.pop();
        S11A.pop();
        if (is_indB) S11m = pow(10, .05*S11m);
        phi = (pi/180)*S11a;

        if (RI)
        {
            S[i] = complex<double>(S11m, S11a);
        }
        else
        {
            S[i] = complex<double>(S11m,0)*complex<double>(cos(phi), sin(phi));
        }

        if (isZ_par)
        {//The data file contains impedance data
            Z[i] =  S[i];
        }
        if (isS_par)
        {//The data file contains s1p data
            Z[i] = Zref*((1.+S[i])/(1.-S[i]));//Z
        }

    }
    vector<string> candidates;
    if (Port == SOURCE)//Set source port properties
    {
        fS = freq;
        ZS = Z;
    }

    if (Port == LOAD)//Set load port properties
    {
        fL = freq;
        ZL = Z;
    }
    return 0;
}

//Reads a s2p Touchstone file
int IO::loadS2Pdata(std::string filepath)
{
   std::ifstream s2pfile(filepath.c_str());//Tries to open the data file.
    if(!s2pfile.is_open())//The data file cannot be opened => error
    {
        return -1;
    }
    std::string line;
    double freq_scale = 1;
    //double Zref = 50;

    std::getline(s2pfile, line);
    while(line.compare(0, 1, "#"))//Looking for # field
    {
        std::getline(s2pfile, line);
    }

    line = tolower(line);
    freq_scale = getS2PfreqScale(line);


    //Get the impedance at which the S params were measured

    int Rindex = line.find_last_of("r");
    Rindex = line.find_first_not_of(" ", Rindex);
    AmpS2P.Z0 = atof(line.substr(Rindex+1).c_str());

    bool is_indB = (line.find("db") != string::npos);
    bool RI = (line.find("ri") != string::npos);

    while( getline(s2pfile, line) )
    {//Looking for the start of the raw data

        line = RemoveBlankSpaces(line);

        if ((!line.compare(0,1, "!"))|| (line.length() == 1)) continue;
        else break;


    }

    //DATA beginning.
    //At this point, the number of frequency samples is not known, so it's better to
    //push data into queues and then arrange it into armadillo structures
    std::queue <double> frequency, S11M, S11A, S21M, S21A, S12M, S12A, S22M, S22A;
    unsigned int qsize=0;

    do
    {
        line = RemoveBlankSpaces(line);
        if (line.empty()|| (line.length()==1))break;
        if (line.at(0) == '!') break;//Comment

        //Frequency
        int index = line.find_first_of(" ");
        if (index == -1)index = line.find_first_of("\t");
        frequency.push(string_to_double(line.substr(0,index)));
        line.erase(0, index+1);

        //Read S11
        //-----------------------------------------------
        index = line.find_first_of(" ");
        if (index == -1)index = line.find_first_of("\t");
        S11M.push(string_to_double(line.substr(0,index)));
        line.erase(0, index+1);

        index = line.find_first_of(" ");
        if (index == -1)index = line.find_first_of("\t");
        S11A.push(string_to_double(line.substr(0,index)));
        line.erase(0, index+1);
        //-----------------------------------------------

        //Read S21
        //-----------------------------------------------
        index = line.find_first_of(" ");
        if (index == -1)index = line.find_first_of("\t");
        S21M.push(string_to_double(line.substr(0,index)));
        line.erase(0, index+1);

        index = line.find_first_of(" ");
        if (index == -1)index = line.find_first_of("\t");
        S21A.push(string_to_double(line.substr(0,index)));
        line.erase(0, index+1);
        //-----------------------------------------------

        //Read S12
        //-----------------------------------------------
        index = line.find_first_of(" ");
        if (index == -1)index = line.find_first_of("\t");
        S12M.push(string_to_double(line.substr(0,index)));
        line.erase(0, index+1);

        index = line.find_first_of(" ");
        if (index == -1)index = line.find_first_of("\t");
        S12A.push(string_to_double(line.substr(0,index)));
        line.erase(0, index+1);
        //-----------------------------------------------

        //Read S22
        //-----------------------------------------------
        index = line.find_first_of(" ");
        if (index == -1)index = line.find_first_of("\t");
        S22M.push(string_to_double(line.substr(0,index)));
        line.erase(0, index+1);

        index = line.find_first_of(" ");
        if (index == -1)index = line.find_first_of("\t");
        S22A.push(string_to_double(line.substr(0,index)));
        //-----------------------------------------------

        qsize++;
    }while (std::getline(s2pfile, line));
    s2pfile.close();
    vector<double> freq(qsize);

    double S11m, S11a, S21m, S21a, S12m, S12a, S22m, S22a;
    for (unsigned int i = 0; i < qsize; i++)
    {
        freq[i] = freq_scale*frequency.front();
        frequency.pop();

        S11m = S11M.front();
        S11a = S11A.front();
        S11M.pop();
        S11A.pop();

        S21m = S21M.front();
        S21a = S21A.front();
        S21M.pop();
        S21A.pop();

        S12m = S12M.front();
        S12a = S12A.front();
        S12M.pop();
        S12A.pop();

        S22m = S22M.front();
        S22a = S22A.front();
        S22M.pop();
        S22A.pop();

        if (is_indB) S11m = pow(10, .05*S11m);
        if (is_indB) S21m = pow(10, .05*S21m); 
        if (is_indB) S12m = pow(10, .05*S12m); 
        if (is_indB) S22m = pow(10, .05*S22m);

        if (RI)
        {
    	    AmpS2P.S11.push_back(complex<double>(S11m, S11a));
    	    AmpS2P.S21.push_back(complex<double>(S21m, S21a)); 
    	    AmpS2P.S12.push_back(complex<double>(S12m, S12a)); 
    	    AmpS2P.S22.push_back(complex<double>(S22m, S22a)); 
        }
        else
        {            
            S11a = (pi/180)*S11a;
            S21a = (pi/180)*S21a;
            S12a = (pi/180)*S12a;
            S22a = (pi/180)*S22a;
    	    
            AmpS2P.S11.push_back(complex<double>(S11m,0)*complex<double>(cos(S11a), sin(S11a)));
    	    AmpS2P.S21.push_back(complex<double>(S21m,0)*complex<double>(cos(S21a), sin(S21a))); 
    	    AmpS2P.S12.push_back(complex<double>(S12m,0)*complex<double>(cos(S12a), sin(S12a))); 
    	    AmpS2P.S22.push_back(complex<double>(S22m,0)*complex<double>(cos(S22a), sin(S22a)));
        }
 


    }
    fAMP = freq;
    return 0;
}

// Load and source impedances may be sampled at different frequencies. It is essential to resample them
// using the same frequency basis. This requires interpolation of complex data. It would be desirable to use
// spline or cubic interpolation, but it seems that they are not implemented in Armadillo
int IO::ResampleImpedances()
{
    if (ZS.size() == 0) return 0;//Not set
    if (ZL.size() == 0) return 0;//Not set
    if ((Two_Port_Matching)&&(fAMP.size() == 0)) return 0;//Not set

    if (fmatching_min == -1) return 0;

    freq = linspace(fmatching_min, fmatching_max, Nsamples);//Frequency vector employed for matching

    //Source impedance
    if (ZS.size() == 1)
    {
       complex<double> zs_temp = ZS[0];
       ZS.resize(freq.size());
       ZS = ones(ZS);
       ZS = Product(ZS,zs_temp);
    }
    else
    {
        //Extract impedance lying on the specified frequency band defined by the user in the UI
        unsigned int i1s = closestIndex(fS, fmatching_min);
        unsigned int i2s = closestIndex(fS, fmatching_max);
        vector<complex<double>> zs = SubVector(ZS, i1s, i2s);//Useful impedance data
        vector<double> fs = SubVector(fS, i1s, i2s);//Frequency points where the impedance zs is sampled
        vector<complex<double>> ZS_ = interp(fs, zs, freq);//Interpolation using input data
        ZS.resize(ZS_.size());//Resize ZS array
        ZS = ZS_;//Copy data
    }

    //Load impedance
    if (ZL.size() == 1)
    {
       complex<double> zl_temp = ZL[0];
       ZL.resize(freq.size());
       ZL = ones(ZL);
       ZL = Product(ZL,zl_temp);
    }
    else
    {
        //Extract impedance lying on the specified frequency band defined by the user in the UI
        unsigned int i1l = closestIndex(fL, fmatching_min);
        unsigned int i2l = closestIndex(fL, fmatching_max);
        vector<complex<double>> zl = SubVector(ZL, i1l, i2l);//Useful impedance data
        vector<double> fl = SubVector(fL, i1l, i2l);//Frequency points where the impedance zs is sampled
        vector<complex<double>> ZL_ = interp(fl, zl, freq);//Interpolation using input data
        ZL.resize(ZL_.size());//Resize ZS array
        ZL = ZL_;//Copy data
    }

    if (AmpS2P.S11.empty()) return 0;//One port matching

    //Amplifier SPAR data
    if (AmpS2P.S11.size() == 1)//Constant SPAR vs frequency. Corner case, but some user may input data like this
    {
       complex<double> S11_temp = AmpS2P.S11[0];
       complex<double> S21_temp = AmpS2P.S21[0];
       complex<double> S12_temp = AmpS2P.S12[0];
       complex<double> S22_temp = AmpS2P.S22[0];

       AmpS2P.S11.resize(freq.size());
       AmpS2P.S21.resize(freq.size());
       AmpS2P.S12.resize(freq.size());
       AmpS2P.S22.resize(freq.size());

       AmpS2P.S11 = ones(AmpS2P.S11);
       AmpS2P.S21 = ones(AmpS2P.S21);
       AmpS2P.S12 = ones(AmpS2P.S12);
       AmpS2P.S22 = ones(AmpS2P.S22);

       AmpS2P.S11 = Product(AmpS2P.S11,S11_temp);
       AmpS2P.S21 = Product(AmpS2P.S21,S21_temp);
       AmpS2P.S12 = Product(AmpS2P.S12,S12_temp);
       AmpS2P.S22 = Product(AmpS2P.S22,S22_temp);
    }
   else
   {
        //Extract impedance lying on the specified frequency band defined by the user in the UI
        unsigned int i1amp = closestIndex(fAMP, fmatching_min);
        unsigned int i2amp = closestIndex(fAMP, fmatching_max);
        
        vector<complex<double>> s11 = SubVector(AmpS2P.S11, i1amp, i2amp);
        vector<complex<double>> s21 = SubVector(AmpS2P.S21, i1amp, i2amp);
        vector<complex<double>> s12 = SubVector(AmpS2P.S12, i1amp, i2amp);
        vector<complex<double>> s22 = SubVector(AmpS2P.S22, i1amp, i2amp);
        vector<double> fa = SubVector(fAMP, i1amp, i2amp);//Frequency points where the impedance zs is sampled

        vector<complex<double>> S11_ = interp(fa, s11, freq);//Interpolation using input data
        vector<complex<double>> S21_ = interp(fa, s21, freq);
        vector<complex<double>> S12_ = interp(fa, s12, freq);
        vector<complex<double>> S22_ = interp(fa, s22, freq);

        AmpS2P.S11.resize(S11_.size());
        AmpS2P.S21.resize(S21_.size());
        AmpS2P.S12.resize(S12_.size());
        AmpS2P.S22.resize(S22_.size());

        AmpS2P.S11 = S11_;//Copy data
        AmpS2P.S21 = S21_;
        AmpS2P.S12 = S12_;
        AmpS2P.S22 = S22_;
   }

   //Finally, we calculate conj(Zin) and conj(Zout) for achieving maximum gain on a two-port device

    /***********************************************************************
      (gamma_S)                (gamma_in)    (gamma_out)                (gamma_L)
              _________________       _______        __________________
             |                 | Zin |       | Zout |                  |
        ZS---| INPUT MATCHING  |-----|  TRT  |------| OUTPUT MATCHING  |---ZL
             |_________________|     |_______|      |__________________|

    ************************************************************************/

   vector<complex<double>> delta = AmpS2P.S11*AmpS2P.S22 - AmpS2P.S21*AmpS2P.S12;
   vector<double> B1 = 1 + abs(AmpS2P.S11)*abs(AmpS2P.S11) - abs(AmpS2P.S22)*abs(AmpS2P.S22) - abs(delta)*abs(delta);
   vector<double> B2 = 1 + abs(AmpS2P.S22)*abs(AmpS2P.S22) - abs(AmpS2P.S11)*abs(AmpS2P.S11) - abs(delta)*abs(delta);
   vector<complex<double>> C1 = AmpS2P.S11 - delta*conj(AmpS2P.S22);
   vector<complex<double>> C2 = AmpS2P.S22 - delta*conj(AmpS2P.S11);
   complex<double> gamma_S, gamma_L;
   int ext_code = 0;
   Zin_maxg.resize(C1.size());
   Zout_maxg.resize(C1.size());
   //The sign of the square root must be chosen for each frequency so here we cannot apply vector operations
   for (unsigned int i = 0; i < C1.size(); i++)
   {
      gamma_S = (B1[i]-sqrt(B1[i]*B1[i] - 4.*abs(C1[i])*abs(C1[i])))/(2.*C1[i]);
      gamma_L = (B2[i]-sqrt(B2[i]*B2[i] - 4.*abs(C2[i])*abs(C2[i])))/(2.*C2[i]);

      if ((gamma_S != gamma_S) || (gamma_L != gamma_L))//Check if Nan
      {
          ext_code = -2;
          Zin_maxg[i]  = AmpS2P.Z0*(complex<double>(1,0)+conj(AmpS2P.S11.at(i)))/(complex<double>(1,0)-conj(AmpS2P.S11.at(i)));
          Zout_maxg[i] = AmpS2P.Z0*(complex<double>(1,0)+conj(AmpS2P.S22.at(i)))/(complex<double>(1,0)-conj(AmpS2P.S22.at(i)));
      }
      else
      {
         Zin_maxg[i]  = AmpS2P.Z0*(complex<double>(1,0)+conj(gamma_S))/(complex<double>(1,0)-conj(gamma_S));
         Zout_maxg[i] = AmpS2P.Z0*(complex<double>(1,0)+conj(gamma_L))/(complex<double>(1,0)-conj(gamma_L));
         continue;
      }
      if (real(Zin_maxg[i]) <= 0)
      {
        gamma_S = (B1[i]+sqrt(B1[i]*B1[i] - 4.*abs(C1[i])*abs(C1[i])))/(2.*C1[i]);
        Zin_maxg[i]  = ZS[i]*(complex<double>(1,0)+conj(gamma_S))/(complex<double>(1,0)-conj(gamma_S));
      }

      if (real(Zout_maxg[i]) <= 0)
      {
        gamma_L = (B2[i]+sqrt(B2[i]*B2[i] - 4.*abs(C2[i])*abs(C2[i])))/(2.*C2[i]);
        Zout_maxg[i] = ZL[i]*(complex<double>(1,0)+conj(gamma_L))/(complex<double>(1,0)-conj(gamma_L));
      }
   }

   //Notice that the equation below difer from the typical equations for two-port matching in the sense that
   //here ZS and ZL are used instead of Z0. Although in general the amplifier is matched to Z0=50 or 75\Ohm
   //this tool aims to treat the impedance matching problem in a more general fashion


   return ext_code;
}

MatchingData IO::getMatchingData()
{
   MatchingData data;
   data.freq = freq;
   data.ZS = ZS;
   data.ZL = ZL;
   data.Zin_maxg = Zin_maxg;
   data.Zout_maxg = Zout_maxg;
   return data;
}



vector<double> IO::getFrequency()
{
    return freq;
}


void IO::set_constant_ZS_vs_freq(complex<double> zs)
{
    ZS = vector<complex<double>>(1);
    ZS[0] = zs;
}


void IO::set_constant_ZL_vs_freq(complex<double> zl)
{
    ZL = vector<complex<double>>(1);
    ZL[0] = zl;
}


void IO::set_matching_band(double fmin, double fmax)
{
    fmatching_min = fmin;
    fmatching_max = fmax;
    freq = linspace(fmatching_min, fmatching_max, Nsamples);//Available freqs
}


// Gets the index of a given frequency
int IO::getFreqIndex(double f1)
{
    return closestIndex(freq, f1) ;
}



//Get freq scale from a string line
double IO::getS2PfreqScale(string line)
{
    if (line.find("ghz") != std::string::npos)
    {
        return 1e9;
    }
    else
    {
        if (line.find("mhz") != std::string::npos)
        {
            return 1e6;
        }
        else
        {
            if ((line.find("khz") != std::string::npos))
            {
                return 1e3;
            }
            else
            {
                return 1;//Hz
            }
        }
    }
}


// This function exports the best topology found to a Qucs schematic
int IO::ExportQucsSchematic(TwoPortCircuit TPC, string QucsFile)
{
    std::string wirestr = "";
    std::string componentstr = "";
    std::string paintingstr = "";
    int x_pos = 0;
    SchematicParser(TPC, x_pos, componentstr, wirestr, paintingstr);
    if (TPC.QucsVersion.empty()) TPC.QucsVersion = "0.0.19";
    CreateSchematic(componentstr, wirestr, paintingstr, TPC.QucsVersion, QucsFile);
    return 0;
}

//This function creates a s1p file given a constant impedance. It adds a unique point, the S-param simulator should understand that this is constant
void IO::generateConstant_s1p(string datapath, complex<double> Z)
{
  std::ofstream s1pFile(datapath.c_str(), ios_base::out);
  s1pFile << "# Hz Z RI R 1\n1000 " + Num2String(Z.real()) + " " + Num2String(Z.imag());
  s1pFile.close();
}

// Given a string code of inductors, capacitors and transmission lines, it generates the Qucs network. Notice that the schematic is split into
// three part: components, wires and paintings, all of them are passed by reference.
int IO::SchematicParser(TwoPortCircuit TPC, int & x_pos, string & componentstr, string & wirestr, string & paintingstr)
{
    //Clear input strings (just in case)
    componentstr = "";
    wirestr = "";
    paintingstr = "";
    bool term1 = false, term2 = false;//Flags the display equations

    // Schematic code
    // 0: Port 1 + S parameter simulation
    // 1: Port 1
    // 2: Port 2
    // 3: Port 1, Port 2 and S parameter simulation

    GRABIM_Result R = TPC.InputMatching;

    if ((R.source_path.empty()) && (abs(ZS.at(0).imag()) < 1e-3) && (ZS.at(0).real() > 1e-3))
    {//Conventional term
        componentstr += "<Pac P1 1 " + Num2String(x_pos) + " -30 18 -26 0 1 \"1\" 1 \"" + Num2String(ZS.at(0).real()) + " Ohm\" 1 \"0 dBm\" 0 \"1 GHz\" 0>\n";
        componentstr += "<GND * 1 " + Num2String(x_pos) + " 0 0 0 0 0>\n";

        wirestr += "<" + Num2String(x_pos) + " -60 " + Num2String(x_pos) + " -120 "" 0 0 0 "">\n";
        wirestr += "<" + Num2String(x_pos) +" -120 "+ Num2String(x_pos+120) +" -120 "" 0 0 0 "">\n";
        x_pos +=120;
        term1 = true;//Add S11_dB = dB(S[1,1]) equation
    }
    else
    {//Place a S-param file
        if (R.source_path.empty())
        {
          string s1p_path = tmp_path + "/ZS_data.s1p";
          generateConstant_s1p(s1p_path, ZS.at(0));
          R.source_path =  s1p_path;
        }
        componentstr += "<SPfile X1 1 " + Num2String(x_pos) + " -120 -26 -67 1 2 \"" + R.source_path + "\" 1 \"rectangular\" 0 \"linear\" 0 \"open\" 0 \"1\" 0>\n";
        componentstr += "<GND * 1 " + Num2String(x_pos) +" -90 0 0 0 0>\n";
        x_pos = 30;
        wirestr += "<" + Num2String(x_pos) + " -120 " + Num2String(x_pos+100) + " -120>\n";
        x_pos +=100;
    }

    //Draw ladder network
    CreateLadder(R, componentstr, wirestr, x_pos);
    if (!TPC.OutputMatching.freq.empty())
    {//Two-port network
        //Place S2P device
        x_pos += 30;
        componentstr += "<SPfile X1 1 " + Num2String(x_pos) + " -120 -26 -67 0 0 \"" + TPC.amplifier_path + "\" 1 \"rectangular\" 0 \"linear\" 0 \"open\" 0 \"2\" 0>\n";
        componentstr += "<GND * 1 " + Num2String(x_pos) +" -90 0 0 0 0>\n";
        x_pos += 30;
        wirestr += "<" + Num2String(x_pos+30) + " -120 " + Num2String(x_pos) + " -120>\n";
        R = TPC.OutputMatching;
        CreateLadder(R, componentstr, wirestr, x_pos);
    }

    int spacing = 30;
    x_pos += spacing;

    if ((R.load_path.empty()) && (abs(ZL.at(0).imag()) < 1e-3) && (ZL.at(0).real() > 1e-3))
    {//Conventional term
        componentstr += "<Pac P1 1 " + Num2String(x_pos) + " -30 18 -26 0 1 \"1\" 1 \"" + Num2String(ZL.at(0).real()) + " Ohm\" 1 \"0 dBm\" 0 \"1 GHz\" 0>\n";
        componentstr += "<GND * 1 "+Num2String(x_pos)+" 0 0 0 0 0>\n";

        wirestr += "<" + Num2String(x_pos) + " -60 " + Num2String(x_pos) + " -120 "" 0 0 0 "">\n";
        wirestr += "<" + Num2String(x_pos-spacing) + " -120 " + Num2String(x_pos) + " -120 "" 0 0 0 "">\n";
        term2 = true;//Add S22_dB = dB(S[2,2]) equation
    }
    else
    {//Place a S-param file
        if (R.load_path.empty())
        {
          string s1p_path = tmp_path + "/ZL_data.s1p";
          generateConstant_s1p(s1p_path, ZL.at(0));
          R.load_path =  s1p_path;
        }
        componentstr += "<SPfile X1 1 " + Num2String(x_pos) + " -120 -26 -67 0 0 \"" + R.load_path + "\" 1 \"rectangular\" 0 \"linear\" 0 \"open\" 0 \"1\" 0>\n";
        componentstr += "<GND * 1 " + Num2String(x_pos) + " -90 0 0 0 0>\n";
    }

    //Finally, add the S-par simulation block
    double fstart = fmatching_min*0.3;
    double fstop = fmatching_max*1.7;
    componentstr += "<.SP SP1 1 200 200 0 67 0 0 \"lin\" 1 \"" + Num2String(fstart) + "\" 1 \"" + Num2String(fstop) + "\" 1 \"300\" 1 \"no\" 0 \"1\" 0 \"2\" 0>\n";
   
    //Add equations
    string eqn = ""; 
    if (term1) eqn = "\"S11_dB=dB(S[1,1])\" 1 ";
    if (term2) eqn += "\"S22_dB=dB(S[2,2])\" 1 ";
    if (term1 && term2) eqn += "\"S21_dB=dB(S[2,1])\" 1 ";
    if (term1 || term2) componentstr += "<Eqn Eqn1 1 50 200 -28 15 0 0 " + eqn + "\"yes\" 0>\n";

    return 0;
}


//-----------------------------------------------------------------------------
// Given the components, wires and paintings, it creates the schematic and copies on the clipboard
bool IO::CreateSchematic(string components, string wires, string paintings, string QucsVersion, string QucsFilePath)
{
    //Header
    std::string Schematic = "<Qucs Schematic " + QucsVersion + ">\n";

    //Add components
    Schematic += "<Components>\n";
    Schematic += components;
    Schematic += "</Components>\n";

    //Add wires
    Schematic+= "<Wires>\n";
    Schematic += wires;
    Schematic+= "</Wires>\n";

    //Add paintings
    Schematic += "<Paintings>\n";
    Schematic += paintings;
    Schematic += "</Paintings>\n";

    if (QucsFilePath.empty())
    {
       QApplication::clipboard()->setText(QString(Schematic.c_str()), QClipboard::Clipboard);//Copy into clipboard
    }
    else//Dump content into a file
    {
       std::ofstream QucsFile(QucsFilePath.c_str(), ios_base::out);
       QucsFile << Schematic;
       QucsFile.close();
    }

    return true;
}



void IO::CreateLadder(GRABIM_Result R, string & componentstr, string & wirestr, int & x_pos)
{
    string component;
    int x_series = 150, x_shunt = 120;//x-axis spacing depending on whether the component is placed in a series or shunt configuration

    // The string format is as follows: "XX<value>;XX<value2>;...XX<valueN>;"
    // where XX, YY, ZZ define the type of component and its configuration.
    //    0: Series inductance
    //    1: Series capacitor
    //    2: Shunt inductance
    //    3: Shunt capacitor
    //    4: Series transmission line
    //    5: Open stub
    //    6: Short circuited stub

    int value_index = 0;
    for (unsigned int i = 0; i < R.topology.length(); i++)
    {
        component = R.topology.at(i);

        if (!component.compare("0"))//Series inductor
        {
            componentstr += "<L L1 1 " + Num2String(x_pos+60) + " -120 -26 10 0 0 \"" + Num2String(R.x_local_opt.at(value_index)) + "\" 1 \"\" 0>\n";
            wirestr +=  "<" + Num2String(x_pos) + " -120 " +  Num2String(x_pos+30) + " -120 \"\" 0 0 0 \"\">\n";
            wirestr += "<" + Num2String(x_pos+90) + " -120 " + Num2String(x_pos+x_series) + " -120 \"\" 0 0 0 \"\">\n";
            x_pos += x_series;
            value_index++;
        }
        else if (!component.compare("1"))//Series capacitor
        {
            componentstr += "<C C1 1 " + Num2String(x_pos+60) + " -120 -26 17 0 0 \"" + Num2String(R.x_local_opt.at(value_index)) + "\" 1 "" 0>\n";
            wirestr += "<" + Num2String(x_pos) + " -120 " + Num2String(x_pos+30) + " -120 \"\" 0 0 0 \"\">\n";
            wirestr += "<"+ Num2String(x_pos+90) +" -120 " + Num2String(x_pos+x_series) + " -120 \"\" 0 0 0 \"\">\n";
            x_pos += x_series;
            value_index++;
        }
        else if (!component.compare("2"))//Shunt inductor
        {
            componentstr += "<GND * 1 " + Num2String(x_pos) + " 0 0 0 0 0>\n";
            componentstr += "<L L1 1 " + Num2String(x_pos) + " -30 -40 40 0 1 \"" + Num2String(R.x_local_opt.at(value_index)) + "\" 1 \"\" 0>\n";
            wirestr += "<" + Num2String(x_pos) + " -60 " + Num2String(x_pos) +" -120 \"\" 0 0 0 \"\">\n";
            wirestr += "<" + Num2String(x_pos) + " -120 " + Num2String(x_pos+x_shunt) + " -120 \"\" 0 0 0 "">\n";
            x_pos += x_shunt;
            value_index++;
        }
        else if (!component.compare("3"))//Shunt capacitor
        {
            componentstr += "<GND * 1 " + Num2String(x_pos) + " 0 0 0 0 0>\n";
            componentstr += "<C C1 1 " + Num2String(x_pos) + " -30 -40 40 0 1 \"" + Num2String(R.x_local_opt.at(value_index)) + "\" 1 \"\" 0>\n";
            wirestr += "<" + Num2String(x_pos) +" -60 " + Num2String(x_pos) + " -120 \"\" 0 0 0 \"\">\n";
            wirestr += "<" + Num2String(x_pos) + " -120 " + Num2String(x_pos+x_shunt) + " -120 \"\" 0 0 0 "">\n";
            x_pos += x_shunt;
            value_index++;
        }
        else if (!component.compare("4"))//Transmission line
        {
            componentstr += "<TLIN Line1 1 " + Num2String(x_pos+60) + " -120 -26 20 0 0 \"" + Num2String(R.x_local_opt.at(value_index)) + "\" 1 \"" + Num2String(R.x_local_opt.at(value_index+1)) + "\" 1 \"0 dB\" 0 \"26.85\" 0>\n";
            wirestr += "<" + Num2String(x_pos) + " -120 " + Num2String(x_pos+30) + " -120 \"\" 0 0 0 \"\">\n";
            wirestr += "<" + Num2String(x_pos+90) + " -120 " + Num2String(x_pos+x_series) + " -120 \"\" 0 0 0 \"\">\n";
            x_pos += x_series;
            value_index+=2;
        }
        else if (!component.compare("5"))//Open stub
        {
            componentstr += "<TLIN Line1 1 " + Num2String(x_pos) + " -60 -26 20 0 1 \"" + Num2String(R.x_local_opt.at(value_index)) + "\" 1 \"" + Num2String(R.x_local_opt.at(value_index+1)) + "\" 1 \"0 dB\" 0 \"26.85\" 0>\n";
            wirestr += "<" + Num2String(x_pos) + "-90 " + Num2String(x_pos) + " -120 \"\" 0 0 0 \"\">\n";
            wirestr += "<" + Num2String(x_pos) + " -120 "+ Num2String(x_pos+x_shunt) + " -120 \"\" 0 0 0 \"\">\n";
            x_pos += x_shunt;
            value_index+=2;
        }
        else if (!component.compare("6"))//Short circuited stub
        {
            componentstr += "<TLIN Line1 1 " + Num2String(x_pos) + " -60 -26 20 0 1 \"" + Num2String(R.x_local_opt.at(value_index)) + "\" 1 \""+Num2String(R.x_local_opt.at(value_index+1))+"\" 1 \"0 dB\" 0 \"26.85\" 0>\n";
            componentstr += "<GND * 1 " + Num2String(x_pos) + " -30 0 0 0 0>\n";
            wirestr += "<" + Num2String(x_pos) +" -90 " + Num2String(x_pos) + " -120 \"\" 0 0 0 \"\">\n";
            wirestr += "<" + Num2String(x_pos) + "-120 " + Num2String(x_pos+x_shunt) + "-120 \"\" 0 0 0 \"\">\n";
            x_pos += x_shunt;
            value_index+=2;
        }

    }
}


string IO::Num2String(double Num)
{
  char c = 0;
  double cal = abs(Num);
  if(cal > 1e-20) {
    cal = log10(cal) / 3.0;
    if(cal < -0.2)  cal -= 0.98;
    int Expo = int(cal);

    if(Expo >= -5) if(Expo <= 4)
      switch(Expo) {
        case -5: c = 'f'; break;
        case -4: c = 'p'; break;
        case -3: c = 'n'; break;
        case -2: c = 'u'; break;
        case -1: c = 'm'; break;
        case  1: c = 'k'; break;
        case  2: c = 'M'; break;
        case  3: c = 'G'; break;
        case  4: c = 'T'; break;
      }

    if(c)  Num /= pow(10.0, double(3*Expo));
  }

  std::ostringstream s;
  s << Num;
  string Str = s.str();
  if(c)  Str += c;

  return Str;
}


string IO::Num2String(int x)
{
    std::ostringstream s;
    s << x;
    return s.str();
}




void IO::PrintNetwork_StandardOutput(GRABIM_Result Res)
{
    printf("\n+----SRC-----+");
    unsigned int index = 0;
    for(unsigned int i=0;i<Res.topology.size();i++, index++)
        {
                                          cout<<"\n|            |  ";
    if(!Res.topology.substr(i,1).compare("0"))cout<<"\n|            L  "<<Res.x_local_opt[index]*1E9 << "nH";
    if(!Res.topology.substr(i,1).compare("1"))cout<<"\n|            C  "<<Res.x_local_opt[index]*1E12<< "pF";
    if(!Res.topology.substr(i,1).compare("2"))cout<<"\n+-----L------+  "<<Res.x_local_opt[index]*1E9 << "nH";
    if(!Res.topology.substr(i,1).compare("3"))cout<<"\n+-----C------+  "<<Res.x_local_opt[index]*1E12<< "pF";
    if(!Res.topology.substr(i,1).compare("4"))cout<<"\n|            T  "<<
                                                "\n|            l  "<<Res.x_local_opt[index] << " " << Res.x_local_opt[index+1], index++;
    if(!Res.topology.substr(i,1).compare("5"))cout<<"\n|      oc+stub  "<<Res.x_local_opt[index] << " " << Res.x_local_opt[index+1], index++;
    if(!Res.topology.substr(i,1).compare("6"))cout<<"\n|      sc+stub  "<<Res.x_local_opt[index] << " " << Res.x_local_opt[index+1], index++;
                                          cout<<"\n|            |  ";
        }
    printf("\n+----LOAD----+\n\n");

    cout << "min(S11) = " << Res.nlopt_val << "dB" << endl;


    //LOG
    std::ofstream LogFile("log", ios_base::app);
    LogFile << Res.ZS.at(0) << " " << Res.ZS.at(round(2*Nsamples/5)-1) << " " << Res.ZS.at(round(3*Nsamples/5)-1) << " " << Res.ZS.at(round(4*Nsamples/5)-1) << " " << Res.ZS.at(Nsamples-1) << endl;
    LogFile << Res.ZL.at(0) << " " << Res.ZL.at(round(2*Nsamples/5)-1) << " " << Res.ZL.at(round(3*Nsamples/5)-1) << " " << Res.ZL.at(round(4*Nsamples/5)-1) << " " << Res.ZL.at(Nsamples-1) << endl;
    LogFile << min(Res.freq) << endl;
    LogFile << max(Res.freq) << endl;
    LogFile << Res.topology << endl;
    for (unsigned int i=0; i < Res.S11_gridsearch.size(); i++) LogFile << "(" << Res.S11_gridsearch.at(i).real() << "," <<  Res.S11_gridsearch.at(i).imag() << ") "<< endl;
    LogFile << Res.nlopt_val << endl;
    LogFile << "###" << endl;
    LogFile.close();
}


void IO::UseClipboard(bool B)
{
  CopyToClipboard=B;
}