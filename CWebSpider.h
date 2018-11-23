
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma once

#include<string>
#include<set>
#include<queue>
#include<regex>
#include<fstream>
#include<chrono>

#include "winsock2.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define DEFAULT_PAGE_BUF_SIZE 1048576

class CWebSpider
{
public:
	CWebSpider();
	~CWebSpider();

	void RunSpider(const string& strUrl);
	void TestDownLoad(const string& strUrl);

private:
	///����URL��ȡ����������Դ��
	bool ParseURL(const string & strUrl, string & strHost, string & strResource);
	///ʹ��Get���󣬵õ���Ӧ  
	bool GetHttpResponse(const string & strUrl, string& strResponce);
	bool GetHttpResponse_Img(const string & strUrl, char* & pResponse, int & iBytesRead);
	///����HTML����ȡImgUrl
	void HTMLParse(const string & strHTMLValue, vector<string>& vecImgUrl);
	///����ͼƬ
	void DownLoadImg(vector<string>& vecImgUrl, const string& strUrl);
	///��ȱ���URL
	void BFS_Url(const string& strUrl);

	string ToFileName(const string& strUrl, bool bFolder);

private:
	string m_strLogFilePath;
	string m_strURLFilePath;
	ofstream m_FileOpen;

	queue<string> m_VisitingUrl;
	set<string> m_setVisitedUrl;
	set<string> m_setVisitedImg;
};

