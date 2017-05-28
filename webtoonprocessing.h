#pragma once

#include <cv.h>

#include <vector>

vector<Mat> extractROI(Mat image);
Mat pyramidHOG(Mat image, int pyLvl, int wSize, int bSize, int bStride, int cSize, int numBins);
vector<Mat> sumAndCopy(Mat image, Mat cannyImg, bool topOnly = false);
Mat binarize(Mat mat, int numOnes = 4);
Mat binarizeDescriptors(Mat descriptor, int range, int numOnes = 4);