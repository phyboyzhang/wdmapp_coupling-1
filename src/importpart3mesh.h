#ifndef IMPORTPART3MESH_H
#define IMPORTPART3MESH_H

#include "couplingTypes.h"
#include "testutilities.h"
#include "adios2Routines.h"
#include "commpart1.h"

namespace coupler {

//forward declare the Part1ParalPar3D class; we don't care how 
// the class is defined in this header since we don't call
// any of its functions or access its member variables

template<class T>
class Array2d;

class Part1ParalPar3D;

class Part3Mesh3D{
  public:
/*variables of XGC for both gene-xgc and gem-xgc coupling*/
    LO nsurf;  // number of flux surfaces on the coupled domain
    LO npsi_surf; // The notal number of surfaces provided by XGC
    LO block_count;  // number of nodes
    LO* versurf = NULL; // numbers of vertices on the flux surfaces locating on the part1 domain.
    double* xcoords = NULL;
    LO  li0,li1,li2;
    LO** xboxinds=NULL;  //The indexes of all boxes on the radial dimension
    LO lj0;
    LO* mylk0=NULL;
    LO* mylk1=NULL;
    LO* mylk2=NULL; // The indexes of box along z dimension
    LO* nstart; // Store the the index of the minimal element of the array of 
                // the z coordinates on each cross section

//  temporary members which would be deleted in the next time refactor
    double* zcoordall = NULL; //zcoordall will be removed in the next refacotr.
    int* cce = NULL; //store cce_ variables only


    // parameters for receiving and sending global 2d arrays
    GO  blockstart,blockend,blockcount; // The  indexes of the 2d box global in z dimension
    LO  cce_first_surface; // The number of the first flux surface where GENE begins.
    LO  cce_last_surface;  // The number of the last flux surface where GENE ends.
    GO  cce_first_node; // The number of the first active node sent by part3;
    GO  cce_last_node; // The number of the last active node send by part3;
    GO  cce_node_number;  // The number of active nodes sent by part3.
    GO  totnodes; // the total number of nodes sent by part3;
    GO  activenodes; // The number of nodes part1 has on the poloidal cross section. 
    LO  shiftx;  // The number of surfaces shifting from part3 first surface to part1 first surface.

    // paramters to create the background of mgnetic field, density and temperature profile
   
    double** pzcoords=NULL;  // The z coordinates of all points within the 2d box.
    double** zcoordsurf=NULL;


/*------------------------------------------------------*/
/*variables specially owned by XGC for GEM-XGC coupling*/

    /* constructor - versurf has length = numsurf & versurf[i] = the number of nodes surface[i]
     * xcoords saves the radial coordinate of each surface.
     * zcoords saves the poloidal angle of each node on each surface.
     */

    Part3Mesh3D(Part1ParalPar3D &p1pp3d,
        LO nsurf_,
        LO block_count_,
        LO* versurf_,
        int* cce_,
        double* xcoords_,
        double* zcoord_,
        bool pproc = true)
      : nsurf(nsurf_),
        block_count(block_count_),
        versurf(versurf_),
        cce(cce_),
        xcoords(xcoords_),
        zcoordall(zcoord_),
	preproc(pproc), test_case(TestCase::off) {
        init(p1pp3d);
    }
    /* testing constructor
     * arguments support reading
     * the test case number and directory
     * from the user
     */
    Part3Mesh3D(Part1ParalPar3D &p1pp3d,
        bool pproc,
        TestCase tcase,
        std::string tdir)
      : preproc(pproc), test_case(tcase) {
      assert(tcase < TestCase::invalid);
      init(p1pp3d,tdir);
    }

    ~Part3Mesh3D()
    {
     if(versurf!=NULL) delete[] versurf;
     if(xboxinds!=NULL) delete[] xboxinds;
     if(xcoords!=NULL) delete[] xcoords;
     if(mylk0!=NULL) delete[] mylk0;
     if(mylk1!=NULL) delete[] mylk1;
     if(mylk2!=NULL) delete[] mylk2;
//     if(Rcoordall!=NULL) delete[] Rcoordall;
//     if(Zcoordall!=NULL) delete[] Zcoordall;
     if(pzcoords!=NULL) delete[] pzcoords;
   }

  private:
    const bool preproc;
    const TestCase test_case;
    class Part1ParalPar3D* p1;
    void init(const Part1ParalPar3D &p1pp3d, const std::string test_dir="");
//    void initXgcGem(Array2d<int>* xgcnodes,Array2d<double>* rzcoords);
    /* helper function called by init */
    void DistriPart3zcoords(const Part1ParalPar3D  &p1pp3d,
        const std::string test_dir="");
    /* helper function called by DistriPart3zcoords */
    void DistributePoints(const double* exterarr, const LO gstart, const LO li,
        const double* interarr, const Part1ParalPar3D  &p1pp3d);
    void BlockIndexes(const MPI_Comm comm_x,const LO mype_x,const LO npx);
    void  JugeFirstSurfaceMatch(double xp1);
/*
    inline LO search_zeta(const double dlength,const double length,const LO nlength,double tmp);
    inline struct flxxgc* search_flux_3rdorder_periodbound(double tmpflx,const double* flxin, LO num);
    void search_y(LO j1,LO j2,double w1,double w2,const double dy,const double ly,const double tmp); 
*/ 

    /* default constructor 
     * put this in private to prevent users from calling it
     */
    Part3Mesh3D() : preproc(true), test_case(TestCase::off) {};
};


// The following utility functions should go into a header for utilities if they
// need to be called by other other classes or code.
// If they are not needed elsewhere, the prototypes/definitions below should
// be removed.
LO  minloc(const double* zcoords, const LO n); 

template<class T>
void reshuffleforward(T* array,const LO nstart,const LO vertnum)
{
  T* tmp=new T[vertnum];
  for(LO i=0;i<vertnum-nstart;i++)
    tmp[i]=array[nstart+i];
  for(LO j=vertnum-nstart;j<vertnum;j++)
    tmp[j]=array[j-vertnum+nstart];
  for(LO k=0;k<vertnum;k++)
    array[k]=tmp[k];
  delete[] tmp;
}


template<class T>
void reshufflebackward(T* array,const LO nstart,const LO vertnum)
{
  T* tmp=new T[vertnum];
  for(LO i=vertnum-nstart;i<vertnum;i++)
    tmp[i-vertnum+nstart]=array[i];
  for(LO j=0;j<vertnum-nstart;j++)
    tmp[j+nstart]=array[j];
  for(LO k=0;k<vertnum;k++)
    array[k]=tmp[k];
  delete[] tmp;
}

double minimalvalue(const double* array, const LO n);

}

#endif
