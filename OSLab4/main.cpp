#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include "bufferedChannel.h"

std::vector<double> threadResults;
std::vector<double> seqResults;

std::mutex g_lock;

void blocksMul(const std::vector <std::vector <int>>&m1, const std::vector <std::vector <int>>&m2, std::vector < std::vector <int>>&resM, int blockI, int blockJ, int matrixSize, int blockSize)
{
	for (int i = blockI; i < std::min(blockI + blockSize,matrixSize); ++i)
		for (int j = blockJ; j < std::min(blockJ + blockSize,matrixSize); ++j)
			for (int k = 0; k < matrixSize; ++k)
				resM[i][j] += m1[i][k] * m2[k][j];			
}

void blocksMulUsingChannel(const std::vector <std::vector <int>>& m1, const std::vector <std::vector <int>>& m2, std::vector < std::vector <int>>& resM,  std::pair<int,int> & pair, int matrixSize, int blockSize)
{	
	for (int i = pair.first; i < std::min(pair.first + blockSize, matrixSize); ++i)
		for (int j = pair.second; j < std::min(pair.second + blockSize, matrixSize); ++j)
			for (int k = 0; k < matrixSize; ++k) 
				resM[i][j] += m1[i][k] * m2[k][j];	
}

void defaultMul(const std::vector <std::vector <int>>& m1, const std::vector <std::vector <int>>& m2, std::vector < std::vector <int>>& resM, int matrixSize, int blockSize)
{
	for (int blockI = 0; blockI < matrixSize; blockI += blockSize)
		for (int blockJ = 0; blockJ < matrixSize; blockJ += blockSize)
			blocksMul(m1, m2, resM, blockI, blockJ, matrixSize, blockSize);
}

void get1(const std::vector <std::vector <int>>& m1, const std::vector <std::vector <int>>& m2, std::vector < std::vector <int>>& resM, int matrixSize, int blockSize, BufferedChannel < std::pair<int, int>>& channel)
{
	std::pair<std::pair<int, int>, bool> pair = channel.recv();
	while (pair.second)
	{
		 blocksMulUsingChannel(m1, m2, resM, pair.first, matrixSize, blockSize);
		 pair = channel.recv();
		 std::cout << '\n';
	}	
}

void threadMul(const std::vector <std::vector <int>>& m1, const std::vector <std::vector <int>>& m2, std::vector < std::vector <int>>& resM, int matrixSize, int blockSize)
{
	std::vector<std::thread> threads;
	int buffSize = std::pow(matrixSize % blockSize == 0 ? matrixSize / blockSize : matrixSize / blockSize + 1,2);
	BufferedChannel<std::pair<int,int>> channel(buffSize);
	for (int blockI = 0; blockI < matrixSize; blockI += blockSize)
		for (int blockJ = 0; blockJ < matrixSize; blockJ += blockSize)
		{
			std::pair<int, int> blockPair = std::make_pair(blockI,blockJ);
			channel.send(std::move(blockPair));
		}

	channel.close();
	for (int i = 0; i < 16; ++i)
		threads.emplace_back(get1,std::ref(m1), std::ref(m2), std::ref(resM),matrixSize, blockSize, std::ref(channel));
	
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

void test(std::vector< std::vector<int>>& m1, std::vector< std::vector<int>>& m2, std::vector< std::vector<int>>& m3, int matrixSize)
{
	std::ofstream out("output.txt");
	auto startTimeThread =  std::chrono::high_resolution_clock::now();
	for (int i = 1; i < matrixSize; ++i)
	{
		threadMul(m1, m2, m3, matrixSize, i);
		auto tempTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> tempDuration = tempTime - startTimeThread;
		threadResults.push_back(tempDuration.count());
		m3 = std::vector<std::vector<int>>(matrixSize, std::vector<int>(matrixSize));
	}
	auto endTimeThread = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> durationThread = endTimeThread - startTimeThread;
	std::cout << durationThread.count() << '\n';

	auto startTimeSeq = std::chrono::high_resolution_clock::now();
	for (int  i = 1; i < matrixSize; ++i)
	{
		defaultMul(m1, m2, m3, matrixSize, i);
		auto tempTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> tempDuration = tempTime - startTimeSeq;
		seqResults.push_back(tempDuration.count());
		m3 = std::vector<std::vector<int>>(matrixSize, std::vector<int>(matrixSize));
	}
	auto endTimeSeq = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> durationSeq = endTimeSeq - startTimeSeq;
	std::cout << durationSeq.count() << '\n';
	out << "------------------------------------\n";
	for (int i = 0; i < threadResults.size(); ++i)
	{
		out << "| Block size " << i+1 << ", difference " << seqResults[i] / threadResults[i]  << '\n';
		out << "------------------------------------\n";
	}
	out << "final difference " << durationSeq.count() /  durationThread.count() << '\n';

}


int main()
{
	int matrixSize = 100;
	std::vector< std::vector<int>> m1(matrixSize);
	std::vector< std::vector<int>> m2(matrixSize);
	std::vector< std::vector<int>> m3(matrixSize,std::vector<int>(matrixSize));
	getMatrix(m1, "matrix5.txt");
	getMatrix(m2, "matrix6.txt");
	//test(m1, m2, m3, matrixSize);

	
		threadMul(m1, m2, m3, matrixSize, 3);


		for (auto& a: m3)
		{
			for (auto& b : a)
				std::cout << b <<  ' ';
			std::cout << '\n';
		}

	

}