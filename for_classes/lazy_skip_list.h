#pragma once

// lazy sync skip list

#include <mutex>
#include <iostream>


//#include "fast_rand.hpp" #define RAND() fast_rand()
#include <random>

#define RAND() rand()





using namespace std;

template <size_t MAX_LEVEL = 10>
class lazy_skip_list
{
	//static constexpr size_t MAX_LEVEL = 10;
	struct alignas(64) node_t
	{
		static_assert(MAX_LEVEL > 2, "MAX_LEVEL must be greater than 2.");

		node_t*					next[MAX_LEVEL];
		int						key;
		int						height;
		volatile bool			is_removed;
		volatile bool			is_fully_linked;
		recursive_mutex			mtx;

		node_t() : next{}, key{}, height{ MAX_LEVEL }, is_removed{ }, is_fully_linked{}
		{}
		node_t(int my_key) : next{}, key{ my_key }, height{ MAX_LEVEL }, is_removed{ }, is_fully_linked{}
		{}
		node_t(int my_key, int height) : next{}, key{ my_key }, height{ height }, is_removed{}, is_fully_linked{}
		{}

		void lock()
		{
			mtx.lock();
		}

		void unlock()
		{
			mtx.unlock();
		}
	};


private:
	alignas(64) node_t head;
	alignas(64) node_t tail;

public:
	lazy_skip_list() : head{ 0x80000000L }, tail{ 0x7FFFFFFFL }
	{
		head.is_fully_linked = true;
		tail.is_fully_linked = true;
		//head.level = tail.level = MAX_LEVEL;

		for (auto& ptr : head.next)
		{
			ptr = &tail;
		}
	}

	~lazy_skip_list()
	{
		unsafe_clear();
	}

	void unsafe_clear()
	{
		node_t* to_delete;
		while (head.next[0] != &tail)
		{
			to_delete = head.next[0];
			head.next[0] = head.next[0]->next[0];
			delete to_delete;
		}

		for (auto& ptr : head.next)
		{
			ptr = &tail;
		}
	}

	int find(int key, node_t* preds[], node_t* currs[])
	{
		int		current_level = MAX_LEVEL - 1;
		int		founded = -1;
		node_t* pred = &head;


		for (int level = MAX_LEVEL - 1; level >= 0; level -= 1)
		{
			volatile node_t* curr = pred->next[level];
			while (key > curr->key)
			{
				pred = curr;
				curr = pred->next[level];
			}

			// 최초 찾앗는지 founded가 -1이면 
			if (founded == -1 && key == curr->key)
			{
				founded = level;
			}

			preds[level] = pred;
			currs[level] = curr;
		}


		// 최초 찾은 레벨.
		return founded;
	}


	int calc_top_level()
	{
		int top_level = 1; //높이가 0층은 없고 무조건 1층은 있어야 함. 

		// 랜덤하게 층을 정한다. 단 상위 층으로 가능 확률은 줄어든다.
		while (RAND() % 2 == 0) ////위로 올라갈수록 확률이 줄어들어야한다. if문 10층 쌓기 싫으니까 while로 
		{
			top_level++;

			if (MAX_LEVEL == top_level)
			{
				break;
			}
		}

		return top_level;
	}

	bool add(int key)
	{
		int top_level = calc_top_level();

		node_t* preds[MAX_LEVEL];
		node_t* succs[MAX_LEVEL];

		while (true)
		{
			int founded = find(key, preds, succs);
			if (-1 != founded) // 찾았으면.. 
			{
				node_t* node_found = succs[founded];
				if (false == node_found->is_removed) // 찾았는데 마킹이 되어 
				{
					while (false == node_found->fully_linked) // 마킹 기다림...
					{}

					return false; // 이미 있는 경우.
				}
				else // invliad 
				{
					continue;
				}
			}

			int highest_locked = -1; // 어디까지 락이 되어 있는가.
			{
				node_t* pred;
				node_t* succ;

				bool valid = true;

				for (int level = 0; valid && (level <= top_level); ++level)
				{
					pred = preds[level];
					succ = succs[level];
					pred->lock();
					highest_locked = level;
					valid = !pred->is_removed && !succ->is_removed && pred->next[level] == succ;
				}

				//if (false == valid)
				//{
				//	continue;
				//}


				// 유효하지 안다면 바져 나감.
				if (false == valid)
				{
					for (int level = 0; level <= highest_locked; ++level)
					{
						preds[level]->unlock(); // 락 건수만큼 unlock을 해줌.
					}
					continue;
				}

				node_t* new_node = new node_t{ key, top_level };
				for (int level = 0; level <= top_level; ++level)
				{
					new_node->next[level] = succs[level];
				}



				for (int level = 0; level <= top_level; ++level)
				{
					preds[level]->next[level] = new_node;
				}

				new_node->fully_linked = true;

				for (int level = 0; level <= highest_locked; ++level)
				{
					preds[level]->unlock();
				}

				return true;
			}
		}

	}

	bool remove(int key)
	{
		node_t* preds[MAX_LEVEL];
		node_t* succs[MAX_LEVEL];
		node_t* victim = nullptr;
		bool is_removed = false;
		int top_level = -1;

		while (true)
		{
			int founded = find(key, preds, succs);
			if (founded != -1) victim = succs[founded];
			if (is_removed
				|| (founded != -1 && (victim->fullyLinked && victim->top_level == founded && !victim->is_removed)))
			{
				if (!is_removed)
				{
					top_level = victim->top_level;
					victim->lock();
					if (victim->is_removed)
					{
						victim->unlock();
						return false;
					}
					victim->is_removed = true;
					is_removed = true;
				}
				int highest_locked = -1;
				{
					node_t* pred;
					node_t* succ;
					bool valid = true;
					for (int level = 0; valid && (level <= top_level); ++level)
					{
						pred = preds[level];
						pred->lock();
						highest_locked = level;
						valid = !pred->is_removed && pred->next[level] == victim;
					}
					if (!valid)
					{
						for (int level = 0; level <= highest_locked; ++level)
						{
							preds[level]->unlock();
						}
						continue;
					}
					for (int level = top_level; level >= 0; --level)
					{
						preds[level]->next[level] = victim->next[level];
					}
					victim->unlock();
					for (int level = 0; level <= highest_locked; ++level)
					{
						preds[level]->unlock();
					}
					return true;
				}
			}
		}
	}

	bool contain(int key)
	{
		node_t* preds[MAX_LEVEL];
		node_t* succs[MAX_LEVEL];

		int founded = find(key, preds, succs);
		return (founded != -1 && succs[founded]->fully_linked && false == succs[founded]->removed);
	}

	void unsafe_display20()
	{
		int c = 20;
		node_t* p = head.next[0];

		while (p != &tail)
		{
			cout << p->key << ", ";

			p = p->next[0];
			c--;

			if (c == 0)
			{
				break;
			}
		}
		cout << endl;
	}
};
