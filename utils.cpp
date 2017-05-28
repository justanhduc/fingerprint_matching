#pragma once

#include <comdef.h>
#include <atlstr.h>
#include <fstream>

#include "utils.h"

mutex m;

void DisplayErrorBox(LPTSTR lpszFunction) {
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and clean up

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40)*sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

VecString dir(string directory, bool folder) {
	WIN32_FIND_DATA ffd;
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;
	TCHAR dir[MAX_PATH];
	VecString itemList;

	_tcscpy_s(dir, CA2T(directory.c_str()));
	StringCchLength(dir, MAX_PATH, &length_of_arg);

	if (length_of_arg > (MAX_PATH - 3))
	{
		_tprintf(TEXT("\nDirectory path is too long.\n"));
		exit(-1);
	}

	_tprintf(TEXT("\nTarget directory is %s\n\n"), dir);

	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.

	StringCchCopy(szDir, MAX_PATH, dir);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	// Find the first file in the directory.

	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		DisplayErrorBox(TEXT("FindFirstFile"));
		exit(dwError);
	}

	// List all the files in the directory with some info about them.
	do
	{
		wstring ws(ffd.cFileName);
		string str(ws.begin(), ws.end());
		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && folder) {
			if (str.compare(".") != 0 && str.compare("..") != 0)
				itemList.push_back(str);
		}
		else if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !folder) {
			if (str.compare(".") != 0 && str.compare("..") != 0)
				itemList.push_back(str);
		}
		else
			continue;
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
	{
		DisplayErrorBox(TEXT("FindFirstFile"));
	}
	FindClose(hFind);
	return itemList;
}

string strtok(string str, string delimiter) {
	size_t pos;
	string token;
	while ((pos = str.find(delimiter)) != string::npos) {
		token = str.substr(0, pos);
		break;
	}
	return token;
}

VecString strsplit(string str, string delimiter) {
	VecString strings;
	size_t pos;
	while ((pos = str.find(delimiter)) != string::npos) {
		strings.push_back(str.substr(0, pos));
		str = str.substr(pos + 1, string::npos);
	}
	strings.push_back(str.substr(pos + 1, string::npos));
	return strings;
}

MYSQL *connectTo(MYSQL* mysql, string host, string user, string password, string name, int port, char *socket, int flag) {
	MYSQL *connection;
	mysql_init(mysql);
	connection = mysql_real_connect(mysql, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, (char *)NULL, 0);

	if (connection == NULL)
	{
		fprintf(stderr, "Mysql connection error : %s", mysql_error(mysql));
		exit(1);
	}
	return connection;
}

void executeQueryNoResult(string query, MYSQL* connection, MYSQL *mysql) {
	const char* queryChar = query.c_str();
	int query_stat = mysql_query(connection, queryChar);
	if (query_stat != 0) {
		cout << "Mysql query error: " << mysql_error(mysql);
		exit(-1);
	}
}

MYSQL_RES *executeQueryGetResult(string query, MYSQL* connection, MYSQL *mysql) {
	MYSQL_RES* res;
	const char* queryChar = query.c_str();
	int query_stat = mysql_query(connection, queryChar);
	if (query_stat != 0) {
		cout << "Mysql query error: " << mysql_error(mysql);
		exit(-1);
	}
	res = mysql_store_result(connection);
	return res;
}

void logThis(string file, string log) {
	ofstream logfile;
	logfile.open(file, ios::app);
	logfile << log << "\n";
	logfile.close();
}

void writeMatToFile(Mat m, string file) {
	ofstream logfile;
	logfile.open(file, ios::trunc);
	for (int i = 0; i < m.rows; i++) {
		for (int j = 0; j < m.cols; j++) {
			if (j == m.cols - 1)
				logfile << m.at<float>(i, j) << "\n";
			else
				logfile << m.at<float>(i, j) << ",";
		}
	}
	logfile.close();
}

Mat norm(Mat mat1, Mat mat2, int metric) {
	assert(mat1.cols == mat2.cols && "The number of columns in the two matrices must be the same");
	Mat mat2Tmp;
	if (mat2.rows == 1)
		repeat(mat2, mat1.rows, 1, mat2Tmp);
	else if (mat2.rows == mat1.rows)
		mat2Tmp = mat2;
	else {
		cout << "Dimension mismatched. Too many rows in the second matrix." << endl;
		exit(-1);
	}
	Mat dist;

	switch (metric)
	{
	case 1:
		pow(mat1 - mat2Tmp, 2, dist); //Euclidean
		break;
	case 2:
		dist = abs(mat1 - mat2Tmp); //Manhattan
		break;
	default:
		cout << "Not a valid choice. Using Euclidean norm" << endl;
		pow(mat1 - mat2Tmp, 2, dist); //Euclidean
		break;
	}
	reduce(dist, dist, 1, CV_REDUCE_SUM);
	return dist;
}
