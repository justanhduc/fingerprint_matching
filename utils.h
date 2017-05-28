#pragma once

#include <cv.h>
#include <highgui.h>
#include <cxcore.h>
#include <mysql.h>
#include <string>
#include <windows.h>
#include <tchar.h>
#include <vector>
#include <opencv2/nonfree/features2d.hpp> 
#include <opencv2/nonfree/nonfree.hpp>
#include <WinSock.h>
#include <strsafe.h>
#include <mutex>
#include <thread>
#include <future>

using namespace cv;
using namespace std;

//data containers
using VecMat = vector<Mat>;
using VecString = vector<string>;
using VecFloat = vector<float>;
using VecInt = vector<int>;

//HOG
//324: 192 64 32
//225: 160 32 32
//144: 128 64 32
#define RESIZED_SIZE		192
#define WINDOW_SIZE			192
#define BLOCK_SIZE			64
#define BLOCK_STRIDE		64
#define CELL_SIZE			32
#define NUM_BINS			9
#define PYRAMID				1
#define DIMENSION			(WINDOW_SIZE / BLOCK_STRIDE)*(WINDOW_SIZE / BLOCK_STRIDE)*(BLOCK_SIZE / CELL_SIZE)*(BLOCK_SIZE / CELL_SIZE)*NUM_BINS

//roi extraction
#define FIXED_WIDTH			700
#define ROI_MIN_SIZE		128
#define CANNY_THRESHOLD1	120
#define CANNY_THRESHOLD2	300

//mysql db
#define DB_HOST				"localhost"
#define DB_USER				"root"
#define DB_PASS				"insight7734"
#define DB_NAME				"webtoon"
#define DB_PORT				3307
#define DB_METADATA			"webtoon_metadata_tmp"
#define DB_DESCRIPTOR		"webtoon_descriptor_tmp"
#define DB_LIMIT			30000

//
#define NUM_SLAVES		3

//multithreading
#define NUM_THREADS		10//thread::hardware_concurrency()

extern mutex m;

//dir file
void DisplayErrorBox(LPTSTR lpszFunction);
VecString dir(string directory, bool folder = false);

//connect to database
MYSQL *connectTo(MYSQL* mysql, string host, string user, string password, string name, int port,
	char *socket, int flag);
void executeQueryNoResult(string query, MYSQL* connection, MYSQL *mysql);
MYSQL_RES *executeQueryGetResult(string query, MYSQL* connection, MYSQL *mysql);

//Miscellaneous
string strtok(string str, string delimiter = " ");
VecString strsplit(string str, string delimiter = " ");
void logThis(string file, string log);
void writeMatToFile(Mat m, string file);
float hamming(Mat mat1, Mat mat2, bool weighting = false);
Mat norm(Mat mat1, Mat mat2, int metric = 1);

//
