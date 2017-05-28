#pragma once

#include "utils.h"
#include "webtoonprocessing.h"

Mat pyramidHOG(Mat image, int pyLvl, int wSize, int bSize, int bStride, int cSize, int numBins) {
	if (wSize == 0)
		wSize = image.rows >= image.cols ? image.cols : image.rows;

	VecFloat descriptors;
	Mat imgPyr = image.clone();
	for (int i = 0; i < pyLvl; ++i) {
		HOGDescriptor hogPyr(Size(int(wSize / pow(2, i)), int(wSize / pow(2, i))), Size(bSize, bSize),
			Size(bStride, bStride), Size(cSize, cSize), numBins);
		VecFloat descriptorsPyr;
		hogPyr.compute(imgPyr, descriptorsPyr);
		descriptors.insert(descriptors.end(), descriptorsPyr.begin(), descriptorsPyr.end());
		pyrDown(imgPyr, imgPyr, Size(int(imgPyr.rows / 2), int(imgPyr.cols / 2)));
	}
	Mat des = Mat(1, (int)descriptors.size(), CV_32FC1);
	memcpy(des.data, descriptors.data(), descriptors.size()*sizeof(float));
	return des;
}

VecMat extractROI(Mat image) {
	int h = image.rows;
	int w = image.cols;
	VecMat colRoiList{ Mat(h, w, CV_32FC3) };
	Mat cannyImg;
	Canny(image, cannyImg, CANNY_THRESHOLD1, CANNY_THRESHOLD2);
	cannyImg.convertTo(cannyImg, CV_32F);
	colRoiList = sumAndCopy(image, cannyImg);
	VecMat roiList;
	for (VecMat::iterator it = colRoiList.begin(); it != colRoiList.end(); ++it) {
		Mat cannyImg;
		it->convertTo(*it, CV_8U);
		Canny(*it, cannyImg, CANNY_THRESHOLD1, CANNY_THRESHOLD2);
		cannyImg.convertTo(cannyImg, CV_32F);
		VecMat roiListTmp;
		roiListTmp = sumAndCopy(it->t(), cannyImg.t(), true);
		if (roiListTmp.size() == 0)
			continue;
		flip(roiListTmp[0], roiListTmp[0], 0);
		Mat cannyCroppedFlipped = cannyImg.t();
		flip(cannyCroppedFlipped.rowRange(cannyCroppedFlipped.rows - roiListTmp[0].rows - 1, cannyCroppedFlipped.rows - 1),
			cannyCroppedFlipped, 0);
		roiListTmp = sumAndCopy(roiListTmp[0], cannyCroppedFlipped, true);
		if (roiListTmp.size() == 0)
			continue;
		flip(roiListTmp[0], roiListTmp[0], 0);
		roiListTmp[0] = roiListTmp[0].t();
		roiList.insert(roiList.end(), roiListTmp.begin(), roiListTmp.end());
	}
	return roiList;
}

VecMat sumAndCopy(Mat image, Mat cannyImg, bool topOnly) {
	int h = image.rows;
	int w = image.cols;
	bool check = false;
	VecMat roiList;
	Mat sum;
	int t = 0, b, e;
	reduce(cannyImg, sum, 1, CV_REDUCE_SUM);
	for (int i = 0; i < h; ++i) {
		if (sum.at<float>(i, 0) == 0) {
			if (t == 0)
				continue;
			else {
				check = false;
				t = 0;
				e = i;
				if (e - b>ROI_MIN_SIZE) {
					Mat roiTmp;
					image.rowRange(b, e).copyTo(roiTmp);
					roiList.push_back(roiTmp);
				}
				else
					continue;
			}
		}
		else {
			if (check) {
				if (topOnly) {
					Mat roiTmp;
					image.rowRange(b, h - 1).copyTo(roiTmp);
					roiList.push_back(roiTmp);
					break;
				}
				else 
					continue;
			}
			else {
				check = true;
				t++;
				b = i;
			}
		}
	}
	if (!topOnly)
		if (check) {
			e = h;
			if (e - b>ROI_MIN_SIZE) {
				Mat roiTmp;
				image.rowRange(b, e).copyTo(roiTmp);
				roiList.push_back(roiTmp);
			}
		}
	return roiList;
}

Mat binarize(Mat mat, int numOnes) {
	Mat idx;
	sortIdx(mat, idx, CV_SORT_EVERY_ROW + CV_SORT_DESCENDING);
	Mat binarizedMat = Mat::zeros(mat.rows, mat.cols, CV_32F);
	for (int i = 0; i < idx.rows; ++i)
		for (int j = 0; j < numOnes; ++j)
			binarizedMat.at<float>(i, idx.at<int>(i, j)) = 1.;
	return binarizedMat;
}

Mat binarizeDescriptors(Mat descriptor, int range, int numOnes) {
	Mat binary = binarize(descriptor.colRange(0, range), numOnes);
	for (int i = 1; i < (descriptor.cols/range); ++i) {
		Mat bin = binarize(descriptor.colRange(i*range, (i + 1)*range), numOnes);
		hconcat(binary, bin, binary);
	}
	return binary;
}
