
#include <glm/glm.hpp>
#include <algorithm>
#include <vector>
#include <queue>
#include "Utils.h"

#include "kdTree.h"

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

struct compareDistance {
    bool operator()(const std::pair<float, glm::vec3>& a,
                    const std::pair<float, glm::vec3>& b) const {
        return a.first < b.first;
    }
};

struct node {
	photon p;
	int dimension;
	node* left = nullptr;
	node* right = nullptr;
};

node* buildkdTree(std::vector<photon>& photons, int start, int end, int dimension) {
	int size = end - start + 1;
	if (size<=0) return nullptr;
	if (dimension == 0) std::sort(photons.begin()+start, photons.begin()+end+1, firstCol());
	else if (dimension == 1) std::sort(photons.begin()+start, photons.begin()+end+1, secondCol());
	else if (dimension == 2) std::sort(photons.begin()+start, photons.begin()+end+1, thirdCol());
	photon p;
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
	int mid = start + (size / 2);
	node* midNode = new node();
	midNode->p = photons[mid];
	midNode->dimension = dimension;
	midNode->left = buildkdTree(photons, start, mid - 1, (dimension+1)%3);
	midNode->right = buildkdTree(photons, mid + 1, end, (dimension+1)%3);
	return midNode;
}

void searchkdTree(node* root, std::priority_queue<float>& result, glm::vec3 point, int number) {
	if (root==nullptr) return;
	float distance = glm::length(root->p.location - point);
	if (result.size() < number) {
		result.emplace(distance);
	} else if (distance < result.top()) {
		result.pop();
		result.emplace(distance);
	}
	float delta;
	if (root->dimension == 0) delta = point.x - root->p.location.x;
	else if (root->dimension == 1) delta = point.y - root->p.location.y;
	else delta = point.z - root->p.location.z;

	if (delta <= 0) {
		searchkdTree(root->left, result, point, number);
		if ((result.size() < number) || (delta*delta < result.top()*result.top())) searchkdTree(root->right, result, point, number);
	} else {
		searchkdTree(root->right, result, point, number);
		if ((result.size() < number) || (delta*delta < result.top()*result.top())) searchkdTree(root->left, result, point, number);
	}

}