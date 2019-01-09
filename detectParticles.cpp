#include "detectParticles.h"
#include <time.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

cv::Mat bytesToMat(BYTE * bytes, int width, int height)
{
	cv::Mat image = cv::Mat(height, width, CV_8U, bytes).clone();
	return image;
}

void rowsProjection(cv::Mat & img,cv::Mat & rowsProjImg)
{
	for (int i = 0; i < img.rows; i++)
	{
		uchar * rowData = img.ptr<uchar>(i);

		int rowWhitePixNum = 0;
		for (int j = 0; j < img.cols; j++)
		{
			if (rowData[j] == 255)
				rowWhitePixNum++;
		}

		cv::line(rowsProjImg, cv::Point(0,i), cv::Point(rowWhitePixNum,i), cv::Scalar(255));
	}
}
void cutRowLines(cv::Mat & threshImg)
{
	cv::Mat dilatedThreshImg;

	//膨胀
	cv::dilate(threshImg, dilatedThreshImg, cv::Mat(15, 15, CV_8U, cv::Scalar(1)));

	//test
//	cv::namedWindow("dilatedThreshImg", 0);
//	cv::imshow("dilatedThreshImg", dilatedThreshImg);

	//水平投影
	cv::Mat rowsProjImg(dilatedThreshImg.size(),CV_8U,cv::Scalar(0));
	rowsProjection(dilatedThreshImg, rowsProjImg);

	//test
//	cv::namedWindow("rowsProjImg", 0);
//	cv::imshow("rowsProjImg", rowsProjImg);

	cv::threshold(rowsProjImg, rowsProjImg, 170, 255, CV_THRESH_BINARY_INV);

	//检测切割段
	std::vector<std::array<int, 2>> cutRange;
	int midX = 7*threshImg.cols / 8;
	int blackPixLength = 0;
	bool isInBlack = false;
	std::array<int,2> yrange;
	for (int i = 0; i < threshImg.rows; i++)
	{
		if (rowsProjImg.at<uchar>(i, midX) == 0)
		{
			if (isInBlack)
				blackPixLength++;
			else
			{
				isInBlack = true;
				yrange[0] = i;
				blackPixLength++;
			}
		}
		else if (isInBlack)
		{
			isInBlack = false;
			
			if (blackPixLength > 40)
			{
				yrange[1] = i;
				cutRange.push_back(yrange);
			}

			blackPixLength = 0;
		}
	}

	//横向分割
	for (int i = 0; i < cutRange.size(); i++)
	{
		cv::Mat ROI = threshImg.rowRange(cutRange[i][0], cutRange[i][1]);
		ROI = cv::Scalar(0);
	}

}

void cutColEdge(cv::Mat & img)
{
	//纵向开运算
	cv::Mat kernel_col(100, 1, CV_8U, cv::Scalar(1));
	cv::Mat tmp_img;
	cv::erode(img, tmp_img, kernel_col);
	cv::dilate(tmp_img, tmp_img, kernel_col);

	cv::dilate(tmp_img, tmp_img, cv::Mat(3, 3, CV_8U, cv::Scalar(1)));

	img = (img != tmp_img);
}

void cutColLines(cv::Mat & cutted_img)
{
	cv::Mat ROI1 = cutted_img.colRange(0, 15);
	cv::Mat ROI2 = cutted_img.colRange(cutted_img.cols-15, cutted_img.cols - 1);

	cutColEdge(ROI1);
	cutColEdge(ROI2);

	cv::line(cutted_img, cv::Point(0, 0), cv::Point(0, cutted_img.rows - 1), cv::Scalar(0), 3);
	cv::line(cutted_img, cv::Point(0, 0), cv::Point(cutted_img.cols - 1, 0), cv::Scalar(0), 3);
	cv::line(cutted_img, cv::Point(cutted_img.cols - 1, cutted_img.rows - 1), cv::Point(0, cutted_img.rows - 1), cv::Scalar(0), 3);
	cv::line(cutted_img, cv::Point(cutted_img.cols - 1, cutted_img.rows - 1), cv::Point(cutted_img.cols - 1, 0), cv::Scalar(0), 3);

}

void selectContours(const std::vector<std::vector<cv::Point>> contours,
	std::vector<std::vector<cv::Point>> & contours2, const int minDiameter)
{
	for (unsigned int i = 0; i<contours.size(); i++)
	{
		double area = cv::contourArea(contours[i]);
		cv::Rect bdrect = cv::boundingRect(contours[i]);
		if (area > 4000 )
			continue;
		else if ((bdrect.width+bdrect.height)/2 < minDiameter)
			continue;
		else if (bdrect.height / bdrect.width > 6 || bdrect.width / (float)bdrect.height >= 2.9)
			continue;
		else if (bdrect.area() / area > 4)
			continue;
		else
			contours2.push_back(contours[i]);
	}
}

void addPatches(const cv::Mat img, const std::vector<std::vector<cv::Point>> contours,
	std::vector<std::vector<cv::Point>> & contours2)
{
	cv::Mat tmpImg(img.size(), CV_8U, cv::Scalar(0));
	cv::drawContours(tmpImg, contours, -1, 255, -1);

	for (int i = 0; i < contours.size(); i++)
	{
		cv::Rect bdrect = cv::boundingRect(contours[i]);
		int x0 = bdrect.x, y0 = bdrect.y, w = bdrect.width, h = bdrect.height;

		if (x0 < 10)
		{
			int colLinePixNum = 0;
			for (int y = y0; y <= y0 + h; y++)
			{
				if (tmpImg.at<uchar>(y, x0) == 255)
					colLinePixNum++;
			}

			if (colLinePixNum > 15)
				for (int y = y0; y <= y0 + h; y++)
				{
					if (tmpImg.at<uchar>(y, x0) == 255)
						cv::line(tmpImg, cv::Point(x0,y), cv::Point(0,y), cv::Scalar(255));
				}
		}

		if (img.cols - (x0 + w) < 10)
		{
			int colLinePixNum = 0;
			for (int y = y0; y <= y0 + h; y++)
			{
				if (tmpImg.at<uchar>(y, x0 + w-1) == 255)
					colLinePixNum++;
			}

			if (colLinePixNum > 15)
				for (int y = y0; y <= y0 + h; y++)
				{
					if (tmpImg.at<uchar>(y, x0 + w - 1) == 255)
						cv::line(tmpImg, cv::Point(x0+w-1,y), cv::Point(img.cols-1,y), cv::Scalar(255));
				}
		}

	}

	cv::findContours(tmpImg, contours2, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

}

void erodeContours(const cv::Mat img,std::vector<std::vector<cv::Point>> & contours,
	std::vector<std::array<int, 3>> & particlesInfo, const int maxTurns, const int minDiameter, const int maxDiameter)
{
	//构造内核
	cv::Mat kernel(6, 6, CV_8U, cv::Scalar(1));
	kernel.at<uchar>(0, 0) = 0;
	kernel.at<uchar>(0, 5) = 0;
	kernel.at<uchar>(5, 0) = 0;
	kernel.at<uchar>(5, 5) = 0;
	kernel.at<uchar>(0, 1) = 0;
	kernel.at<uchar>(1, 0) = 0;
	kernel.at<uchar>(0, 4) = 0;
	kernel.at<uchar>(4, 0) = 0;
	kernel.at<uchar>(1, 5) = 0;
	kernel.at<uchar>(5, 1) = 0;
	kernel.at<uchar>(5, 4) = 0;
	kernel.at<uchar>(4, 5) = 0;

	cv::Mat canvas(img.size(), img.type(), cv::Scalar(0));
	cv::drawContours(canvas, contours, -1, 255, -1);
	cv::Mat eroded_img;

	std::vector<std::vector<cv::Point>> contours_eroded;
	std::vector<std::vector<cv::Point>> contours_tmp;

	cv::Mat avoidOverlapImg(img.size(), CV_8UC1,cv::Scalar(0));

	for (int turns = 0; turns<maxTurns; turns++)
	{
		bool isMostEroded = true;

		//腐蚀
		cv::erode(canvas, eroded_img, kernel);

		//test
//		cv::namedWindow("eroded_img", 0);
//		cv::imshow("eroded_img", eroded_img);
//		cv::waitKey();

		//提取轮廓      
		cv::findContours(eroded_img, contours_eroded, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

		//更新轮廓
		contours_tmp.clear();
		for (int i = 0; i<contours.size(); i++)
		{
			cv::Rect bdrect1 = cv::boundingRect(contours[i]);

			int innerRectNum = 0;

			for (int j = 0; j<contours_eroded.size(); j++)
			{
				cv::Rect bdrect2 = cv::boundingRect(contours_eroded[j]);
				if (bdrect2.x > bdrect1.x && bdrect2.y > bdrect1.y
					&& bdrect2.x + bdrect2.width < bdrect1.x + bdrect1.width
					&& bdrect2.y + bdrect2.height < bdrect1.y + bdrect1.height)
				{
					innerRectNum++;
					contours_tmp.push_back(contours_eroded[j]);
				}
			}

			if (innerRectNum == 0)
			{
				cv::Rect bdrect = cv::boundingRect(contours[i]);

				//if (avoidOverlapImg.at<uchar>(bdrect.y+bdrect.height, bdrect.x+bdrect.width) == 0)
				//{
					int diameter;//根据腐蚀次数逆推直径大小

					diameter = (bdrect.width + bdrect.height) / 2 + (kernel.rows - 1) * turns;

					//if (diameter > minDiameter && diameter < maxDiameter)
					//{
						std::array<int, 3> infoArray;
						infoArray[0] = bdrect.x;
						infoArray[1] = bdrect.y;
						infoArray[2] = diameter;
						particlesInfo.push_back(infoArray);

						//cv::circle(avoidOverlapImg, cv::Point(bdrect.x + bdrect.width, bdrect.y + bdrect.height), 8, 255, -1);
					//}
				//}
			}
			else
				isMostEroded = false;

		}

		canvas = cv::Scalar(0);
		cv::drawContours(canvas, contours_tmp, -1, 255, -1);

		contours.assign(contours_tmp.begin(), contours_tmp.end());

		if (isMostEroded == true)
			break;

	}
}

std::string getSysTime()
{
	time_t timep;
	time(&timep);
	char tmp[64];
	strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&timep));
	return tmp;
}
bool isInTrialPeriod()
{
	std::string time = getSysTime();

	if (time < "2018-10-01 00:00:00")
		return true;
	else
		return false;
}

void detectParticles(BYTE* pbuff, const int width, const int height, std::vector<std::array<int, 3>> & particlesInfo,const int minDiameter, const int maxDiameter)
{
	//不在试用期内则终止程序
	if (!isInTrialPeriod())
	{
		std::cout << "试用期已过" << std::endl;
		cv::namedWindow("warning");
		cv::Mat warningImage(50, 320, CV_8U, 255);
		cv::putText(warningImage, "Out of trial period!",cv::Point(10,30), cv::FONT_HERSHEY_COMPLEX,0.9,0);
		cv::imshow("warning", warningImage);
		cv::waitKey();
		return;
	}

	//读入
	cv::Mat img;
	img = bytesToMat(pbuff, width, height);

	//自适应阈值+反相
	cv::adaptiveThreshold(img, img, 255, 0, CV_THRESH_BINARY_INV, 95, 10);

	//test
//	cv::namedWindow("thresholdedImg", 0);
//	cv::imshow("thresholdedImg", img);

	//纵向分割
	cutColLines(img);

	//test
//	cv::namedWindow("cuttedImg", 0);
//	cv::imshow("cuttedImg", img);

	//一次开运算
	cv::morphologyEx(img, img, cv::MORPH_OPEN, cv::Mat(3, 3, CV_8U, 1));

	//test
//	cv::namedWindow("openedImg", 0);
//	cv::imshow("openedImg", img);

	//提取轮廓
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(img, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	//test
//	cv::Mat contoursImg1(img.size(),img.type(),cv::Scalar(0));
//	cv::drawContours(contoursImg1, contours, -1, 255, 2);
//	cv::namedWindow("contoursImg1", 0);
//	cv::imshow("contoursImg1", contoursImg1);

	//筛选轮廓
	std::vector<std::vector<cv::Point>> contours2;
	selectContours(contours, contours2, minDiameter);

	//test
//	cv::Mat contoursImg2(img.size(), img.type(), cv::Scalar(0));
//	cv::drawContours(contoursImg2, contours2, -1, 255, 2);
//	cv::namedWindow("contoursImg2", 0);
//	cv::imshow("contoursImg2", contoursImg2);

	//修补左右边缘的轮廓
	std::vector<std::vector<cv::Point>> contours_fixed;
	addPatches(img,contours2,contours_fixed);

	//test
//	cv::Mat contoursImg3(img.size(), img.type(), cv::Scalar(0));
//	cv::drawContours(contoursImg3, contours_fixed, -1, 255, 2);
//	cv::namedWindow("contoursImg3", 0);
//	cv::imshow("contoursImg3", contoursImg3);

	//腐蚀+更新轮廓
	erodeContours(img, contours_fixed, particlesInfo, 15, minDiameter, maxDiameter);

}