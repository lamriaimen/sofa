cmake_minimum_required(VERSION 3.22)
project(Sofa.Component.LinearSystem LANGUAGES CXX)

set(SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR "src/sofa/component/linearsystem")

set(HEADER_FILES
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/init.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/BaseMatrixProjectionMethod.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/BaseMatrixProjectionMethod.inl
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/CompositeLinearSystem.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/CompositeLinearSystem.inl
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/ConstantSparsityPatternSystem.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/ConstantSparsityPatternSystem.inl
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/ConstantSparsityProjectionMethod.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/ConstantSparsityProjectionMethod.inl
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/CreateMatrixDispatcher.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/LinearSystemData.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/MappedMassMatrixObserver.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/MappedMassMatrixObserver.inl
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/MappingGraph.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/MatrixFreeSystem.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/MatrixLinearSystem.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/MatrixLinearSystem.inl
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/MatrixProjectionMethod.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/MatrixProjectionMethod.inl
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/TypedMatrixLinearSystem.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/TypedMatrixLinearSystem.inl
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/config.h.in

    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/matrixaccumulators/AssemblingMappedMatrixAccumulator.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/matrixaccumulators/AssemblingMatrixAccumulator.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/matrixaccumulators/BaseAssemblingMatrixAccumulator.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/matrixaccumulators/ConstantLocalMappedMatrix.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/matrixaccumulators/ConstantLocalMatrix.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/matrixaccumulators/SparsityPatternLocalMappedMatrix.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/matrixaccumulators/SparsityPatternLocalMatrix.h

    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/visitors/AssembleGlobalVectorFromLocalVectorVisitor.h
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/visitors/DispatchFromGlobalVectorToLocalVectorVisitor.h
)

set(SOURCE_FILES
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/init.cpp
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/BaseMatrixProjectionMethod.cpp
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/CompositeLinearSystem.cpp
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/ConstantSparsityPatternSystem.cpp
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/ConstantSparsityProjectionMethod.cpp
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/MappedMassMatrixObserver.cpp
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/MappingGraph.cpp
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/MatrixLinearSystem.cpp
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/MatrixProjectionMethod.cpp
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/TypedMatrixLinearSystem.cpp

    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/visitors/AssembleGlobalVectorFromLocalVectorVisitor.cpp
    ${SOFACOMPONENTLINEARSOLVERLINEARSYSTEM_SOURCE_DIR}/visitors/DispatchFromGlobalVectorToLocalVectorVisitor.cpp
)

sofa_find_package(Sofa.Simulation.Core REQUIRED)

add_library(${PROJECT_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PUBLIC Sofa.Simulation.Core Sofa.LinearAlgebra)

sofa_create_package_with_targets(
    PACKAGE_NAME ${PROJECT_NAME}
    PACKAGE_VERSION ${Sofa_VERSION}
    TARGETS ${PROJECT_NAME} AUTO_SET_TARGET_PROPERTIES
    INCLUDE_SOURCE_DIR "src"
    INCLUDE_INSTALL_DIR "${PROJECT_NAME}"
)

cmake_dependent_option(SOFA_COMPONENT_LINEARSYSTEM_BUILD_TESTS "Compile the automatic tests" ON "SOFA_BUILD_TESTS OR NOT DEFINED SOFA_BUILD_TESTS" OFF)
if(SOFA_COMPONENT_LINEARSYSTEM_BUILD_TESTS)
    add_subdirectory(tests)
endif()
