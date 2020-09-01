/***********************************************************************************
 * Project: S-QuAPI for CPU
 * Major version: 0
 * version: 0.1.0 (OpenMP, made for multi-core single node)
 * Date Created : 8/15/20
 * Date Last mod: 8/26/20
 * Author: Yoshihiro Sato
 * Description: the main function of squapi_omp 
 * Notes: 
 *      - Functions used in this program are written in sqmodule.cpp and sqmodule_omp.cpp
 *      - Based on C++11
 *      - Develped using gcc 10.2.0 on MacOS 10.14 
 *      - size of Cn has to be lower than 2,147,483,647 (limit of int)
 * Copyright (C) 2020 Yoshihiro Sato - All Rights Reserved
 **********************************************************************************/
#include <iostream>
#include <iomanip>
#include <fstream>
#include <complex>
#include <vector>
#include <deque>
#include <unordered_map>
#include <string>
#include <omp.h>
#include "sqmodule.h"
#include "sqmodule_omp.h"
#include "cont.h"


int main(int argc, char* argv[])
{
    double time0 = omp_get_wtime(); // for time measurement using OMP
    
    // --- global S-QuAPI variables:
    std::vector<std::complex<double>> Dtc;
    std::vector<std::complex<double>> energy;
    std::vector<std::complex<double>> eket;
    std::vector<std::complex<double>> U;
    std::vector<std::vector<std::complex<double>>> s;
    std::vector<std::complex<double>> gm0;
    std::vector<std::complex<double>> gm1;
    std::vector<std::vector<std::complex<double>>> gm2;
    std::vector<std::vector<std::complex<double>>> gm3;
    std::vector<std::vector<std::complex<double>>> gm4;
    std::vector<std::complex<double>> rhos0;
    std::vector<std::complex<double>> D;
    int Nmax, Dkmax, M, N0; // N0 is not used in genrhos_omp.cpp
    double Dt, theta;

    // load data from files and store them in the S-QuAPI parameters: 
    load_data(argv, energy, eket, U, s, 
              gm0, gm1, gm2, gm3, gm4, rhos0, Nmax, Dkmax, M, Dt,theta);

    // --- generate U for the propagators
    getU(Dt, energy, eket, U);  

    // -------- load S-QuAPI parameters -------------------------
    std::cout << "----- Date and Time --------------------" << std::endl;
    std::system("date");
    std::cout << "----- parameters -----------------------" << std::endl;
    std::cout << "Dt     = " << Dt << " ps" << std::endl;
    std::cout << "Nmax   = " << Nmax   << std::endl;
    std::cout << "Dkmax  = " << Dkmax  << std::endl;
    std::cout << "M      = " << M      << std::endl;
    std::cout << "theta  = " << theta  << std::endl;
    
    /******************* STEP 1 **************************/
    // -------- Generate the paths and weights -------------------------
    std::cout << "----- generate Cn and Wn ---------------" << std::endl;
    std::vector<std::vector<unsigned long long>>   C;
    std::vector<std::vector<std::complex<double>>> W;
    getCW_omp(theta, U, s, gm0, gm1, gm2, gm3, gm4, C, W);
    
    // --- generate Cnmap ------------
    std::cout << "----- generate Cnmap -------------------" << std::endl;
    std::unordered_map<unsigned long long, int> Cnmap;
    getCnmap(C[Dkmax], Cnmap);
    //getCnmap_omp(C[Dkmax], Cnmap); // very slow. for devel only 

    // -------- Generate rhos ---------------------------------------
    std::cout << "----- generate rhos --------------------" << std::endl;

    // ***** Start time evolution **************
    for (int N = 0; N < Nmax + 1; N++){
        int n;
        std::vector<std::complex<double>> rhos(M * M);
        double time1 = omp_get_wtime(); // for time measurement using OMP
        if (N == 0){
            rhos = rhos0;
        }
        else if (N > 0 && N < Dkmax + 1){
            /******************* STEP 2 **************************/
            n = N;
            getD0_omp(N, C[n], rhos0, U, s, gm0, gm1, gm2, gm3, gm4, D);
            getrhos_omp(N, U, C[n], W[n], D, s, gm0, gm1, gm2, gm3, gm4, rhos);
        }
        else if (N == Dkmax + 1){
            /******************* STEP 3 **************************/
            n = Dkmax + 1;
            getD0_omp(N, C[n-1], rhos0, U, s, gm0, gm1, gm2, gm3, gm4, D);
            getrhos_omp(N, U, C[n-1], W[n-1], D, s, gm0, gm1, gm2, gm3, gm4, rhos);
        }
        else{
            /******************* STEP 4 **************************/
            n = Dkmax + 1;
            getD1_omp(C[n-1], Cnmap, U, s, gm0, gm1, gm2, gm3, gm4, D);
            getrhos_omp(N, U, C[n-1], W[n-1], D, s, gm0, gm1, gm2, gm3, gm4, rhos);
        }
        // *** write N and rhos into rhos.dat ***
        save_rhos(N, rhos);

        std::cout << "N = " << N << " of " << Nmax;
        std::cout << " lap time = " << omp_get_wtime() - time1 << " sec"; 
        std::cout << " tr = " << trace(rhos) << std::endl;

        // --- save D to D.dat for cont 
        if (N == Nmax){
            double time2 = omp_get_wtime();
            std::cout << "----- saving D to D.dat ----------------" << std::endl;
            save_D (N, theta, D);
            std::cout << "    lap time = " << omp_get_wtime() - time2 << " sec" << std::endl;
            /***********************************************
            // store backup files in zip (optional)
            std::system(
                    "now=`date '+%Y_%m_%d_%H%M%S'`;\
                    zip -rq D.dat.$now.zip D.dat;\
                    zip -rq rhos.dat.$now.zip rhos.dat;");  
            ************************************************/
        }

    }

    std::cout << "----- end -----------------------------" << std::endl;
    std::cout << "    elapsed_time = " << omp_get_wtime() - time0 << " sec" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

}

//=======================  EOF  ================================================
