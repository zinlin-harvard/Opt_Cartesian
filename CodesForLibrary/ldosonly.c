#include <petsc.h>
#include <time.h>
#include "libOPT.h"
#include <complex.h>
#include "petsctime.h"

#define Ptime PetscTime
#define BLOCH 1

//define Global variables;
extern int Nxyz, count;
extern double hxyz;
extern Mat B, C, D;
extern Vec vR, weight;
extern Vec vgradlocal;
extern Vec epsSReal;
IS from, to;
VecScatter scatter;
extern char filenameComm[PETSC_MAX_PATH_LEN];
extern int outputbase;

#if BLOCH == 0

#undef __FUNCT__ 
#define __FUNCT__ "ldosonly"
double ldosonly(int DegFree,double *epsopt, double *grad, void *data)
{
  
  PetscErrorCode ierr;

  PetscPrintf(PETSC_COMM_WORLD,"********Entering the LDOS solver (Full Vectorial Version). Minimum approach NOT available.********** \n");

  LDOSdataGroup *ptdata = (LDOSdataGroup *) data;

  double omega = ptdata->omega;
  Mat M = ptdata->M;
  Mat A = ptdata->A;
  Vec x = ptdata->x;
  Vec b = ptdata->b;
  Vec weightedJ = ptdata->weightedJ;
  Vec epspmlQ  = ptdata->epspmlQ;
  Vec epsmedium = ptdata->epsmedium;
  Vec epsDiff = ptdata->epsDiff;
  int *its = ptdata->its; 
  Vec epscoef = ptdata->epscoef;  
  Vec vgrad = ptdata->vgrad;
  KSP ksp = ptdata->ksp;
  
  //declare temporary variables
  Vec epsC, epsCi, epsP, tmp, Grad;
  Mat tmpM;

  PetscPrintf(PETSC_COMM_WORLD,"==========************!!!!!!!!!!omega debug check in ldosonly function is %g *********======\n",omega);
  
  VecDuplicate(x,&epsC);
  VecDuplicate(x,&epsCi);
  VecDuplicate(x,&epsP);
  VecDuplicate(x,&tmp);
  VecDuplicate(x,&Grad);

  // copy epsopt to epsSReal, fills the DegFree elements;
  ierr=ArrayToVec(epsopt, epsSReal); CHKERRQ(ierr);
  
  // Update the diagonals of M;
  MatDuplicate(M,MAT_COPY_VALUES,&tmpM);
  VecSet(epsP,0.0);
  ModifyMatDiagonals(tmpM, A, D, epsSReal, epspmlQ, epsmedium, epsC, epsCi, epsP, Nxyz, omega, epsDiff);

  /*-----------------KSP Solving------------------*/   
  SolveMatrix(PETSC_COMM_WORLD,ksp,tmpM,b,x,its);

  /*-------------Calculate and print out the LDOS----------*/
  //tmpldos = -Re((wt.*J^*)'*E) 
  double tmpldosr, tmpldosi, ldos;
  CmpVecDot(x,weightedJ,&tmpldosr,&tmpldosi);
  ldos=-1.0*hxyz*tmpldosr;
  PetscPrintf(PETSC_COMM_WORLD,"---*****The current ldos for omega %.4e at step %.5d is %.16e \n", omega/(2*PI),count,ldos);

  if (grad) {
    // Derivative of LDOS wrt eps = Re [ x^2 wt I/omega epscoef ];
    CmpVecProd(x,x,Grad);
    CmpVecProd(Grad,epscoef,tmp);
    ierr = MatMult(D,tmp,Grad); CHKERRQ(ierr);
    ierr = VecPointwiseMult(Grad,Grad,weight); CHKERRQ(ierr);
    VecScale(Grad,1.0*hxyz/omega);
    ierr = VecPointwiseMult(Grad,Grad,vR); CHKERRQ(ierr);

    ierr = MatMultTranspose(A,Grad,vgrad);CHKERRQ(ierr);
    ierr = VecToArray(vgrad,grad,scatter,from,to,vgradlocal,DegFree);

  }

  char buffer [100];
  int STORE=1;
  if(STORE==1 && (count%outputbase==0))
    {
      sprintf(buffer,"%.5depsSReal.m",count);
      OutputVec(PETSC_COMM_WORLD, epsSReal, filenameComm, buffer);
    }
  
  ierr = MatDestroy(&tmpM); CHKERRQ(ierr);

  VecDestroy(&epsC);
  VecDestroy(&epsCi);
  VecDestroy(&epsP);
  VecDestroy(&tmp);
  VecDestroy(&Grad);

  count++;
  return ldos;
}

#endif

#if BLOCH == 1

#undef __FUNCT__ 
#define __FUNCT__ "ldosonly"
double ldosonly(int DegFree,double *epsopt, double *grad, void *data)
{
  
  PetscErrorCode ierr;

  PetscPrintf(PETSC_COMM_WORLD,"********Entering the LDOS solver (Full Vectorial Version). Minimum approach NOT available.********** \n");
  PetscPrintf(PETSC_COMM_WORLD,"********NOTE!!!! this version handles the Bloch boundaries with transpose adjoint solution.*********** \n");

  LDOSdataGroup *ptdata = (LDOSdataGroup *) data;

  double omega = ptdata->omega;
  Mat M = ptdata->M;
  Mat A = ptdata->A;
  Vec x = ptdata->x;
  Vec b = ptdata->b;
  Vec weightedJ = ptdata->weightedJ;
  Vec epspmlQ  = ptdata->epspmlQ;
  Vec epsmedium = ptdata->epsmedium;
  Vec epsDiff = ptdata->epsDiff;
  int *its = ptdata->its; 
  Vec epscoef = ptdata->epscoef;  
  Vec vgrad = ptdata->vgrad;
  KSP ksp = ptdata->ksp;
  
  //declare temporary variables
  Vec epsC, epsCi, epsP, tmp, Grad;
  Mat tmpM;

  PetscPrintf(PETSC_COMM_WORLD,"==========************!!!!!!!!!!omega debug check in ldosonly function is %g *********======\n",omega);
  
  VecDuplicate(x,&epsC);
  VecDuplicate(x,&epsCi);
  VecDuplicate(x,&epsP);
  VecDuplicate(x,&tmp);
  VecDuplicate(x,&Grad);

  // copy epsopt to epsSReal, fills the DegFree elements;
  ierr=ArrayToVec(epsopt, epsSReal); CHKERRQ(ierr);
  
  // Update the diagonals of M;
  MatDuplicate(M,MAT_COPY_VALUES,&tmpM);
  VecSet(epsP,0.0);
  ModifyMatDiagonals(tmpM, A, D, epsSReal, epspmlQ, epsmedium, epsC, epsCi, epsP, Nxyz, omega, epsDiff);

  /*-----------------KSP Solving------------------*/   
  SolveMatrix(PETSC_COMM_WORLD,ksp,tmpM,b,x,its);

  /*-------------Calculate and print out the LDOS----------*/
  //tmpldos = -Re((wt.*J^*)'*E) 
  double tmpldosr, tmpldosi, ldos;
  CmpVecDot(x,weightedJ,&tmpldosr,&tmpldosi);
  ldos=-1.0*hxyz*tmpldosr;
  PetscPrintf(PETSC_COMM_WORLD,"---*****The current ldos for omega %.4e at step %.5d is %.16e \n", omega/(2*PI),count,ldos);

  Vec u,Urhs;
  VecDuplicate(x,&u);
  VecDuplicate(x,&Urhs);
  
  if (grad) {
    //grad = -hxyz Re[ u epscoef x]
    //M^T u = conj(weightedJ);
    VecCopy(weightedJ,Urhs);
    KSPSolveTranspose(ksp,Urhs,tmp);
    MatMult(C,tmp,u);

    CmpVecProd(x,epscoef,tmp);
    CmpVecProd(tmp,u,Grad);
    VecPointwiseMult(Grad,Grad,vR);
    VecScale(Grad,-1.0*hxyz);    

    ierr = MatMultTranspose(A,Grad,vgrad);CHKERRQ(ierr);
    ierr = VecToArray(vgrad,grad,scatter,from,to,vgradlocal,DegFree);

  }

  char buffer [100];
  int STORE=1;
  if(STORE==1 && (count%outputbase==0))
    {
      sprintf(buffer,"%.5depsSReal.m",count);
      OutputVec(PETSC_COMM_WORLD, epsSReal, filenameComm, buffer);
    }
  
  ierr = MatDestroy(&tmpM); CHKERRQ(ierr);

  VecDestroy(&epsC);
  VecDestroy(&epsCi);
  VecDestroy(&epsP);
  VecDestroy(&tmp);
  VecDestroy(&Grad);
  VecDestroy(&u);
  VecDestroy(&Urhs);
  
  count++;
  return ldos;
}

#endif

