#pragma once 

#include <time.h>
#include <fstream>
#include <future>
#include <iomanip>
#include <ctime>
#include <tuple>

#include "utils.h"
#include "webtoonprocessing.h"

#pragma comment(lib, "User32.lib")

template <typename T=int>
inline T findMostFrequent(vector<T> vec);

template <typename KeyType, typename FunPtrType, typename Comp>
void Switch(const KeyType &value, initializer_list<pair<const KeyType, FunPtrType>> sws, Comp comp);

template <typename... T>
VecInt identify(VecString queryList, tuple<T...> database, string testingDir, bool mode, int metric, string logFile, string logFile2);

int main() {
	clock_t startTime = clock();
	while (1) {
		cout << "Select an option" << endl;
		cout << "\t[1] Construct a database" << endl;
		cout << "\t[2] Identify webtoon images" << endl;
		cout << "\t[3] Analyze mis-identification" << endl;
		cout << "\t[4] Export database to binary file" << endl;
		cout << "\t[5] Extract ROIs" << endl;
		cout << "\t[6] Extract fingerprint of images" << endl;
		cout << "\t[7] Identify ROIs" << endl;
		cout << "\t[8] Binarize a database file" << endl;
		cout << "\t[9] Print out fingerprints" << endl;

		string c;
		getline(cin, c);
		Switch<const string, void(*)()>(c.c_str(), { // sorted:                      
			{ "1", []{
				cout << "Connect to the Webtoon database... ";
				MYSQL *connection = NULL, mysql;
				connection = connectTo(&mysql, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, (char*)NULL, 0);
				cout << "Connected!" << endl << endl;

				clock_t start = clock();
				ofstream logfile;
				string logFile = "dbstore.log";
				logfile.open(logFile, ios::trunc);
				logfile.close();
				string dirStr;
				cout << "Webtoon company folder: ";
				getline(cin, dirStr);
				VecString dirList = dir(dirStr, true);

				int count = 0;
				for (int m = 20; m < 400; ++m) {
					ostringstream strstr;
					strstr << dirStr << "\\" << dirList[m];
					string dirStrChild = strstr.str();
					VecString folderList = dir(dirStrChild.c_str(), true);
					for (VecString::iterator itFol = folderList.begin(); itFol != folderList.end(); ++itFol) {
						ostringstream ss;
						ss << dirStrChild << "\\" << *itFol;
						string fileStrChild = ss.str();
						VecString fileList = dir(fileStrChild);
						clock_t processTime = clock();
						for (VecString::iterator itF = fileList.begin(); itF != fileList.end(); ++itF) {
							ostringstream fss;
							fss << fileStrChild << "\\" << *itF;
							string fullPath = fss.str();
							int pageId = atoi(strtok(*itF, ".").c_str());

							cout << "Processing " << fullPath << endl;
							try {
								Mat img;
								img = imread(fullPath.c_str(), CV_LOAD_IMAGE_COLOR);
								VecMat roiList = extractROI(img);
								for (int n = 0; n < roiList.size(); ++n) {
									Mat imgGray;
									cvtColor(roiList[n], imgGray, CV_RGB2GRAY);
									resize(imgGray, imgGray, Size(RESIZED_SIZE, RESIZED_SIZE));

									Mat hogDesc = pyramidHOG(imgGray, PYRAMID, 0, BLOCK_SIZE, BLOCK_STRIDE, CELL_SIZE, NUM_BINS);
									if (hogDesc.cols != DIMENSION) {
										cout << "The dimension of HOG descriptor is wrong!" << endl;
										exit(1);
									}
									
									stringstream ss;
									ss << "insert into " << DB_METADATA;
									ss << " (book_id,webtoon_id,page_id,slave_id) values (" << m << "," << *itFol << "," << pageId << ",0);";
									executeQueryNoResult(ss.str(), connection, &mysql);

									ss.clear();
									ss.str(string());
									ss << "insert into " << DB_DESCRIPTOR << " (id,";
									stringstream ssVal;
									ssVal << " values (" << mysql_insert_id(connection) << ",";
									for (int i = 0; i < DIMENSION; ++i) {
										if (i == DIMENSION - 1) {
											ss << "dim" << i + 1 << ")";
											ssVal << hogDesc.at<float>(0, i) << ")";
										}
										else {
											ss << "dim" << i + 1 << ",";
											ssVal << hogDesc.at<float>(0, i) << ",";
										}
									}
									ss << ssVal.str();
									executeQueryNoResult(ss.str(), connection, &mysql);
								}
								cout << roiList.size() << " ROIs processed!" << endl;
							}
							catch (exception e) {
								cout << "An error occurred. See " << logFile << " for details." << endl;
								logThis(logFile, e.what());
								continue;
							}
						}
						cout << "Processing time: " << ((double)(clock() - processTime)) / CLOCKS_PER_SEC << "s" << endl;
						++count;
						if (DB_LIMIT > 0 && count >= DB_LIMIT) {
							cout << "The data limit of " << DB_LIMIT << " reached. The program will now be terminated" << endl;
							cout << "Time elapsed: " << (((double)(clock() - start)) / CLOCKS_PER_SEC) / 3600. << " h" << endl;
							exit(0);
						}
					}
				}
				cout << "Total volumes processed: " << count << endl;
			} },
			{ "2", []{ 
				ofstream logfile;
				string logFile = "error.log";
				logfile.open(logFile, ios::trunc);
				logfile.close();

				ofstream logfile2;
				string logFile2;
				cout << "Misidentification result log file: ";
				getline(cin, logFile2);
				logfile2.open(logFile2, ios::trunc);
				logfile2.close();

				string testingDir;
				cout << "Query image folder: ";
				getline(cin, testingDir);
				VecString dirList = dir(testingDir);

				string modeChoice;
				cout << "Binary mode? ";
				getline(cin, modeChoice);

				bool binaryMode = modeChoice.compare("y") == 0 || modeChoice.compare("yes") == 0 ? true : false;

				string dbfile;
				if (binaryMode)
					cout << "Binary mode was chosen. ";
				cout << "Database file: ";
				getline(cin, dbfile);

				string metricChoice;
				cout << "Choose a metric: " << endl;
				cout << "\t[1] Euclidean" << endl;
				cout << "\t[2] Manhattan" << endl;
				getline(cin, metricChoice);

				cout << "Preparing database..." << endl;
				ifstream myDB(dbfile, ios::in | ios::binary);
				myDB.seekg(0, ios::beg);
				VecInt wIDList, bIDList, pIDList;
				Mat descriptor;
				while (!myDB.eof()) {
					int wID, slaveID, bID, pID;
					Mat desc = Mat::zeros(1, DIMENSION, CV_32F);

					myDB.read((char*)&bID, sizeof(int));
					myDB.read((char*)&wID, sizeof(int));
					myDB.read((char*)&pID, sizeof(int));
					myDB.read((char*)&slaveID, sizeof(int));

					for (int i = 0; i < DIMENSION; ++i) {
						myDB.read((char*)&desc.at<float>(0, i), sizeof(float));
					}
					bIDList.push_back(bID);
					wIDList.push_back(wID);
					pIDList.push_back(pID);
					descriptor.push_back(desc);
				}
				myDB.close();
				bIDList.pop_back();
				wIDList.pop_back();
				pIDList.pop_back();
				descriptor.pop_back();

				int share = dirList.size() / NUM_THREADS;
				vector<future<VecInt>> futures(NUM_THREADS);
				for (unsigned int i = 0; i < NUM_THREADS; ++i) {
					VecString::const_iterator first = dirList.begin() + i*share;
					VecString::const_iterator last = i == NUM_THREADS - 1 ? dirList.end() : dirList.begin() + (i + 1)*share;
					VecString job(first, last);
					futures[i] = async(launch::async, [=]() -> VecInt {
						return identify(job, make_tuple(bIDList, wIDList, pIDList, descriptor), testingDir, binaryMode, 
							atoi(metricChoice.c_str()), logFile, logFile2);
					});
				}
				int correct = 0;
				int numImages = 0;
				for (int i = 0; i != NUM_THREADS; ++i) {
					VecInt ret = futures[i].get();
					correct += ret[0];
					numImages += ret[1];
				}

				cout << "Overall accuracy: " << (float)correct / numImages * 100 << "%" << endl;
				cout << "Testing finished." << endl;
			} },
			{ "3", []{ 
				string logFile;
				cout << "Input log file: ";
				getline(cin, logFile);

				string outLog;
				cout << "Output fingerprint file: ";
				getline(cin, outLog);

				string descBinFile;
				cout << "Database binary file: ";
				getline(cin, descBinFile);

				string orgImgFol;
				cout << "Original image folder: ";
				getline(cin, orgImgFol);

				VecString fileList = dir(orgImgFol);

				try{
					ifstream file(logFile);
					if (!file) {
						cout << "Cannot open input file." << endl;
						exit(-1);
					}

					cout << "Preparing database..." << endl;
					ifstream myDB(descBinFile, ios::in | ios::binary);
					myDB.seekg(0, ios::beg);
					VecInt webtoonIDList;
					Mat descriptor;
					while (!myDB.eof()) {
						int wtID, slaveID;
						Mat desc = Mat::zeros(1, DIMENSION, CV_32F);

						myDB.read((char*)&wtID, sizeof(wtID));
						myDB.read((char*)&slaveID, sizeof(slaveID));

						for (int i = 0; i < DIMENSION; ++i) {
							myDB.read((char*)&desc.at<float>(0, i), sizeof(float));
						}
						webtoonIDList.push_back(wtID);
						descriptor.push_back(desc);
					}
					myDB.close();
					webtoonIDList.pop_back();
					descriptor.pop_back();

					string imageDir;
					Mat matLog;
					int k = 0;
					while (getline(file, imageDir)) {
						cout << "Processing " << imageDir << endl;
						Mat img, imgGray;
						img = imread(imageDir.c_str(), CV_LOAD_IMAGE_COLOR);
						VecMat roiList = extractROI(img);
						if (roiList.size() == 0)
							continue;
						cout << roiList.size() << " distorted ROIs extracted." << endl;

						ostringstream fss;
						fss << orgImgFol << "\\" << fileList[k];
						string fullPath = fss.str();
						cout << "Processing " << fullPath << endl;
						img = imread(fullPath.c_str(), CV_LOAD_IMAGE_COLOR);
						VecMat oriRoiList = extractROI(img);
						cout << oriRoiList.size() << " original ROIs extracted." << endl;
						if (oriRoiList.size() != roiList.size()) {
							k++;
							continue;
						}

						int j = 0;
						for (VecMat::iterator iter = roiList.begin(); iter != roiList.end(); iter++, j++) {
							cvtColor(*iter, imgGray, CV_RGB2GRAY);
							resize(imgGray, imgGray, Size(RESIZED_SIZE, RESIZED_SIZE));
							Mat hogDesc = pyramidHOG(imgGray, 3, 0, BLOCK_SIZE, BLOCK_STRIDE, CELL_SIZE, NUM_BINS);

							cvtColor(oriRoiList[j], imgGray, CV_RGB2GRAY);
							resize(imgGray, imgGray, Size(RESIZED_SIZE, RESIZED_SIZE));
							Mat oriHogDesc = pyramidHOG(imgGray, 3, 0, BLOCK_SIZE, BLOCK_STRIDE, CELL_SIZE, NUM_BINS);

							Mat hogDescRep;
							repeat(hogDesc, descriptor.rows, 1, hogDescRep);
							Mat dist;
							pow(hogDescRep - descriptor, 2, dist);
							reduce(dist, dist, 1, CV_REDUCE_SUM);
							Point min_loc;
							minMaxLoc(dist, 0, 0, &min_loc, 0);
							matLog.push_back(hogDesc);
							matLog.push_back(descriptor.row(min_loc.y));
							matLog.push_back(oriHogDesc);
						}
						k++;
					}
					writeMatToFile(matLog, outLog);
				}
				catch (exception e) {
					cout << e.what();
					exit(-1);
				}
			} },
			{ "4", []{
				cout << "Output database file: ";
				string file;
				getline(cin, file);
				ofstream dbFile(file, ios::out | ios::binary | ios::trunc);

				cout << "Connect to the Webtoon database... ";
				MYSQL *connection = NULL, mysql;
				connection = connectTo(&mysql, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, (char*)NULL, 0);
				cout << "Connected!" << endl << endl;

				ofstream logfile;
				string logFile = "dbexport.log";
				logfile.open(logFile, ios::trunc);
				logfile.close();

				MYSQL_RES* res;
				MYSQL_ROW row;
				stringstream ss;

				cout << "Exporting database..." << endl;
				ss << "select book_id, webtoon_id, page_id, slave_id,";
				for (int i = 0; i < DIMENSION; ++i) {
					if (i == DIMENSION - 1)
						ss << "dim" << i + 1;
					else
						ss << "dim" << i + 1 << ",";
				}
				ss << " from " << DB_METADATA << "," << DB_DESCRIPTOR << " where " << DB_DESCRIPTOR << ".id=" << DB_METADATA << ".id";
				string query = ss.str();
				res = executeQueryGetResult(query, connection, &mysql);

				while (row = mysql_fetch_row(res)) {
					int bID = atoi(row[0]);
					int wtID = atoi(row[1]);
					int pID = atoi(row[2]);
					int slaveID = atoi(row[3]);
					dbFile.write((char*)&bID, sizeof(int));
					dbFile.write((char*)&wtID, sizeof(int));
					dbFile.write((char*)&pID, sizeof(int));
					dbFile.write((char*)&slaveID, sizeof(int));
					for (int i = 0; i < DIMENSION; ++i) {
						float desc = (float)atof(row[i + 4]);
						dbFile.write((char*)&desc, sizeof(float));
					}
				}
				dbFile.close();
				cout << "Succesfully exported" << endl;
			} },
			{ "5", []{
				string str;
				cout << "Image folder: ";
				getline(cin, str);

				string outStr;
				cout << "Output folder: ";
				getline(cin, outStr);

				string separateFolChoice;
				cout << "Separate folder for each image? ";
				getline(cin, separateFolChoice);

				bool separateFol = separateFolChoice.compare("y") == 0 || separateFolChoice.compare("yes") == 0 ? true : false;
				VecString fileList = dir(str);
				for (int j = 0; j < fileList.size(); ++j) {
					string imageName = strtok(fileList[j], ".");
					if (separateFol) {
						stringstream cmd;
						cmd << "md \"" << outStr << "/" << imageName << "\"";
						string cmdStr = cmd.str();
						system(cmdStr.c_str());
					}

					stringstream ss;
					ss << str << "/" << fileList[j];
					cout << "Processing " << ss.str() << endl;
					Mat img = imread(ss.str(), CV_LOAD_IMAGE_COLOR);
					VecMat roiList = extractROI(img);
					for (int i = 0; i < roiList.size(); ++i) {
						stringstream filename;
						if (separateFol)
							filename << outStr << "/" << imageName << "/" << i + 1 << ".jpg";
						else 
							filename << outStr << "/" << imageName << "_" << i + 1 << ".jpg";
						imwrite(filename.str(), roiList[i]);
					}
					cout << "\t" << roiList.size() << " ROIs are saved" << endl;
				}
				cout << "ROI extraction completed" << endl;
			} },
			{ "6", []{
				string str;
				cout << "Image folder: ";
				getline(cin, str);

				string outLog;
				cout << "Output log file: ";
				getline(cin, outLog);

				VecString fileList = dir(str);
				Mat matLog;
				for (int j = 0; j < fileList.size(); j++) {
					ostringstream fss;
					fss << str << "\\" << fileList[j];
					string fullPath = fss.str();
					Mat img, imgGray;
					img = imread(fullPath.c_str(), CV_LOAD_IMAGE_COLOR);
					VecMat roiList = extractROI(img);
					for (int i = 0; i < roiList.size(); i++){
						cvtColor(roiList[i], imgGray, CV_RGB2GRAY);
						resize(imgGray, imgGray, Size(RESIZED_SIZE, RESIZED_SIZE));
						matLog.push_back(pyramidHOG(imgGray, 3, 0, BLOCK_SIZE, BLOCK_STRIDE, CELL_SIZE, NUM_BINS));
					}
				}
				writeMatToFile(matLog, outLog);
			} },
			{ "7", []{
				ofstream logfile;
				string logFile = "error.log";
				logfile.open(logFile, ios::trunc);
				logfile.close();

				ofstream logfile2;
				string logFile2;
				cout << "Misidentification result log file: ";
				getline(cin, logFile2);
				logfile2.open(logFile2, ios::trunc);
				logfile2.close();

				string testingDir;
				cout << "Query ROI folder: ";
				getline(cin, testingDir);
				VecString dirList = dir(testingDir);

				string modeChoice;
				cout << "Binary mode? ";
				getline(cin, modeChoice);

				bool binaryMode = modeChoice.compare("y") == 0 || modeChoice.compare("yes") == 0 ? true : false;

				string dbfile;
				if (binaryMode)
					cout << "Binary mode was chosen. ";
				cout << "Database file: ";
				getline(cin, dbfile);

				string metricChoice;
				cout << "Choose a metric: " << endl;
				cout << "\t[1] Euclidean" << endl;
				cout << "\t[2] Manhattan" << endl;
				getline(cin, metricChoice);

				cout << "Preparing database..." << endl;
				ifstream myDB(dbfile, ios::in | ios::binary);
				myDB.seekg(0, ios::beg);
				VecInt wIDList, bIDList, pIDList;
				Mat descriptor;
				while (!myDB.eof()) {
					int wID, slaveID, bID, pID;
					Mat desc = Mat::zeros(1, DIMENSION, CV_32F);

					myDB.read((char*)&bID, sizeof(int));
					myDB.read((char*)&wID, sizeof(int));
					myDB.read((char*)&pID, sizeof(int));
					myDB.read((char*)&slaveID, sizeof(int));

					for (int i = 0; i < DIMENSION; ++i) {
						myDB.read((char*)&desc.at<float>(0, i), sizeof(float));
					}
					bIDList.push_back(bID);
					wIDList.push_back(wID);
					pIDList.push_back(pID);
					descriptor.push_back(desc);
				}
				myDB.close();
				bIDList.pop_back();
				wIDList.pop_back();
				pIDList.pop_back();
				descriptor.pop_back();

				VecString roiList = dir(testingDir);
				int correct = 0;
				int numProcessed = 0;
				clock_t startTime = clock();
				for (VecString::iterator it = roiList.begin(); it != roiList.end(); ++it) {
					try {
						stringstream ss;
						ss << testingDir << "/" << *it;
						cout << "Processing " << ss.str() << endl;
						string trueWebtoonIDStr = strtok(*it, "_");
						int trueWebtoonID = atoi(trueWebtoonIDStr.c_str());

						Mat img = imread(ss.str()), imgGray;
						cvtColor(img, imgGray, CV_RGB2GRAY);
						resize(imgGray, imgGray, Size(RESIZED_SIZE, RESIZED_SIZE));

						Mat hogDesc = pyramidHOG(imgGray, PYRAMID, 0, BLOCK_SIZE, BLOCK_STRIDE, CELL_SIZE, NUM_BINS);
						if (hogDesc.cols != DIMENSION) {
							cout << "The dimension of HOG descriptor is wrong!" << endl;
							exit(1);
						}
						hogDesc = binaryMode ? binarizeDescriptors(hogDesc, NUM_BINS, NUM_BINS / 2) : hogDesc;
						Mat dist = norm(descriptor, hogDesc, atoi(metricChoice.c_str()));
						Point min_loc;
						minMaxLoc(dist, 0, 0, &min_loc, 0);

						if (wIDList[min_loc.y] == trueWebtoonID) {
							correct++;
							cout << "CORRECT" << endl;
						}
						else {
							stringstream sss;
							sss << ss.str() << "," << bIDList[min_loc.y] << "," << wIDList[min_loc.y] << "," << pIDList[min_loc.y];
							logThis(logFile2, sss.str());
							cout << "FALSE" << endl;
						}
						numProcessed++;
					}
					catch (exception e) {
						cout << "An error occurred. See " << logFile << " for details." << endl;
						logThis(logFile, e.what());
						continue;
					}
					cout << "Accuracy: " << (float)correct / numProcessed *100. << endl;
					cout << "Elapsed time: " << (float)(clock() - startTime) / CLOCKS_PER_SEC << "s \n" << endl;
				}
			} },
			{ "8", []{
				ofstream logfile;
				string logFile = "error.log";
				logfile.open(logFile, ios::trunc);
				logfile.close();

				string dbfile;
				cout << "Database file to be binarized: ";
				getline(cin, dbfile);

				string outputDbFile;
				cout << "Output binary file: ";
				getline(cin, outputDbFile);

				cout << "Preparing database..." << endl;
				ifstream myDB(dbfile, ios::in | ios::binary);
				myDB.seekg(0, ios::beg);
				VecInt wIDList, bIDList, pIDList, slaveIDList;
				Mat descriptor;
				while (!myDB.eof()) {
					int wID, slaveID, bID, pID;
					Mat desc = Mat::zeros(1, DIMENSION, CV_32F);

					myDB.read((char*)&bID, sizeof(int));
					myDB.read((char*)&wID, sizeof(int));
					myDB.read((char*)&pID, sizeof(int));
					myDB.read((char*)&slaveID, sizeof(int));

					for (int i = 0; i < DIMENSION; ++i)
						myDB.read((char*)&desc.at<float>(0, i), sizeof(float));

					bIDList.push_back(bID);
					wIDList.push_back(wID);
					pIDList.push_back(pID);
					slaveIDList.push_back(slaveID);
					descriptor.push_back(desc);
				}
				myDB.close();
				descriptor.pop_back();

				Mat binaryDesc = binarizeDescriptors(descriptor, 2 * NUM_BINS*BLOCK_SIZE / CELL_SIZE, 16);
				ofstream dbFile(outputDbFile, ios::out | ios::binary | ios::trunc);
				for (int i = 0; i < binaryDesc.rows; i++) {
					dbFile.write((char*)&bIDList[i], sizeof(int));
					dbFile.write((char*)&wIDList[i], sizeof(int));
					dbFile.write((char*)&pIDList[i], sizeof(int));
					dbFile.write((char*)&slaveIDList[i], sizeof(int));
					for (int j = 0; j < binaryDesc.cols; j++)
						dbFile.write((char*)&binaryDesc.at<float>(i, j), sizeof(float));
				}
				dbFile.close();
			} },
			{ "9", []{
				ofstream logfile;
				string logFile = "error.log";
				logfile.open(logFile, ios::trunc);
				logfile.close();

				string dbFile;
				cout << "Database file to print: ";
				getline(cin, dbFile);

				cout << "Preparing database..." << endl;
				ifstream myDB(dbFile, ios::in | ios::binary);
				myDB.seekg(0, ios::beg);
				while (!myDB.eof()) {
					int wID, slaveID, bID, pID;
					Mat desc = Mat::zeros(1, DIMENSION, CV_32F);

					myDB.read((char*)&bID, sizeof(int));
					myDB.read((char*)&wID, sizeof(int));
					myDB.read((char*)&pID, sizeof(int));
					myDB.read((char*)&slaveID, sizeof(int));

					for (int i = 0; i < DIMENSION; ++i)
						myDB.read((char*)&desc.at<float>(0, i), sizeof(float));

					cout << wID << ", " << bID << ", " << pID << endl;
					cout << desc << endl;
					cout << "\n";
					system("PAUSE");
				}
				myDB.close();
			} }
		}, [](const string a, const string b){ return a.compare(b) < 0; });
	}
	cout << "Time elapsed: " << (((double)(clock() - startTime)) / CLOCKS_PER_SEC) / 3600. << " h" << endl;
	return 0;
}

template <typename T, typename U>
inline bool greaterEqual(T i, U j) {
	return i > j;
}

template <typename T>
inline T findMostFrequent(vector<T> vec) {
	T freqVal = *(vec.begin());
	int freq = 0;
	int highestFreq = 0;
	sort(vec.begin(), vec.end(), greaterEqual<T, T>);
	T prevVal = *(vec.begin());
	auto lambda = [&](T elem) mutable {
		if (elem == prevVal)
			freq++;
		else
			freq = 0;
		if (freq > highestFreq) {
			highestFreq = freq;
			freqVal = elem;
		}
		prevVal = elem;
	};
	for_each(vec.begin() + 1, vec.end(), lambda);
	return freqVal;
}

template <typename KeyType, typename FunPtrType, typename Comp>
void Switch(const KeyType &value, initializer_list<pair<const KeyType, FunPtrType>> sws, Comp comp) {
	typedef pair<const KeyType &, FunPtrType> KVT;
	auto cmp = [&comp](const KVT &a, const KVT &b){ return comp(a.first, b.first); };
	auto val = KVT(value, FunPtrType());
	auto r = lower_bound(sws.begin(), sws.end(), val, cmp);
	if ((r != sws.end()) && (!cmp(val, *r))) {
		r->second();
	}
	else {
		cout << "Not a valid option. The program is terminated." << endl;
		exit(-1);
	}
}

template <typename... T>
VecInt identify(VecString queryList, tuple<T...> database, string testingDir, bool mode, int metric, string logFile, string logFile2) {
	m.lock();
	cout << "Thread " << this_thread::get_id() << " started" << endl;
	m.unlock();

	int correct = 0;
	int idx = 0;
	VecInt bIDList = get<0>(database);
	VecInt wIDList = get<1>(database);
	VecInt pIDList = get<2>(database);
	Mat descriptor = get<3>(database);
	for (VecString::iterator it = queryList.begin(); it != queryList.end(); ++it) {
		ostringstream fss;
		fss << testingDir << "\\" << *it;
		string fullPath = fss.str();
		string token = strtok(*it, "_");
		int trueWebtoonID = atoi(token.c_str());

		auto t = time(nullptr);
		auto tm = *localtime(&t);

		clock_t processTime = clock();
		try {
			Mat img, imgGray;
			img = imread(fullPath.c_str(), CV_LOAD_IMAGE_COLOR);
			VecMat roiList = extractROI(img);
			if (roiList.size() == 0)
				continue;

			VecInt prediction;
			VecInt minIndices;
			for (VecMat::iterator iter = roiList.begin(); iter != roiList.end(); ++iter) {
				cvtColor(*iter, imgGray, CV_RGB2GRAY);
				resize(imgGray, imgGray, Size(RESIZED_SIZE, RESIZED_SIZE));

				Mat hogDesc = pyramidHOG(imgGray, PYRAMID, 0, BLOCK_SIZE, BLOCK_STRIDE, CELL_SIZE, NUM_BINS);
				if (hogDesc.cols != DIMENSION) {
					m.lock();
					cout << "The dimension of HOG descriptor is wrong!" << endl;
					m.unlock();
					exit(1);
				}
				hogDesc = mode ? binarizeDescriptors(hogDesc, 2 * NUM_BINS*BLOCK_SIZE / CELL_SIZE, 16) : hogDesc;
				m.lock();
				Mat dist = norm(descriptor, hogDesc, metric);
				m.unlock();
				Point min_loc;
				minMaxLoc(dist, 0, 0, &min_loc, 0);
				minIndices.push_back(min_loc.y);
				prediction.push_back(wIDList[min_loc.y]);
			}
			int pred = findMostFrequent(prediction);
			idx++;
			m.lock();
			cout << "[" << put_time(&tm, "%d-%m-%Y %H-%M-%S") << "] From thread " << this_thread::get_id() << endl;
			cout << "Processing " << fullPath << endl;
			if (pred == trueWebtoonID) {
				correct++;
				cout << "CORRECT" << endl;
			}
			else {
				int index = findIndex(prediction, pred);
				if (index < 0) {
					cout << "There is some problem with indexing" << endl;
					exit(-1);
				}
				stringstream ss;
				ss << fullPath << "," << bIDList[minIndices[index]] << "," << pred << "," << pIDList[minIndices[index]];
				logThis(logFile2, ss.str());
				cout << "FALSE" << endl;
			}
			cout << "True Webtoon ID: " << trueWebtoonID << ". Predicted Webtoon ID: " << pred << endl;
			cout << "Accuracy: " << (float)correct / idx * 100 << "%" << endl;
			cout << "Processing time: " << ((double)(clock() - processTime)) / CLOCKS_PER_SEC <<
				". Processing time per ROI : " <<
				(((double)(clock() - processTime)) / CLOCKS_PER_SEC) / roiList.size() << "\n" << endl;
			m.unlock();
		}
		catch (exception e) {
			cout << "An error occurred. See " << logFile << " for details." << endl;
			logThis(logFile, e.what());
			continue;
		}
	}
	const int res[] = { correct, idx };
	VecInt v(res, res + sizeof(res) / sizeof(res[0]));

	m.lock();
	cout << "Thread " << this_thread::get_id() << " finished" << endl;
	m.unlock();
	return v;
}

template<typename T>
int findIndex(vector<T> vec, T val) {
	for (vector<T>::iterator it = vec.begin(); it != vec.end();++it)
		if (*it == val)
			return it - vec.begin();
	return -1;
}