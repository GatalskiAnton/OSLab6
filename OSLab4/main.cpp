#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>


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

void defaultMul(const std::vector <std::vector <int>>& m1, const std::vector <std::vector <int>>& m2, std::vector < std::vector <int>>& resM, int matrixSize, int blockSize)
{
	for (int blockI = 0; blockI < matrixSize; blockI += blockSize)
		for (int blockJ = 0; blockJ < matrixSize; blockJ += blockSize)
			blocksMul(m1, m2, resM, blockI, blockJ, matrixSize, blockSize);
}

void threadMul(const std::vector <std::vector <int>>& m1, const std::vector <std::vector <int>>& m2, std::vector < std::vector <int>>& resM, int matrixSize, int blockSize)
{
	std::vector<std::thread> threads;
	for (int blockI = 0; blockI < matrixSize; blockI += blockSize)
		for (int blockJ = 0; blockJ < matrixSize; blockJ += blockSize)
		{
			g_lock.lock();
			threads.emplace_back(blocksMul, std::cref(m1), std::cref(m2), std::ref(resM), blockI, blockJ, matrixSize, blockSize);
			g_lock.unlock();
		}

	for (auto& thrd: threads)
		thrd.join();
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
	int matrixSize = 300;
	std::vector< std::vector<int>> m1(matrixSize);
	std::vector< std::vector<int>> m2(matrixSize);
	std::vector< std::vector<int>> m3(matrixSize,std::vector<int>(matrixSize));
	getMatrix(m1, "matrix1.txt");
	getMatrix(m2, "matrix2.txt");
	test(m1, m2, m3, matrixSize);
}