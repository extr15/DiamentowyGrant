// OpenAIL - Open Android Indoor Localization
// Copyright (C) 2015 Michal Nowicki (michal.nowicki@put.poznan.pl)
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <jni.h>
#include <string.h>
#include <android/log.h>
#include <pthread.h>

// Core
#include "g2o/core/sparse_optimizer.h"
#include "g2o/core/block_solver.h"
#include "g2o/core/optimization_algorithm_gauss_newton.h"
#include "g2o/core/optimization_algorithm_levenberg.h"
#include "g2o/core/optimizable_graph.h"
#include "g2o/core/base_binary_edge.h"

// Types
#include "g2o/types/slam3d/vertex_se3.h"
#include "g2o/types/slam2d/g2o_types_slam2d_api.h"
#include "g2o/types/slam2d/vertex_se2.h"
#include "g2o/types/slam2d/vertex_point_xy.h"
#include "g2o/types/slam3d/vertex_pointxyz.h"

// Solver
#include "g2o/solvers/pcg/linear_solver_pcg.h"

// G2o rest
#include "g2o/stuff/command_args.h"
#include "g2o/config.h"

// Custom edges
#include "edge_se2_pointXY_distance.h"
#include "edge_se2_pointXYZ_fixedZ_distance.h"
#include "edge_se2_distanceOrientation.h"
#include "edge_se2_orientation.h"
#include "edge_se2_placeVicinity.h"
#include "edge_se2_qr.h"


using namespace std;
using namespace g2o;

G2O_USE_TYPE_GROUP(slam2d);
G2O_USE_TYPE_GROUP(slam3d);

#define DEBUG_TAG "NDK_DG_GraphManager"

namespace ail {
class Vertex {
public:
	int vertexId;

	enum Type {
		/// Vertex 2D -- x, y
		VERTEX2D,
		/// Vertex SE(2) -- x, y, theta
		VERTEXSE2,
		/// Vertex 3D -- x, y, z
		VERTEX3D
	};

	Type type;
};

class Vertex2D: public Vertex {
public:
	Vertex2D() {
		pos = Eigen::Vector2d::Zero();
	}
	Eigen::Vector2d pos;
};

class VertexSE2: public Vertex {
public:
	VertexSE2() {
		pos = Eigen::Vector2d::Zero();
		orient = 0.0;
	}
	Eigen::Vector2d pos;
	double orient;
};

class Vertex3D: public Vertex {
public:
	Vertex3D() {
		pos = Eigen::Vector3d::Zero();
	}
	Eigen::Vector3d pos;
};

}



class GraphManager {
private:
	SparseOptimizer optimizer;

	// User only for initial estimates
	double prevUserPositionX, prevUserPositionY, prevUserPositionTheta;

public:
	GraphManager();

	// Perform optimization for given number of iterations
	// Returns chi2 and 0 if not ok (e.g. 0 vertices)
	double optimize(int iterationCount);

	// Save optimized graph to file
	int saveOptimizationResult(ofstream &ofs);

	// Delay adding to the graph
	void delayedAddToGraph(string dataToProcess);

	// Add information in string to graph
	void addToGraph();

	// Get information about position of vertex with given id
	std::vector<double> getVertexPosition(int id);

	// Get information about position of all vertices
	std::vector<double> getPositionOfAllVertices();

private:
	// Adding vertices:
	// 0 - SE2
	// 1 - XY
	int addVertex(stringstream &data, ail::Vertex::Type type);

	// add Vicinity Edge
	int addVicinityEdge(stringstream &data, string name);

	// WiFi Edge
	int addEdgeWiFi(stringstream &data);
	int addEdgeWiFi_SE2_XYZ(stringstream &data);

	// Stepometer
	int addEdgeStepometer(stringstream &data);

	// Stepometer
	int addEdgeOrientation(stringstream &data);

	// Typical odometry SE2 edge
	int addEdgeSE2(stringstream &data);

	// QR edge
	int addEdgeQR(stringstream &data);

	// findIndex of vertex with given id
	int findIndexInVertices(int id);

	// Takes the edges from the graph and extracts new vertex poses
	void extractVerticesEstimates(std::set<g2o::OptimizableGraph::Vertex*,
					g2o::OptimizableGraph::VertexIDCompare> & verticesToCopy);

	// Takes the new vertex poses and updates the current estimate
	void updateVerticesEstimates(
			const std::set<g2o::OptimizableGraph::Vertex*,
					g2o::OptimizableGraph::VertexIDCompare>& verticesToCopy);

	// Current estimates
	std::vector<ail::Vertex*> vertices;

	// Lock to graph
	pthread_mutex_t graphMtx;

	// Lock to current estimate - vertices
	pthread_mutex_t verticesMtx;

	// Information to add to graph
	pthread_mutex_t addMtx;
	string dataToAdd;

	// Buffor of vertices to add
	pthread_mutex_t bufferMtx;
	std::vector<g2o::OptimizableGraph::Vertex*> bufferVertices;
	std::vector<g2o::EdgeSE2DistanceOrientation*> bufferStepometerEdges;
	std::vector<g2o::EdgeSE2PlaceVicinity> bufferPlaceVicinityEdges;
	std::vector<g2o::EdgeSE2PointXYDistance> bufferPointXYDistanceEdges;
	std::vector<g2o::EdgeSE2*> bufferSE2Edges;
	std::vector<g2o::EdgeSE2QR*> bufferSE2QREdges;
};
