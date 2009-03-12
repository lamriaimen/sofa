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
#ifndef SOFA_COMPONENT_TOPOLOGY_HEXAHEDRONSETTOPOLOGYALGORITHMS_H
#define SOFA_COMPONENT_TOPOLOGY_HEXAHEDRONSETTOPOLOGYALGORITHMS_H

#include <sofa/component/topology/QuadSetTopologyAlgorithms.h>

namespace sofa
{

namespace component
{

namespace topology
{
class HexahedronSetTopologyContainer;

class HexahedronSetTopologyModifier;

template < class DataTypes >
class HexahedronSetGeometryAlgorithms;

/**
* A class that performs topology algorithms on an HexahedronSet.
*/
template < class DataTypes >
class HexahedronSetTopologyAlgorithms : public QuadSetTopologyAlgorithms<DataTypes>
{
public:
    typedef typename DataTypes::Real Real;

    HexahedronSetTopologyAlgorithms()
        : QuadSetTopologyAlgorithms<DataTypes>()
    { }

    virtual ~HexahedronSetTopologyAlgorithms() {}

    virtual void init();

private:
    HexahedronSetTopologyContainer*						m_container;
    HexahedronSetTopologyModifier*						m_modifier;
    HexahedronSetGeometryAlgorithms< DataTypes >*		m_geometryAlgorithms;
};

#if defined(WIN32) && !defined(SOFA_COMPONENT_TOPOLOGY_HEXAHEDRONSETTOPOLOGYALGORITHMS_CPP)
#pragma warning(disable : 4231)
#ifndef SOFA_FLOAT
extern template class SOFA_COMPONENT_TOPOLOGY_API HexahedronSetTopologyAlgorithms<defaulttype::Vec3dTypes>;
extern template class SOFA_COMPONENT_TOPOLOGY_API HexahedronSetTopologyAlgorithms<defaulttype::Vec2dTypes>;
extern template class SOFA_COMPONENT_TOPOLOGY_API HexahedronSetTopologyAlgorithms<defaulttype::Vec1dTypes>;
//extern template class SOFA_COMPONENT_TOPOLOGY_API HexahedronSetTopologyAlgorithms<defaulttype::Rigid3dTypes>;
//extern template class SOFA_COMPONENT_TOPOLOGY_API HexahedronSetTopologyAlgorithms<defaulttype::Rigid2dTypes>;
#endif

#ifndef SOFA_DOUBLE
extern template class SOFA_COMPONENT_TOPOLOGY_API HexahedronSetTopologyAlgorithms<defaulttype::Vec3fTypes>;
extern template class SOFA_COMPONENT_TOPOLOGY_API HexahedronSetTopologyAlgorithms<defaulttype::Vec2fTypes>;
extern template class SOFA_COMPONENT_TOPOLOGY_API HexahedronSetTopologyAlgorithms<defaulttype::Vec1fTypes>;
//extern template class SOFA_COMPONENT_TOPOLOGY_API HexahedronSetTopologyAlgorithms<defaulttype::Rigid3fTypes>;
//extern template class SOFA_COMPONENT_TOPOLOGY_API HexahedronSetTopologyAlgorithms<defaulttype::Rigid2fTypes>;
#endif
#endif

} // namespace topology

} // namespace component

} // namespace sofa

#endif
