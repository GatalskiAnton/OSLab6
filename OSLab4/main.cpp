#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>

void blocksMul(const std::vector <std::vector <int>>&m1, const std::vector <std::vector <int>>&m2, std::vector < std::vector <int>>&resM, int blockI, int blockJ, int matrixSize, int blockSize)
{
	for (int i = blockI; i < std::min(blockI + blockSize,matrixSize); ++i)
		for (int j = blockJ; j < std::min(blockJ + blockSize,matrixSize); ++j)
			for(int k = 0; k < matrixSize; ++k)
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
			threads.emplace_back(blocksMul, std::cref(m1), std::cref(m2), std::ref(resM), blockI, blockJ, matrixSize, blockSize);
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

int main()
{
	int matrixSize = 30;
	std::vector< std::vector<int>> m1(matrixSize);
	std::vector< std::vector<int>> m2(matrixSize);
	std::vector< std::vector<int>> m3(matrixSize,std::vector<int>(matrixSize));
	getMatrix(m1, "matrix1.txt");
	getMatrix(m2, "matrix2.txt");
	for (int i = 1; i < matrixSize; i++)
	{
		threadMul(m1, m2, m3, matrixSize, i);
	}
}