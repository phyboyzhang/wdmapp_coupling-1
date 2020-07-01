#include "dataprocess.h"
#include "BoundaryDescr3D.h"
#include "couplingTypes.h"
#include "importpart3mesh.h"
#include "commpart1.h"
#include "sendrecv_impl.h"
#include <mpi.h>
#include <math.h>

namespace coupler{


//fix me: lowzpart3 and uppart3 will be sparated. 
// In the wdm/app(cuda_under_hood) the parallel boundary condition is not used for the potential.
void DatasProc3D::zPotentBoundaryBufAssign(
    const BoundaryDescr3D& bdesc,
    const Part3Mesh3D& p3m3d,
    const Part1ParalPar3D &p1pp3d)
{
  LO nzb=bdesc.nzb;
  if(bdesc.lowpotentz==NULL||bdesc.uppotentz==NULL){
    std::cout<<"ERROR:the boundary buffer of the potential must be allocated beforing invoking this routine.";
    std::exit(EXIT_FAILURE);
  }
  LO li0,lj0,lk0;
  li0=p3m3d.xboxinds[p1pp3d.mype_x][0];
  lj0=p3m3d.lj0/2;
  if(p1pp3d.npz>1){
    if(p1pp3d.periods[2]==1){
      for(LO i=0;i<li0;i++){
        lk0=p3m3d.mylk0[i];
        if(lk0<nzb){
          std::cout<<"ERROR: the interpolation order is larger than the box count along z dimension.";
          std::exit(EXIT_FAILURE);
        }
        mpisendrecv_aux1D(p1pp3d.comm_z,nzb,li0,lj0,lk0,bdesc.lowzpart3[i],bdesc.upzpart3[i],
          p3m3d.pzcoords[i]);

       //for test debugging
       if(testcase==TestCase::t0){
         if(p1pp3d.mype_z==0){
             std::cout<<"bdesc.lowzpart3="<<bdesc.lowzpart3[i][0]<<" "<<bdesc.lowzpart3[i][1]<<'\n';
             std::cout<<"lowpzcoords="<<p3m3d.pzcoords[i][0]<<" "<<p3m3d.pzcoords[i][1]<<'\n';

         } else if(p1pp3d.mype_z==p1pp3d.npz-1){
            std::cout<<"upzpart3="<<bdesc.upzpart3[i][0]<<" "<<bdesc.upzpart3[i][1]<<'\n';
            std::cout<<"upzcoords="<<p3m3d.pzcoords[i][lk0-2]<<" "<<p3m3d.pzcoords[i][lk0-1]<<'\n';
         }
       }

       for(LO j=0;j<lj0;j++){
          mpisendrecv_aux1D(p1pp3d.comm_z,nzb,li0,lj0,lk0,bdesc.lowpotentz[i][j],bdesc.uppotentz[i][j],
              potentinterpo[i][j]);
        //enforce the parallel boundary condition
          if(p1pp3d.mype_z==0){
            for(LO k=0;k<nzb;k++)
              bdesc.lowpotentz[i][j][k]=bdesc.lowpotentz[i][j][k]*bdesc.lowpbmat[i][j];
          } else if(p1pp3d.mype_z==p1pp3d.npz-1){
             for(LO k=0;k<nzb;k++)
               bdesc.uppotentz[i][j][k]=bdesc.uppotentz[i][j][k]*bdesc.uppbmat[i][j];
          }
       }
     }

     if(p1pp3d.mype_z==0){
         for(LO h=0;h<li0;h++){
           for(LO k=0;k<nzb;k++){
             bdesc.lowzpart3[h][k]=bdesc.lowzpart3[h][k]-2.0*cplPI;
           }
         }
     }else if(p1pp3d.mype_z==p1pp3d.npz-1){
         for(LO h=0;h<li0;h++){
           for(LO k=0;k<nzb;k++){
              bdesc.upzpart3[h][k]=bdesc.upzpart3[h][k]+2.0*cplPI;
           }
         }
     }
     //for test debugging
     if(testcase==TestCase::t0){
         if(p1pp3d.mype_z==0){
           for(LO k=0;k<li0;k++){
             std::cout<<"lowzpart3["<<k<<"][1]="<<bdesc.lowzpart3[k][1]<<'\n';
           }
         }else if(p1pp3d.mype_z==p1pp3d.npz-1){
            for(LO k=0;k<li0;k++){
              std::cout<<"bdesc.upzpart3["<<k<<"][1]="<<bdesc.upzpart3[k][1]<<'\n';
            }
         }
      }
    } else if(p1pp3d.periods[2]==0){
         std::cout<<"The parallelization of 'z' domain is not down with unperiodic boundary condiiton"
                  <<" and npz>1"<<'\n';
         std::exit(EXIT_FAILURE);
    }
  } else {
      if(p1pp3d.periods[2]==1){
        for(LO i=0;i<li0;i++){
           lk0=p3m3d.mylk0[i];
           if(lk0<nzb){
             std::cout<<"ERROR: the interpolation order is larger than the box count along z dimension.";
             std::exit(EXIT_FAILURE);
           }
           for(LO k=0;k<nzb;k++){
             bdesc.lowzpart3[i][k]=p3m3d.pzcoords[i][lk0-nzb+k];
             bdesc.upzpart3[i][k]=p3m3d.pzcoords[i][k];
           }
           for(LO j=0;j<lj0;j++){
             for(LO k=0;k<nzb;k++){
               bdesc.uppotentz[i][j][k]=potentin[i][j][k];
               bdesc.lowpotentz[i][j][k]=potentin[i][j][lk0-nzb+k];
             }
           }
        }
     } else if(p1pp3d.periods[2]==0) {
         std::cout<<"The parallelization of 'z' domain is not down with unperiodic boundary condiiton"
                  <<" and npz=1"<<'\n';
         std::exit(EXIT_FAILURE);
      }
   }
}



void DatasProc3D::zDensityBoundaryBufAssign(
    CV*** box,
    const BoundaryDescr3D& bdesc,
    const Part1ParalPar3D& p1pp3d) 
{
  LO nzb=bdesc.nzb;
  if (bdesc.lowdenz == NULL || bdesc.updenz == NULL) {
    std::cout << "ERROR:the boundary buffer must be alloctted before "
                 "calling this routine.";
    std::exit(EXIT_FAILURE);
  }
  const LO lx = p1pp3d.li0;
  const LO ly = p1pp3d.lj0;
  const LO lz = p1pp3d.lk0;
  if (p1pp3d.npz > 1) {
    if (lz >= nzb) {
/*
if(p1pp3d.mype==0){
for(int i=0;i<lx;i++){
  for(int j=0;j<ly;j++){
    for(int k=0;k<nzb;k++)
     std::cout<<"i,j="<<i<<" "<<j<<" "<<box[i][j][k]<<'\n';
  }
}
}
*/

      mpisendrecv_aux2D(p1pp3d.comm_z, nzb, lx, ly, lz, bdesc.lowdenz, bdesc.updenz, box);
//fix the parallel boundary condition here.
    } else {
      std::cout << "ERROR: nzb is larger than lz. A larger lz is required.";
      std::exit(EXIT_FAILURE);
    }
  } else {
    if (p1pp3d.periods[2] == 1) {
      for (LO i = 0; i < lx ; i++) {
        for (LO j = 0; j < ly; j++) {
          for (LO k = 0; k < nzb; k++) {
            bdesc.lowdenz[i][j][k] = box[i][j][lz - nzb + k];
            bdesc.updenz[i][j][k] = box[i][j][k];
            if(p1pp3d.mype_z==0){
              bdesc.lowdenz[i][j][k]=bdesc.lowdenz[i][j][k]*bdesc.lowpbmat[i][j];
            } else if(p1pp3d.mype_z==p1pp3d.npz-1){
              bdesc.updenz[i][j][k]=bdesc.updenz[i][j][k]*bdesc.uppbmat[i][j];
            }
          }
        }
      }
    } else {
      std::cout << "The topology is not right." << '\n';
      std::exit(EXIT_FAILURE);
    }
  }
}


}

