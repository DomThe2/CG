#pragma once

#include <queue>

struct node;
struct compareDistance;
node* buildkdTree(std::vector<photon>& photons, int start, int end, int dimension);
void searchkdTree(node* root, std::priority_queue<photon>& result, glm::vec3 point, int number);