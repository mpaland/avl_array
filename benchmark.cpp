#include <chrono>
#include "static_avl.hpp"
#include <stdlib.h>
#include <array>
#include <vector>
#include <map>
#include <iostream>
#include <memory>

std::string size2str(size_t s)
{
	if (s > 1024 * 1024 * 1024) {
		return std::to_string(double(s) / (1024 * 1024 * 1024)) + "Gb";
	}
	else if (s > 1024 * 1024)
	{
		return std::to_string(double(s) / (1024 * 1024)) + "Mb";
	}
	else if (s > 1024)
	{
		return std::to_string(double(s) / (1024)) + "kb";
	}
	return std::to_string(s) + "b";
}

template <size_t mapSize,size_t testCount,size_t missPercent>
void benchmark()
{
	using namespace std;
	using namespace nicehero;
	using namespace chrono;
	{
		vector<int> arrs0;
		vector<int> arrs;
		vector<int> arrs2;
		arrs0.resize(mapSize);
		arrs.resize(mapSize);
		arrs2.resize(mapSize);
		for (int i = 0; i < mapSize; ++i) {
			arrs0[i] = i + 1;
		}
		for (int i = 0; arrs0.size() > 0;++ i) {
			int x = rand() % arrs0.size();
			arrs[i] = arrs0[x];
			arrs0[x] = arrs0.back();
			arrs0.pop_back();
		}
		for (int i = 0; i < mapSize; ++i) {
			arrs2[i] = rand() % mapSize;
			if (rand() % 100 < missPercent) {
				arrs2[i] = 0;
			}
			else {
				arrs2[i] = arrs[arrs2[i]];
			}
		}

		{
			using AVL = static_avl<int, int, mapSize>;
			using MAP = map<int, int>;
			cout << "mapSize:" << mapSize << " testCount:" << testCount<< " missPercent:" << missPercent << endl;
			cout << "sizeof static_avl:" << size2str(sizeof(AVL)) << endl;
			auto start = system_clock::now();
			for (int j = 0; j < testCount; ++j) {
				auto avl = make_unique<AVL>();
				for (int i = 0; i < mapSize; ++i) {
 					avl->emplace(arrs[i], arrs[i]);
				}
			}
			auto end = system_clock::now();
			auto duration = double(duration_cast<microseconds>(end - start).count())
				* microseconds::period::num / microseconds::period::den;
			cout << "avl insert " << mapSize << "x" << testCount << " cost"
				<< duration
				<< "s QPS:" << uint32_t(double(mapSize) / duration * double(testCount)) << endl;

			start = system_clock::now();
			for (int j = 0; j < testCount; ++j) {
				MAP mm;
				for (int i = 0; i < mapSize; ++i) {
					mm.emplace(make_pair(arrs[i], arrs[i]));
				}
			}
			end = system_clock::now();
			duration = double(duration_cast<microseconds>(end - start).count())
				* microseconds::period::num / microseconds::period::den;
			cout << "std::map insert " << mapSize << "x" << testCount << " cost"
				<< duration
				<< "s QPS:" << uint32_t(double(mapSize) / duration * double(testCount)) << endl;

			{
				auto avl = make_unique<AVL>();
				for (int i = 0; i < mapSize; ++i) {
					avl->emplace(arrs[i], arrs[i]);
				}
				MAP mm;
				for (int i = 0; i < mapSize; ++i) {
					mm.emplace(make_pair(arrs[i], arrs[i]));
				}
				cout << "totalNum avl:" << size_t(avl->size()) << " map:" << mm.size() << endl;;
				int missNum = 0;
				start = system_clock::now();
				for (int j = 0; j < testCount; ++j) {
					missNum = 0;
					for (int i = 0; i < mapSize; ++i) {
						if (avl->find(arrs2[i]) == avl->end()) {
							++missNum;
						}
					}
				}
				end = system_clock::now();
				duration = double(duration_cast<microseconds>(end - start).count())
					* microseconds::period::num / microseconds::period::den;
				cout << "static_avl find " << mapSize << "x" << testCount
					<<" missNum:" << missNum << " cost:"
					<< duration
					<< "s QPS:" << uint32_t(double(mapSize) / duration * double(testCount)) << endl;

				start = system_clock::now();
				for (int j = 0; j < testCount; ++j) {
					missNum = 0;
					for (int i = 0; i < mapSize; ++i) {
						if (mm.find(arrs2[i]) == mm.end()) {
							++missNum;
						}
					}
				}
				end = system_clock::now();
				duration = double(duration_cast<microseconds>(end - start).count())
					* microseconds::period::num / microseconds::period::den;
				cout << "std::map find " << mapSize << "x" << testCount
					<< " missNum:" << missNum << " cost:"
					<< duration
					<< "s QPS:" << uint32_t(double(mapSize) / duration * double(testCount)) << endl;

				start = system_clock::now();
				for (int i = 0; i < testCount; ++i) {
					for (int i = 0; i < mapSize; ++i) {
						avl->erase(arrs[i]);
						avl->emplace(arrs[i], arrs[i]);
					}
				}
				end = system_clock::now();
				duration = double(duration_cast<microseconds>(end - start).count())
					* microseconds::period::num / microseconds::period::den;
				cout << "static_avl erase&insert " << mapSize << "x" << testCount << " cost:"
					<< duration
					<< "s QPS:" << uint32_t(double(mapSize) / duration * double(testCount)) << endl;

				start = system_clock::now();
				for (int i = 0; i < testCount; ++i) {
					for (int i = 0; i < mapSize; ++i) {
						mm.erase(arrs[i]);
						mm.emplace(arrs[i], arrs[i]);
					}
				}
				end = system_clock::now();
				duration = double(duration_cast<microseconds>(end - start).count())
					* microseconds::period::num / microseconds::period::den;
				cout << "std::map erase&insert " << mapSize << "x" << testCount << " cost:"
					<< duration
					<< "s QPS:" << uint32_t(double(mapSize) / duration * double(testCount)) << endl;

				start = system_clock::now();
				for (int i = 0; i < mapSize; ++i) {
					avl->erase(arrs[i]);
				}
				end = system_clock::now();
				duration = double(duration_cast<microseconds>(end - start).count())
					* microseconds::period::num / microseconds::period::den;
				cout << "static_avl erase " << mapSize << "cost:"
					<< duration
					<< "s QPS:" << uint32_t(double(mapSize) / duration * double(testCount)) << endl;

				start = system_clock::now();
				for (int i = 0; i < mapSize; ++i) {
					mm.erase(arrs[i]);
				}
				end = system_clock::now();
				duration = double(duration_cast<microseconds>(end - start).count())
					* microseconds::period::num / microseconds::period::den;
				cout << "std::map erase " << mapSize << "cost:"
					<< duration
					<< "s QPS:" << uint32_t(double(mapSize) / duration * double(testCount)) << endl;
			}
		}
	}
	cout << endl;
}

int main()
{
	benchmark<128,10000,20>();
	benchmark<128,10000,10>();
	benchmark<128,10000,1>();
	benchmark<1024,1000,10>();
	benchmark<1024,1000,0>();
	benchmark<65535,50,10>();
	benchmark<65535,50,0>();
	benchmark<500000,5,10>();
	benchmark<500000,5,0>();
	benchmark<5000000,1,10>();
	benchmark<5000000,1,0>();
	benchmark<50000000,1,10>();
	benchmark<50000000,1,0>();
}
