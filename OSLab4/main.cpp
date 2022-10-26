#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include "bufferedChannel.h"


std::vector<double> threadResults;

std::mutex g_lock;

void blocksMulUsingChannel(const std::vector <std::vector <int>>& m1, const std::vector <std::vector <int>>& m2, std::vector < std::vector <int>>& resM, std::pair<int, int>& pair, int matrixSize, int blockSize)
{
	for (int i = pair.first; i < std::min(pair.first + blockSize, matrixSize); ++i)
		for (int j = pair.second; j < std::min(pair.second + blockSize, matrixSize); ++j)
			for (int k = 0; k < matrixSize; ++k)
			{
				g_lock.lock();
				resM[i][j] += m1[i][k] * m2[k][j];
				g_lock.unlock();
			}
}


void threadMulUsingChannel(const std::vector <std::vector <int>>& m1, const std::vector <std::vector <int>>& m2, std::vector < std::vector <int>>& resM, int matrixSize, int blockSize, BufferedChannel < std::pair<int, int>>& channel)
{
	std::pair<std::pair<int, int>, bool> pair = channel.recv();
	while (pair.second)
	{
		 blocksMulUsingChannel(m1, m2, resM, pair.first, matrixSize, blockSize);
		 pair = channel.recv();
	}	
}

void threadMul(const std::vector <std::vector <int>>& m1, const std::vector <std::vector <int>>& m2, std::vector < std::vector <int>>& resM, int matrixSize, int blockSize, int countThreads)
{
	std::vector<std::thread> threads;
	int buffSize = std::pow(matrixSize % blockSize == 0 ? matrixSize / blockSize : matrixSize / blockSize + 1,2);
	BufferedChannel<std::pair<int,int>> channel(buffSize);
	for (int blockI = 0; blockI < matrixSize; blockI += blockSize)
		for (int blockJ = 0; blockJ < matrixSize; blockJ += blockSize)
		{
			std::pair<int, int> blockPair = { blockI,blockJ };
			channel.send(std::move(blockPair));
		}

	channel.close();
	for (int i = 0; i < countThreads; ++i)
		threads.emplace_back(threadMulUsingChannel, std::ref(m1), std::ref(m2), std::ref(resM), matrixSize, blockSize, std::ref(channel));
	
	for (auto& thread: threads)
		thread.join();
}

void getMatrix(std::vector< std::vector<int>> &m1, std::string file)
{
	std::ifstream input(file);
	std::string row;
	int k = 0;
	while (getline(input,row))
	{
		std::stringstream ss;
		ss.str(row);
		std::string value;
	
		while (ss >> value)
		{
			m1[k].push_back(std::stoi(value));
		}
		++k;
	}
}

void testChannel(std::vector< std::vector<int>>& m1, std::vector< std::vector<int>>& m2, std::vector< std::vector<int>>& m3, int matrixSize,int blockSize, int countThreads)
{
	std::ofstream out("outputChannel.txt");
	auto startTimeThread = std::chrono::high_resolution_clock::now();
	for (int i = 1; i <= countThreads; ++i)
	{
		threadMul(m1, m2, m3, matrixSize, blockSize, i);
		auto tempTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> tempDuration = tempTime - startTimeThread;
		threadResults.push_back(tempDuration.count());
		m3 = std::vector<std::vector<int>>(matrixSize, std::vector<int>(matrixSize));
	}
	out << "------------------------------------\n";
	for (int i = 0; i < threadResults.size() - 1; ++i)
	{
		out << "| Count of thread: " << i + 1  << ", time: " << threadResults[i + 1] - threadResults[i] << '\n';
		out << "------------------------------------\n";
	}
}


int main()
{
	int matrixSize = 100;
	std::vector< std::vector<int>> m1(matrixSize);
	std::vector< std::vector<int>> m2(matrixSize);
	std::vector< std::vector<int>> m3(matrixSize,std::vector<int>(matrixSize));
	getMatrix(m1, "matrix5.txt");
	getMatrix(m2, "matrix6.txt");
	testChannel(m1, m2, m3, matrixSize, 10,40);
}