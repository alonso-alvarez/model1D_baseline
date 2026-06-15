// include necessary packages
#include <unistd.h>
#include <stdio.h>  
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <math.h>
#include "metis.h"
#undef log
#undef exp
#define log(x) __builtin_log(x)
#define exp(x) __builtin_exp(x)
#include <time.h>
//#include <cblas.h>
#include <string.h>
#include "omp.h"

#ifdef USE_MPI
#include "mpi.h"

#endif

#include "linalg.h"
#include "model1d.h"
using namespace std;





int main (int argc, char *argv[]){//char** const argv) {

//  int DebugWait = 1;
//  while (DebugWait) ;
  Model1d pb;

  string input_file_name;
  std::stringstream sstm;

  int id = 0, p = 1;
  double wtime = 0.0;
#ifdef USE_MPI
  int thsupport;
  MPI_Init_thread (&argc, &argv, MPI_THREAD_FUNNELED, &thsupport);
  if (thsupport < MPI_THREAD_FUNNELED)
    {
     printf("The MPI library does not have minimal thread support\n");
     MPI_Abort (MPI_COMM_WORLD, 1);
   }

  MPI_Comm_rank (MPI_COMM_WORLD, &id);
  MPI_Comm_size (MPI_COMM_WORLD, &p);
  {
    int i = 1;
    while (i)
      {
	if (access("wait_for_debugger", F_OK ) != -1 ) { // file exists	  {
	    printf("Found file wait_for_debugger\n");
	    printf("task %d, PID %d ready for attach\n", id, getpid());
	    printf("delete/rename file or use: 'set var i = 0'"
		   " in gdb to continue\n");
	    sleep (5);
	  }
	else
	  i = 0;
      }
  // delete/rename file or use: "set var i = 0" in gdb to continue
  }
#endif

#ifdef USE_MPI
  if ( id == 0 ) {
    wtime = MPI_Wtime();
  }
#endif

  // define process ID and number of processes 
  pb.partitionID = id;
  pb.nproc = p;



  if ( id == 0 && p>1) { 
	Model1d pbGlob;
	pbGlob.partitionID = id;
  	pbGlob.nproc = p;
    input_file_name = argv[1];
    // initialize model
    pbGlob.init(input_file_name);
    // partition graph
    pbGlob.metisPartGraphClosedLoop(p);
    // cout << "Partitioning done, time: " << MPI_Wtime() - wtime << " s" << endl;
	pbGlob.end();
  }

cout << id << " Partitioning done, waiting for all processes ..." << endl;
#ifdef USE_MPI
  cout << id << " Waiting for partitioning to be done ..." << endl;
  MPI_Barrier(MPI_COMM_WORLD);
#endif
  
  cout << id << " Reading partitioned model ..." << endl;
  if(p>1){
    sstm << "input1d_" << id <<".dat";  
    input_file_name = sstm.str();
  }
  else{
    input_file_name = argv[1];
  }

  //cout << sstm.str() << endl;
	cout << id << " Initializing model ..." << endl;
  // initialize model
  pb.verbose = 1;

  pb.init(input_file_name);

  ofstream dts;
  string dtsName = pb.outDir+"dts.txt";
  dts.open(dtsName.c_str());

  for(int i = 0; i< pb.NV; i++)
    dts << i+1 << " " << pb.vess[i].L << " " << pb.vess[i].dt << endl;
  dts.close();

#ifdef USE_MPI
  if ( id == 0 ) {
    wtime = MPI_Wtime();
  }
#endif
  // enter time loop
#ifdef USE_MPI
  MPI_Barrier(MPI_COMM_WORLD);
#endif
  pb.iT = 0;

  // double start = omp_get_wtime();
 // if(pb.NV>0){
    for(int iter = 0; iter < 10000000000000; iter++){
      // solve time step
      pb.solveTimeStep(pb.dtMaxLTSLIMIT);
      // pb.iT += 1;
      if (pb.endOfSimulation==1)
	break;
    }
    cout << id << " Time loop done, waiting for all processes ..." << endl;
 // }
// double end = omp_get_wtime();
	//cout << "#######################" << endl;
//cout << "OMP CPU time: " << end-start;

  if ( id == 0 ){
    cout << "#######################" << endl;
#ifdef USE_MPI
    wtime = MPI_Wtime() - wtime;
#else
    wtime = 0.0;
#endif
    cout << "\n";       
    cout << "  Wall clock elapsed seconds = " << wtime << "\n";
    cout << "#######################" << endl;      
  }
  // free memory
  pb.end();

  cout << id << " END." << endl;
  //int date;
  //date = system("date");
#ifdef USE_MPI
  MPI_Finalize();
#endif
  return 0;
}
