/******************************************************************************
*                 SOFA, Simulation Open-Framework Architecture                *
*                    (c) 2006 INRIA, USTL, UJF, CNRS, MGH                     *
*                                                                             *
* This program is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This program is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this program. If not, see <http://www.gnu.org/licenses/>.        *
*******************************************************************************
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/

#include <sofa/simulation/Node.h>
#include <SofaBaseMechanics/MechanicalObject.h>
#include <SofaBaseMechanics/UniformMass.h>
#include <SofaBaseTopology/RegularGridTopology.h>
#include <SofaBaseLinearSolver/CGLinearSolver.h>
#include <SofaImplicitOdeSolver/EulerImplicitSolver.h>
#include <SofaGeneralEngine/PairBoxRoi.h>
#include <SofaEngine/BoxROI.h>


namespace sofa
{

typedef component::container::MechanicalObject<defaulttype::Vec3Types> MechanicalObject3;

/// Structure which contains the nodes and the pointers useful for the patch test
template<class T>
struct PatchTestStruct
{
    simulation::Node::SPtr SquareNode;
    typename component::projectiveconstraintset::AffineMovementConstraint<T>::SPtr affineConstraint;
    typename component::container::MechanicalObject<T>::SPtr dofs;
};

/// Create a scene with a regular grid and an affine constraint for patch test

template<class DataTypes> 
PatchTestStruct<DataTypes> createRegularGridScene(
        simulation::Node::SPtr root,
        typename DataTypes::Coord startPoint,
        typename DataTypes::Coord endPoint,
        int numX,
        int numY,
        int numZ,
        sofa::defaulttype::Vec<6,SReal> entireBoxRoi,
        sofa::defaulttype::Vec<6,SReal> inclusiveBox,
        sofa::defaulttype::Vec<6,SReal> includedBox)
{
    // Definitions
    PatchTestStruct<DataTypes> patchStruct;
    typedef typename DataTypes::Real Real;
    typedef typename component::container::MechanicalObject<DataTypes> MechanicalObject;
    typedef typename sofa::component::mass::UniformMass <DataTypes, Real> UniformMass;
    typedef component::topology::RegularGridTopology RegularGridTopology;
    typedef typename component::engine::BoxROI<DataTypes> BoxRoi;
    typedef typename sofa::component::engine::PairBoxROI<DataTypes> PairBoxRoi;
    typedef typename component::projectiveconstraintset::AffineMovementConstraint<DataTypes> AffineMovementConstraint;
    typedef component::linearsolver::CGLinearSolver<component::linearsolver::GraphScatteredMatrix, component::linearsolver::GraphScatteredVector> CGLinearSolver;

    // Root node
    root->setGravity({ 0,0,0 });
    root->setAnimate(false);
    root->setDt(0.05);

    // Node square
    simulation::Node::SPtr SquareNode = root->createChild("Square");

    // Euler implicit solver and cglinear solver
    component::odesolver::EulerImplicitSolver::SPtr solver = modeling::addNew<component::odesolver::EulerImplicitSolver>(SquareNode,"EulerImplicitSolver");
    solver->f_rayleighStiffness.setValue(0.5);
    solver->f_rayleighMass.setValue(0.5);
    CGLinearSolver::SPtr cgLinearSolver = modeling::addNew< CGLinearSolver >(SquareNode,"linearSolver");
    cgLinearSolver->d_maxIter.setValue(25);
    cgLinearSolver->d_tolerance.setValue(1e-5);
    cgLinearSolver->d_smallDenominatorThreshold.setValue(1e-5);

    // Mass
    typename UniformMass::SPtr mass = modeling::addNew<UniformMass>(SquareNode,"mass");

    // Regular grid topology
    RegularGridTopology::SPtr gridMesh = modeling::addNew<RegularGridTopology>(SquareNode,"loader");
    gridMesh->setSize(numX,numY,numZ);
    gridMesh->setPos(startPoint[0],endPoint[0],startPoint[1],endPoint[1],startPoint[2],endPoint[2]);

    //Mechanical object
    patchStruct.dofs = modeling::addNew<MechanicalObject>(SquareNode,"mechanicalObject");
    patchStruct.dofs->setName("mechanicalObject");
    patchStruct.dofs->setSrc("@"+gridMesh->getName(), gridMesh.get());

    //BoxRoi to find all mesh points
    helper::vector < defaulttype::Vec<6,Real> > vecBox;
    vecBox.push_back(entireBoxRoi);
    typename BoxRoi::SPtr boxRoi = modeling::addNew<BoxRoi>(SquareNode,"boxRoi");
    boxRoi->d_alignedBoxes.setValue(vecBox);
    boxRoi->d_strict.setValue(false);

    //PairBoxRoi to define the constrained points = points of the border
    typename PairBoxRoi::SPtr pairBoxRoi = modeling::addNew<PairBoxRoi>(SquareNode,"pairBoxRoi");
    pairBoxRoi->inclusiveBox.setValue(inclusiveBox);
    pairBoxRoi->includedBox.setValue(includedBox);

    //Affine constraint
    patchStruct.affineConstraint  = modeling::addNew<AffineMovementConstraint>(SquareNode,"affineConstraint");
    modeling::setDataLink(&boxRoi->d_indices,&patchStruct.affineConstraint->m_meshIndices);
    modeling::setDataLink(&pairBoxRoi->f_indices,& patchStruct.affineConstraint->m_indices);

    patchStruct.SquareNode = SquareNode;
    return patchStruct;
}

} // namespace sofa