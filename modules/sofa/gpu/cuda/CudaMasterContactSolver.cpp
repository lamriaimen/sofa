/******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, version 1.0 beta 4      *
*                (c) 2006-2009 MGH, INRIA, USTL, UJF, CNRS                    *
*                                                                             *
* This library is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This library is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this library; if not, write to the Free Software Foundation,     *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.          *
*******************************************************************************
*                               SOFA :: Modules                               *
*                                                                             *
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#include "CudaMasterContactSolver.h"

#include <sofa/simulation/common/AnimateVisitor.h>
#include <sofa/simulation/common/BehaviorUpdatePositionVisitor.h>
#include <sofa/simulation/common/MechanicalVisitor.h>
#include <sofa/simulation/common/SolveVisitor.h>

#include <sofa/helper/LCPcalc.h>
#include <sofa/core/ObjectFactory.h>

#include <sofa/helper/system/thread/CTime.h>
#include <math.h>
#include <iostream>

namespace sofa
{

namespace component
{

namespace odesolver
{

using namespace sofa::defaulttype;
using namespace helper::system::thread;
using namespace core::componentmodel::behavior;

static unsigned MAX_NUM_CONSTRAINTS=2048;

template<class real>
CudaMasterContactSolver<real>::CudaMasterContactSolver()
    :
    useGPU_d(initData(&useGPU_d,-1, "useGPU", "compute LCP using GPU"))
#ifdef DISPLAY_TIME
    ,print_info(initData(&print_info, false, "print_info", "Print infos"))
#endif
    ,initial_guess(initData(&initial_guess, true, "initial_guess","activate LCP results history to improve its resolution performances."))
    ,tol( initData(&tol, 0.001, "tolerance", ""))
    ,maxIt( initData(&maxIt, 1000, "maxIt", ""))
    ,mu( initData(&mu, 0.6, "mu", ""))
    , constraintGroups( initData(&constraintGroups, "group", "list of ID of groups of constraints to be handled by this solver.") )
    ,_mu(0.6)
{
    _W.resize(MAX_NUM_CONSTRAINTS,MAX_NUM_CONSTRAINTS);
    _dFree.resize(MAX_NUM_CONSTRAINTS);
    _f.resize(MAX_NUM_CONSTRAINTS);
    _numConstraints = 0;
    _mu = mu.getValue();
    constraintGroups.beginEdit()->insert(0);
    constraintGroups.endEdit();

    _numPreviousContact=0;
    _PreviousContactList.resize(MAX_NUM_CONSTRAINTS);
    _cont_id_list.resize(MAX_NUM_CONSTRAINTS);

    constraintRenumbering.resize(MAX_NUM_CONSTRAINTS);
    for (unsigned int i=0; i<MAX_NUM_CONSTRAINTS; ++i)
        constraintRenumbering[i] = i+(i/15);

    unsigned nmax = MAX_NUM_CONSTRAINTS + MAX_NUM_CONSTRAINTS/15;
    constraintReinitialize.resize(nmax);
    for (unsigned i=0; i<MAX_NUM_CONSTRAINTS; i++)
    {
        constraintReinitialize[i+i/15] = i;
    }
}

template<class real>
void CudaMasterContactSolver<real>::init()
{
    sofa::core::objectmodel::BaseContext* context = this->getContext();
    context->get<core::componentmodel::behavior::BaseConstraintCorrection>(&constraintCorrections, core::objectmodel::BaseContext::SearchDown);
}

template<class real>
void CudaMasterContactSolver<real>::build_LCP()
{
    _numConstraints = 0;

    // mechanical action executed from root node to propagate the constraints
    simulation::MechanicalResetConstraintVisitor().execute(context);
    _mu = mu.getValue();
    simulation::MechanicalAccumulateConstraint(_numConstraints).execute(context);
    _mu = mu.getValue();

    _realNumConstraints = _numConstraints;

    if (_numConstraints > MAX_NUM_CONSTRAINTS)
    {
        serr<<sendl<<"Error in CudaMasterContactSolver, maximum number of contacts exceeded, "<< _numConstraints/3 <<" contacts detected"<<endl;
        MAX_NUM_CONSTRAINTS=MAX_NUM_CONSTRAINTS+MAX_NUM_CONSTRAINTS;

        _PreviousContactList.resize(MAX_NUM_CONSTRAINTS * sizeof(contactBuf));
        _cont_id_list.resize(MAX_NUM_CONSTRAINTS * sizeof(long));

        constraintRenumbering.resize(MAX_NUM_CONSTRAINTS);
        for (unsigned int i=0; i<MAX_NUM_CONSTRAINTS; ++i)
            constraintRenumbering[i] = i+(i/15);

        unsigned nmax = MAX_NUM_CONSTRAINTS + MAX_NUM_CONSTRAINTS/15;
        constraintReinitialize.resize(nmax);
        for (unsigned i=0; i<MAX_NUM_CONSTRAINTS; i++)
        {
            constraintReinitialize[i+i/15] = i;
        }
    }

    if (_mu > 0.0)
    {
        simulation::MechanicalRenumberConstraint(constraintRenumbering).execute(context);

        _realNumConstraints += _realNumConstraints/15;
    }

    unsigned num3Constraint = _realNumConstraints;

    _realNumConstraints = ((_realNumConstraints+15)/16) * 16;
    if (_realNumConstraints<32) _realNumConstraints = 32;

    _dFree.resize(_realNumConstraints);
    _W.resize(_realNumConstraints,_realNumConstraints);
    _f.resize(_realNumConstraints);

    _W.clear();

    CudaMechanicalGetConstraintValueVisitor(&_dFree).execute(context);
    //simulation::MechanicalComputeComplianceVisitor(_W).execute(context);

    for (unsigned int i=0; i<constraintCorrections.size(); i++)
    {
        core::componentmodel::behavior::BaseConstraintCorrection* cc = constraintCorrections[i];
        cc->getCompliance(&_W);
    }

    if (_mu > 0.0) simulation::MechanicalRenumberConstraint(constraintReinitialize).execute(context);

    for (unsigned i=num3Constraint; i<_realNumConstraints; i++)
    {
        _W.set(i,i,1.0);
    }



    if (initial_guess.getValue())
    {
        MechanicalGetContactIDVisitor(&(_cont_id_list[0])).execute(context);
        computeInitialGuess();
    }
}

template<class real>
void CudaMasterContactSolver<real>::computeInitialGuess()
{
    int numContact = (_mu > 0.0) ? _numConstraints/3 : _numConstraints;

    for (int c=0; c<numContact; c++)
    {
        if (_mu>0.0)
        {
            _f[3*c  ] = 0.0;
            _f[3*c+1] = 0.0;
            _f[3*c+2] = 0.0;
        }
        else
        {
            _f[c] =  0.0;
        }
    }

    for (int c=0; c<numContact; c++)
    {
        for (unsigned int pc=0; pc<_numPreviousContact; pc++)
        {
            if (_cont_id_list[c] == _PreviousContactList[pc].id)
            {
                if (_mu>0.0)
                {
                    _f[3*c  ] = (real)_PreviousContactList[pc].F.x();
                    _f[3*c+1] = (real)_PreviousContactList[pc].F.y();
                    _f[3*c+2] = (real)_PreviousContactList[pc].F.z();
                }
                else
                {
                    _f[c] =  (real)_PreviousContactList[pc].F.x();
                }
            }
        }
    }
}

template<class real>
void CudaMasterContactSolver<real>::keepContactForcesValue()
{
    _numPreviousContact=0;

    int numContact = (_mu > 0.0) ? _numConstraints/3 : _numConstraints;
    //_PreviousContactList.resize(numContact);

    for (int c=0; c<numContact; c++)
    {
        if (_mu>0.0)
        {
            if (_f[3*c]>0.0)
            {
                _PreviousContactList[_numPreviousContact].id = (_cont_id_list[c] >= 0) ? _cont_id_list[c] : -_cont_id_list[c];
                _PreviousContactList[_numPreviousContact].F.x() = _f[3*c];
                _PreviousContactList[_numPreviousContact].F.y() = _f[3*c+1];
                _PreviousContactList[_numPreviousContact].F.z() = _f[3*c+2];
                _numPreviousContact++;
            }
        }
        else
        {
            if (_f[c]>0.0)
            {
                _PreviousContactList[_numPreviousContact].id = (_cont_id_list[c] >= 0) ? _cont_id_list[c] : -_cont_id_list[c];
                _PreviousContactList[_numPreviousContact].F.x() = _f[c];
                _numPreviousContact++;
            }

        }
    }
}

template<class real>
void CudaMasterContactSolver<real>::step(double dt)
{

    context = dynamic_cast<simulation::Node *>(this->getContext()); // access to current node
#ifdef DISPLAY_TIME
    CTime *timer;
    double timeScale = 1.0 / (double)CTime::getRefTicksPerSec();
    timer = new CTime();
    double time_Free_Motion = (double) timer->getTime();
#endif

    // Update the BehaviorModels
    // Required to allow the RayPickInteractor interaction
    simulation::BehaviorUpdatePositionVisitor updatePos(dt);
    context->execute(&updatePos);

    simulation::MechanicalBeginIntegrationVisitor beginVisitor(dt);
    context->execute(&beginVisitor);

    // Free Motion
    simulation::SolveVisitor freeMotion(dt, true);
    context->execute(&freeMotion);
    simulation::MechanicalPropagateFreePositionVisitor().execute(context);

    core::componentmodel::behavior::BaseMechanicalState::VecId dx_id = core::componentmodel::behavior::BaseMechanicalState::VecId::dx();
    simulation::MechanicalVOpVisitor(dx_id).execute( context);
    simulation::MechanicalPropagateDxVisitor(dx_id).execute( context);
    simulation::MechanicalVOpVisitor(dx_id).execute( context);

#ifdef DISPLAY_TIME
    time_Free_Motion = ((double) timer->getTime() - time_Free_Motion)*timeScale;
    double time_computeCollision = (double) timer->getTime();
#endif

    computeCollision();

#ifdef DISPLAY_TIME
    time_computeCollision = ((double) timer->getTime() - time_computeCollision)*timeScale;
    double time_build_LCP = (double) timer->getTime();
#endif
    //MechanicalResetContactForceVisitor().execute(context);

    for (unsigned int i=0; i<constraintCorrections.size(); i++)
    {
        core::componentmodel::behavior::BaseConstraintCorrection* cc = constraintCorrections[i];
        cc->resetContactForce();
    }

    build_LCP();

#ifdef DISPLAY_TIME
    time_build_LCP = ((double) timer->getTime() - time_build_LCP)*timeScale;
    double time_solve_LCP = (double) timer->getTime();
#endif

    double _tol = tol.getValue();
    int _maxIt = maxIt.getValue();

    if (! initial_guess.getValue()) _f.clear();

    double error = 0.0;

#ifdef CHECK
    real t1,t2;

    if (_mu > 0.0)
    {
        f_check.resize(_numConstraints,MBSIZE);
        for (unsigned i=0; i<_numConstraints; i++) f_check[i] = _f[i];

        real toln = ((int) (_realNumConstraints/3) + 1) * (real)_tol;
        t2 = sofa::gpu::cuda::CudaLCP<real>::CudaNlcp_gaussseidel(useGPU_d.getValue(),_numConstraints, _dFree.getCudaVector(), _W.getCudaMatrix(), f_check.getCudaVector(), _mu,_toln, _maxIt);

        t1 = sofa::gpu::cuda::CudaLCP<real>::CudaNlcp_gaussseidel(0,_numConstraints, _dFree.getCudaVector(), _W.getCudaMatrix(), _f.getCudaVector(), _mu,_toln, _maxIt);
    }
    else
    {
        f_check.resize(_numConstraints);
        for (unsigned i=0; i<_numConstraints; i++) f_check[i] = _f[i];
        t2 = sofa::gpu::cuda::CudaLCP<real>::CudaGaussSeidelLCP1(useGPU_d.getValue(),_numConstraints, _dFree.getCudaVector(), _W.getCudaMatrix(), f_check.getCudaVector(), _tol, _maxIt);

        t1 = sofa::gpu::cuda::CudaLCP<real>::CudaGaussSeidelLCP1(0,_numConstraints, _dFree.getCudaVector(), _W.getCudaMatrix(), _f.getCudaVector(), _tol, _maxIt);
    }

    for (unsigned i=0; i<f_check.size(); i++)
    {
        if ((f_check.element(i)-_f.element(i)>CHECK) || (f_check.element(i)-_f.element(i)<-CHECK))
        {
            std::cerr << "Error(" << useGPU_d.getValue() << ") dim(" << _numConstraints << ") realDim(" << _realNumConstraints << ") elmt(" << i << ") : (cpu," << f_check.element(i) << ") (gpu,(" << _f.element(i) << ")" << std::endl;
        }
    }

#else
    if (_mu > 0.0)
    {
// 			_wTmp.resize(_realNumConstraints,_realNumConstraints);
//
// 			_wTmp.clear();
//
// 			for (unsigned k=0;k<_numConstraints;k++) {
// 			  int j = k + k/15;
// 			  for (unsigned l=0;l<_numConstraints;l++) {
// 			    int i = l + l/15;
// 			    _wTmp.set(i,j,_W.element(l,k));
// 			  }
// 			}
//
// 			for (unsigned k=_numConstraints+_numConstraints/15;k<_realNumConstraints;k++) {
// 			  _wTmp.set(k,k,1.0);
// 			}

        real toln = ((int) (_realNumConstraints/3) + 1) * (real)_tol;
        error = sofa::gpu::cuda::CudaLCP<real>::CudaNlcp_gaussseidel(useGPU_d.getValue(),_realNumConstraints, _dFree.getCudaVector(), _W.getCudaMatrix(), _f.getCudaVector(), _mu,toln, _maxIt);

// 			printf("\nFE = [");
// 			for (unsigned j=0;j<_numConstraints;j++) {
// 				printf("%f ",_f.element(j));
// 			}
// 			printf("]\n");

// 			real toln = ((int) (_numConstraints/3) + 1) * (real)_tol;
// 			error = sofa::gpu::cuda::CudaLCP<real>::CudaNlcp_gaussseidel(useGPU_d.getValue(),_realNumConstraints, _dFree.getCudaVector(), _W.getCudaMatrix(), _f.getCudaVector(), _mu,toln, _maxIt);

// 			printf("M = [\n");
// 			for (unsigned j=0;j<_W.rowSize();j++) {
// 				for (unsigned i=0;i<_W.colSize();i++) {
// 					printf("%f\t",_W.element(i,j));
// 				}
// 				printf("\n");
// 			}
// 			printf("]\n");
//
// 			printf("q = [");
// 			for (unsigned j=0;j<_f.size();j++) {
// 				printf("%f\t",_dFree.element(j));
// 			}
// 			printf("]\n");
//
// 			printf("FS = [");
// 			for (unsigned j=0;j<_numConstraints;j++) {
// 				printf("%f ",_f.element(j));
// 			}
// 			printf("]\n");
    }
    else
    {
        error = sofa::gpu::cuda::CudaLCP<real>::CudaGaussSeidelLCP1(useGPU_d.getValue(),_realNumConstraints, _dFree.getCudaVector(), _W.getCudaMatrix(), _f.getCudaVector(), (real)_tol, _maxIt);
    }
#endif

    if (error > _tol) sout << "No convergence in gaussSeidelLCP1 : error = " << error << sendl;

#ifdef DISPLAY_TIME
    time_solve_LCP = ((double) timer->getTime() - time_solve_LCP)*timeScale;
    double time_contactCorrections = (double) timer->getTime();
#endif

    if (initial_guess.getValue()) keepContactForcesValue();

    // MechanicalApplyContactForceVisitor(_result).execute(context);

    for (unsigned int i=0; i<constraintCorrections.size(); i++)
    {
        core::componentmodel::behavior::BaseConstraintCorrection* cc = constraintCorrections[i];
        cc->applyContactForce(&_f);
    }

    simulation::MechanicalPropagateAndAddDxVisitor().execute( context);

    //simulation::MechanicalPropagatePositionAndVelocityVisitor().execute(context);

    //simulation::MechanicalPropagateAndAddDxVisitor().execute( context);

    //MechanicalResetContactForceVisitor().execute(context);

    for (unsigned int i=0; i<constraintCorrections.size(); i++)
    {
        core::componentmodel::behavior::BaseConstraintCorrection* cc = constraintCorrections[i];
        cc->resetContactForce();
    }

#ifdef DISPLAY_TIME
    time_contactCorrections = ((double) timer->getTime() - time_contactCorrections)*timeScale;
#endif


#ifdef DISPLAY_TIME
    double total = time_Free_Motion + time_computeCollision + time_build_LCP + time_solve_LCP + time_contactCorrections;

    if (this->print_info.getValue())
    {
        sout<<"********* Start Iteration : " << _numConstraints << " contacts *********" <<sendl;
        sout<<"Free Motion\t" << time_Free_Motion <<" s \t| " << time_Free_Motion*100.0/total << "%" <<sendl;
        sout<<"ComputeCollision\t" << time_computeCollision <<" s \t| " << time_computeCollision*100.0/total << "%"  <<sendl;
        sout<<"Build_LCP\t" << time_build_LCP <<" s \t| " << time_build_LCP*100.0/total << "%"  <<sendl;
        sout<<"Solve_LCP\t" << time_solve_LCP <<" s \t| " << time_solve_LCP*100.0/total << "%"  <<sendl;
        sout<<"ContactCorrections\t" << time_contactCorrections <<" s \t| " << time_contactCorrections*100.0/total << "%"  <<sendl;

        unsigned nbNnul = 0;
        unsigned sz = _realNumConstraints * _realNumConstraints;
        for (unsigned j=0; j<sz; j++) if (_W.getCudaMatrix().hostRead()[j]==0.0) nbNnul++;

        sout<<"Sparsity =  " << ((double) (((double) nbNnul * 100.0) / ((double)sz))) <<"%" <<sendl;
    }
#endif

    simulation::MechanicalEndIntegrationVisitor endVisitor(dt);
    context->execute(&endVisitor);
}

SOFA_DECL_CLASS(CudaMasterContactSolver)

int CudaMasterContactSolverClass = core::RegisterObject("Cuda Constraint solver")
        .add< CudaMasterContactSolver<float> >(true)
        .add< CudaMasterContactSolver<double> >()
        ;

} // namespace odesolver

} // namespace component

} // namespace sofa
