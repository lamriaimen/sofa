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
#include <SofaGeneral/config.h>

#include <SofaGeneral/initSofaGeneral.h>
#include <SofaGeneralLoader/initGeneralLoader.h>
#include <SofaGeneralObjectInteraction/initGeneralObjectInteraction.h>
#include <SofaGeneralSimpleFem/initGeneralSimpleFEM.h>
#include <SofaGeneralTopology/initGeneralTopology.h>
#include <SofaTopologyMapping/initTopologyMapping.h>
#include <SofaUserInteraction/initUserInteraction.h>
#include <SofaConstraint/initConstraint.h>

namespace sofa
{

namespace component
{


void initSofaGeneral()
{
    static bool first = true;
    if (first)
    {
        first = false;
    }

    initGeneralLoader();
    initGeneralObjectInteraction();
    initGeneralSimpleFEM();
    initGeneralTopology();
    initTopologyMapping();
    initUserInteraction();
    initConstraint();
}


} // namespace component

} // namespace sofa
