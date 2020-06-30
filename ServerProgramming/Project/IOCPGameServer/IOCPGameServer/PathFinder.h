
#pragma once
#include "iostream"
#include "protocol.h"
#include "vector"

int clamp(int pos, int min, int max)
{
	if (pos <= min)return min;
	if (pos >= max)return max;
	return pos;
}

using namespace std;

struct Pos
{
	short x, y;

	bool operator==(const Pos& rhs)
	{
		return this->x == rhs.x && this->y == rhs.y;
	}
	bool operator!=(const Pos& rhs)
	{
		return this->x != rhs.x || this->y != rhs.y;
	}
};
struct Node
{
	int Fs;
	int Gs;
	int Hs;

	Pos pos;

	Node* next;
	Node* parent;
	Node()
	{
		next = nullptr;
		parent = nullptr;
	}

	Node(Pos pos, int Gs, int Hs, Node* parent = nullptr) : pos(pos), Fs(Fs), Gs(Gs), Hs(Hs), parent(parent) {
		Fs = Gs + Hs;
		next = nullptr;
		parent = nullptr;

	}

	void insert(Node* node)
	{
		if (next == nullptr) {
			next = node;
			node->next = nullptr;
			return;
		}
		Node* curr = this;
		while (curr != nullptr) {
			if (curr->next == nullptr) {
				node->next = curr->next;
				curr->next = node;
				return;
			}
			if (node->Fs <= curr->next->Fs) {
				node->next = curr->next;
				curr->next = node;
				return;
			}
			curr = curr->next;
		}
	}
	void insert_front(Node* node)
	{
		if (next == nullptr) {
			next = node;
			node->next = nullptr;
			return;
		}
		node->next = this->next;
		this->next = node;
	}
	void print()
	{
		Node* curr = this;
		int i = 1;
		while (curr->next != nullptr)
		{
			curr = curr->next;
			cout << i << ": Fs-" << curr->Fs << ", Hs-" << curr->Hs << ", Gs-" << curr->Gs << ", Pos: " << curr->pos.x << ", " << curr->pos.y << endl;
			i++;
		}
	}
	Node* search(Pos p)
	{
		if (this == nullptr)return nullptr;
		Node* curr = this;

		while (curr->next != nullptr)
		{
			curr = curr->next;
			if (curr->pos.x == p.x && curr->pos.y == p.y)
				return curr;

		}
		return nullptr;
	}
	void MoveToNodeFront(Node* other)
	{
		Node* node = next->next;
		other->insert(this->next);
		this->next = node;
	}
};

class PathFinder
{

private:
	bool m_map[WORLD_WIDTH][WORLD_HEIGHT];

public:
	PathFinder() :m_map{0} {};
	~PathFinder() {};
	PathFinder(bool map[][WORLD_WIDTH]) :m_map{0} {

		for (int i = 0; i < WORLD_WIDTH; ++i) {
			for (int j = 0; j < WORLD_HEIGHT; ++j) {
				m_map[j][i] = map[i][j];
			}
		}
	};
	void SetMap(bool map[][WORLD_WIDTH])
	{
		for (int i = 0; i < WORLD_WIDTH; ++i) {
			for (int j = 0; j < WORLD_HEIGHT; ++j) {
				m_map[j][i] = map[i][j];
			}
		}
	}
	bool is_needAstar(Pos curr_pos, Pos dest_pos)
	{
		while (curr_pos != dest_pos)
		{
			if (curr_pos.x != dest_pos.x) {
				curr_pos.x += clamp(dest_pos.x - curr_pos.x, -1, 1);
			}
			if (curr_pos.y != dest_pos.y) {
				curr_pos.y += clamp(dest_pos.y - curr_pos.y, -1, 1);
			}
			if (!m_map[curr_pos.x][curr_pos.y]) return true;
		}
		return false;
	}

	int calculate_Hs(Pos curr_pos, Pos dest_pos)
	{
		int Hs = 0;
		while (curr_pos != dest_pos)
		{
			if (curr_pos.x != dest_pos.x) {
				curr_pos.x += clamp(dest_pos.x - curr_pos.x, -1, 1);
			}
			if (curr_pos.y != dest_pos.y) {
				curr_pos.y += clamp(dest_pos.y - curr_pos.y, -1, 1);
			}
			Hs++;
			if (!m_map[curr_pos.x][curr_pos.y]) Hs += 100;
			if (Hs > 300000) break;
		}
		return Hs;
	}

	vector<Node*> possible_NextNode(Node* curr, Node* existNodes, Pos start_pos, Pos dest_pos) {
		vector<Node*> nodes;

		for (short i = -1; i <= 1; ++i) {
			for (short j = -1; j <= 1; ++j) {
				if (i == 0 && j == 0)continue;
				short curr_x = curr->pos.x + i;
				short curr_y = curr->pos.y + j;
				if (existNodes->search({ curr_x, curr_y }))continue;

				if (curr_x == start_pos.x && curr_y == start_pos.y)continue;
				if (curr_x < 0 || curr_x >= WORLD_WIDTH-1)continue;
				if (curr_y < 0 || curr_y >= WORLD_HEIGHT-1)continue;
				if (m_map[curr_x][curr_y] == false)continue;

				Node* node = new Node{ {curr_x,curr_y}, curr->Gs + 1, calculate_Hs({curr_x,curr_y},dest_pos),curr };
				nodes.emplace_back(node);
			}
		}
		return nodes;
	}

	bool Check_Overlap(Node* N1, Node* N2, Node* next)
	{
		if (N1 == nullptr || N2 == nullptr)return false;
		if (N1->search(next->pos) != nullptr || N2->search(next->pos) != nullptr)
			return true;
		return false;
	}

	vector<Pos> execute_Astar(Pos& curr_pos, Pos& destPos) {
		Pos start_pos = curr_pos;
		Pos dest_pos = destPos;

		vector<Pos> v_Path;

		Node* N_Open = new Node;
		Node* N_Close = new Node;
		Node* N_NextLevelNodes = new Node;

		Node* temp = new Node(Node{ curr_pos,0,calculate_Hs(curr_pos,dest_pos) });
		N_Close->insert_front(temp);
		for (auto& node : possible_NextNode(N_Close->next, N_NextLevelNodes, start_pos, dest_pos)) {
			N_Open->insert(node);
		}
		/*N_Open->print();
		N_Close->print();
		cout << "----------1" << endl;*/
		while (true) {
			for (auto next : possible_NextNode(N_Open->next, N_NextLevelNodes, start_pos, dest_pos)) {
				if (next->Hs == 0) {
					Node* curr = next;
					while (curr->parent != nullptr) {
						v_Path.emplace(v_Path.begin(), curr->pos);
						curr = curr->parent;
					}
					return v_Path;
				}
				if (!Check_Overlap(N_Open, N_Close, next)) {
					N_NextLevelNodes->insert(next);
				}
			}

			N_Open->MoveToNodeFront(N_Close);

			while (N_NextLevelNodes->next != nullptr) {
				N_NextLevelNodes->MoveToNodeFront(N_Open);
			}

			//cout << "OPEN===========================" << endl;
			//N_Open->print();
			//cout << "CLOSE===========================" << endl;
			//N_Close->print();
			//cout << "NextLevelNodes===========================" << endl;
			//N_NextLevelNodes->print();
		}

	};
};
