#include "detectParticles.h"
#include <opencv2/opencv.hpp>

void mark(cv::Mat & src, std::vector<std::array<int, 3>> & particlesInfo)
{
	cv::cvtColor(src, src, cv::COLOR_GRAY2BGR);

	for (int i = 0; i < particlesInfo.size(); i++)
	{
		int x = particlesInfo[i][0], y = particlesInfo[i][1];
		int r = particlesInfo[i][2]/2;
		cv::circle(src, cv::Point(x, y), r, cv::Scalar(20, 30, 230), 2);
		cv::putText(src, std::to_string(particlesInfo[i][2]), cv::Point(x, y), 0, 0.4, 255,1.2);
	}
}

int main()
{
	for (int imageNum = 0; imageNum <= 54; imageNum++)
	{
		//输入
		std::string filePath = "D:\\我的文档\\颗粒检测备份\\src&dealedImage\\0000";
		std::string imageNum_str = std::to_string(imageNum);
		filePath.replace(filePath.length() - imageNum_str.length(),filePath.length() ,imageNum_str);
		std::string inputFilePath = filePath;
		inputFilePath.append(".bmp");

		cv::Mat src = cv::imread(inputFilePath, 0);

		//图像数据转化为BYTE
		int length = (int)(src.total() * src.elemSize());
		BYTE *buffer = new BYTE[length];
		memcpy(buffer, src.data, length * sizeof(BYTE));

		//检测颗粒
		std::vector<std::array<int, 3>> particlesInfo;
		detectParticles(buffer, src.cols, src.rows, particlesInfo, 5, 45);

		//test
		mark(src, particlesInfo);

		//输出
		std::string outputFilePath = filePath;
		outputFilePath.append("a.bmp");
		cv::imwrite(outputFilePath, src);
		
	}

//----------------------------------------------------------------------------------------------------------------------------
/*	cv::Mat src = cv::imread("D:\\我的文档\\颗粒检测备份\\src&dealedImage\\0008.bmp", 0);

	//图像数据转化为BYTE
	int length = (int)(src.total() * src.elemSize());
	BYTE *buffer = new BYTE[length];
	memcpy(buffer, src.data, length * sizeof(BYTE));

	//检测颗粒
	//std::vector<std::array<int, 3>> particlesInfo(1000);
	//particlesInfo.clear();
	std::vector<std::array<int, 3>> particlesInfo;
	detectParticles(buffer, src.cols, src.rows, particlesInfo, 2, 45);

	//test
	cv::namedWindow("src", 0);
	cv::imshow("src", src);

	mark(src, particlesInfo);

	cv::namedWindow("dealedImage", 0);
	cv::imshow("dealedImage", src);
	cv::waitKey();
*/
}