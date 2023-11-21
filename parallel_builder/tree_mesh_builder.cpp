/**
 * @file    tree_mesh_builder.cpp
 *
 * @author  FULL NAME <xlogin00@stud.fit.vutbr.cz>
 *
 * @brief   Parallel Marching Cubes implementation using OpenMP tasks + octree early elimination
 *
 * @date    DATE
 **/

#include <iostream>
#include <math.h>
#include <limits>

#include "tree_mesh_builder.h"

TreeMeshBuilder::TreeMeshBuilder(unsigned gridEdgeSize)
    : BaseMeshBuilder(gridEdgeSize, "Octree")
{

}

unsigned TreeMeshBuilder::octree(const unsigned edge_size, const int offsetx, const int offsety, const int offsetz, const ParametricScalarField &field)
{   
    // check if F(p) > l + a * sqrt(3)/2
    const unsigned half_edge = edge_size / 2;
    Vec3_t<float> center( (half_edge + offsetx) * mGridResolution,
                        (half_edge + offsety) * mGridResolution,
                        (half_edge + offsetz) * mGridResolution);
    float eval = evaluateFieldAt(center, field);
    //std::cout << "HERE " << eval << " " << edge_size << " " << offsetx << ", " << offsety << ", " << offsetz << std::endl;
    if (eval > mIsoLevel + (sqrt(3.0f)/2.0f)*(edge_size*mGridResolution)){
        return 0;
    }

    constexpr unsigned threshold_size = 3;
    unsigned totalTriangles = 0;

    // check threshold and build cube if smaller than threshold
    if (edge_size < threshold_size){
        const size_t octreeCubesCount = edge_size * edge_size * edge_size;
        for (int i = 0; i < octreeCubesCount; i++){
            Vec3_t<float> cubeOffset( (i % edge_size) + offsetx,
                                     (i / edge_size) % edge_size + offsety,
                                      i / (edge_size*edge_size) + offsetz);

            
            unsigned triangles = buildCube(cubeOffset, field);
            totalTriangles += triangles;
        }
        return totalTriangles;
    }

    // create new 8 cubes / octree
    for (int i = 0; i < 8; i++){
        #pragma omp task shared(totalTriangles, half_edge, offsetx, offsety, offsetz)
        {
        unsigned triangles = octree(half_edge, 
                                (i % 2) * half_edge + offsetx, 
                                ((i / 2) % 2) * half_edge + offsety, 
                                (i / 4) * half_edge + offsetz, 
                                field);
        #pragma omp critical(TRIANGLE)
        {
        totalTriangles += triangles;
        }
        }
    }
    #pragma omp taskwait
    return totalTriangles;
}

unsigned TreeMeshBuilder::marchCubes(const ParametricScalarField &field)
{
    // Suggested approach to tackle this problem is to add new method to
    // this class. This method will call itself to process the children.
    // It is also strongly suggested to first implement Octree as sequential
    // code and only when that works add OpenMP tasks to achieve parallelism.
    
    unsigned totalTriangles = 0;
    #pragma omp parallel
    {
    #pragma omp single
    {
    totalTriangles = octree(mGridSize, 0, 0, 0, field);
    }
    }
    return totalTriangles;
}

float TreeMeshBuilder::evaluateFieldAt(const Vec3_t<float> &pos, const ParametricScalarField &field)
{
    // NOTE: This method is called from "buildCube(...)"!

    // 1. Store pointer to and number of 3D points in the field
    //    (to avoid "data()" and "size()" call in the loop).
    const Vec3_t<float> *pPoints = field.getPoints().data();
    const unsigned count = unsigned(field.getPoints().size());

    float value = std::numeric_limits<float>::max();

    // 2. Find minimum square distance from points "pos" to any point in the
    //    field.
    for(unsigned i = 0; i < count; ++i)
    {
        float distanceSquared  = (pos.x - pPoints[i].x) * (pos.x - pPoints[i].x);
        distanceSquared       += (pos.y - pPoints[i].y) * (pos.y - pPoints[i].y);
        distanceSquared       += (pos.z - pPoints[i].z) * (pos.z - pPoints[i].z);

        // Comparing squares instead of real distance to avoid unnecessary
        // "sqrt"s in the loop.
        value = std::min(value, distanceSquared);
    }

    // 3. Finally take square root of the minimal square distance to get the real distance
    return sqrt(value);
}

void TreeMeshBuilder::emitTriangle(const BaseMeshBuilder::Triangle_t &triangle)
{
    // NOTE: This method is called from "buildCube(...)"!

    // Store generated triangle into vector (array) of generated triangles.
    // The pointer to data in this array is return by "getTrianglesArray(...)" call
    // after "marchCubes(...)" call ends.
    #pragma omp critical(STORE)
    {
    mTriangles.push_back(triangle);
    }
}
