
#include <glm/glm.hpp>
#include <algorithm>
#include <vector>
#include <queue>
#include "Utils.h"

#include "kdTree.h"

// KD tree implementation 
// based off implementation by : https://blog.krum.io/k-d-trees/

// functors to to use in sorting 
struct firstCol {
	bool operator()(const photon& lhs, const photon& rhs) const
	{
		return lhs.location[0] < rhs.location[0];
	}
};

struct secondCol {
	bool operator()(const photon& lhs, const photon& rhs) const
	{
		return lhs.location[1] < rhs.location[1];
	}
};

struct thirdCol {
	bool operator()(const photon& lhs, const photon& rhs) const
	{
		return lhs.location[2] < rhs.location[2];
	}
};

// wrapper for photon, stores essential data for building and searching tree
struct node {
	photon p;
	int dimension; // direction along which node splits 3d space 
	float distance;
	node* left = nullptr;
	node* right = nullptr;
};

// build tree from vector 
node* buildkdTree(std::vector<photon>& photons, int start, int end, int dimension) {
	int size = end - start + 1;
	if (size<=0) return nullptr;
	// sort using current dimension
	if (dimension == 0) std::sort(photons.begin()+start, photons.begin()+end+1, firstCol());
	else if (dimension == 1) std::sort(photons.begin()+start, photons.begin()+end+1, secondCol());
	else if (dimension == 2) std::sort(photons.begin()+start, photons.begin()+end+1, thirdCol());
	photon p;
	// base cases
	if (size==1) {
		node* n = new node();
		n->p = photons[start];
		n->dimension = dimension;
		n->left = nullptr;
		n->right = nullptr;
		return n;
	} else if (size==2) {
		node* n = new node();
		node* m = new node();
		n->p = photons[start];      
		n->dimension = dimension;            
		m->p = photons[start+1];    
		m->dimension = dimension;  
		n->left = nullptr;
		n->right = m;              
		m->right = nullptr;
		m->left = nullptr;
		return n;
	}
	// recursive case, recur on both children while incrementing dimension
	int mid = start + (size / 2);
	node* midNode = new node();
	midNode->p = photons[mid]; // next current node is median of current vector 
	midNode->dimension = dimension;
	midNode->left = buildkdTree(photons, start, mid - 1, (dimension+1)%3);
	midNode->right = buildkdTree(photons, mid + 1, end, (dimension+1)%3);
	return midNode;
}

// find number closes photons to coordinate
void searchkdTree(node* root, std::priority_queue<photon>& result, glm::vec3 point, int number) {
	if (root==nullptr) return;
	float distance = glm::length(root->p.location - point);
	root->p.distance = distance;
	if (result.size() < number) {
		// result not yet full, add 
		result.emplace(root->p);
	} else if (distance < result.top().distance) {
		// result full, but new photon is closer than existing furthest 
		result.pop();
		result.emplace(root->p);
	}
	// dimensions of split determines direction to search 
	float delta;
	if (root->dimension == 0) delta = point.x - root->p.location.x;
	else if (root->dimension == 1) delta = point.y - root->p.location.y;
	else delta = point.z - root->p.location.z;
    // recur on either child
	if (delta <= 0) {
		searchkdTree(root->left, result, point, number);
		if ((result.size() < number) || (delta*delta < result.top().distance*result.top().distance)) searchkdTree(root->right, result, point, number);
	} else {
		searchkdTree(root->right, result, point, number);
		if ((result.size() < number) || (delta*delta < result.top().distance*result.top().distance)) searchkdTree(root->left, result, point, number);
	}

}