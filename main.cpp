// #include <Windows.h>  

#define _CRT_NONSTDC_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <string>  
#include <iostream>  
#include <fstream>  
#include <vector>  
#include "winsock2.h"  
#include <time.h>  
#include <queue>  
#include <unordered_set>  
#include<regex>
#include"CWebSpider.h"

#pragma comment(lib, "ws2_32.lib")   
using namespace std;

#define DEFAULT_PAGE_BUF_SIZE 1048576  


queue<string> hrefUrl;
unordered_set<string> visitedUrl;
unordered_set<string> visitedImg;
int depth = 0;
int g_ImgCnt = 1;

void WriteHTMLResponce(const string& strHTMLValue)
{
	ofstream OpHTMLTxt("HTMLResponce.txt", ios::app);

	OpHTMLTxt << strHTMLValue << "\n";

	OpHTMLTxt.close();
}

//解析URL，解析出主机名，资源名  
bool ParseURL(const string & strUrl, string & strHost, string & strResource)
{
	if (strUrl.size() > 2000)
	{
		return false;
	}

	string strUrlHead = "http://";
	string::size_type iFindHead = strUrl.find(strUrlHead);

	string strUrlValue = strUrl;
	if (iFindHead != string::npos)
	{
		string::size_type iSubPos = strUrlHead.size();
		strUrlValue = strUrlValue.substr(iSubPos);
	}

	string strSeparator = "/";
	string::size_type iFindStor = strUrlValue.find(strSeparator);
	if (iFindStor == string::npos)
	{
		return false;
	}

	regex regexUrl("/");
	smatch smatchUrl;

	bool bFind = regex_search(strUrlValue, smatchUrl, regexUrl);
	if (bFind)
	{
		strHost = smatchUrl.prefix().str();
		strResource = strSeparator + smatchUrl.suffix().str();
	}
		
	return true;
}

//使用Get请求，得到响应  
bool GetHttpResponse(const string & strUrl, char * &response, int &bytesRead)
{
	string strHost;       ///主机
	string strResource;   ///资源
	if (!ParseURL(strUrl, strHost, strResource))
	{
		cout << "Can not parse the url" << endl;
		return false;
	}

	//建立socket  
	struct hostent * hp = gethostbyname(strHost.c_str());
	if (hp == NULL)
	{
		cout << "Can not find host address" << endl;
		return false;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == -1 || sock == -2) 
	{
		cout << "Can not create sock." << endl;
		return false;
	}

	//建立服务器地址  
	SOCKADDR_IN sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(80);
	memcpy(&sa.sin_addr, hp->h_addr, 4);

	//建立连接  
	if (0 != connect(sock, (SOCKADDR*)&sa, sizeof(sa)))
	{
		cout << "Can not connect: " << strUrl << endl;
		closesocket(sock);
		return false;
	};

	//准备发送数据  
	string request = "GET " + strResource + " HTTP/1.1\r\nHost:" + strHost + "\r\nConnection:Close\r\n\r\n";

	//发送数据  
	if (SOCKET_ERROR == send(sock, request.c_str(), request.size(), 0))
	{
		cout << "send error" << endl;
		closesocket(sock);
		return false;
	}

	//接收数据  
	int m_nContentLength = DEFAULT_PAGE_BUF_SIZE;
	char *pageBuf = (char *)malloc(m_nContentLength);
	memset(pageBuf, 0, m_nContentLength);

	bytesRead = 0;
	int ret = 1;
	cout << "Read: ";
	while (ret > 0) 
	{
		ret = recv(sock, pageBuf + bytesRead, m_nContentLength - bytesRead, 0);

		if (ret > 0)
		{
			bytesRead += ret;
		}

		if (m_nContentLength - bytesRead<100)
		{
			cout << "\nRealloc memorry" << endl;
			m_nContentLength *= 2;
			pageBuf = (char*)realloc(pageBuf, m_nContentLength);       //重新分配内存  
		}
		cout << ret << " ";
	}
	cout << endl;

	pageBuf[bytesRead] = '\0';
	response = pageBuf;
	closesocket(sock);
	return true;
}

//提取所有的URL以及图片URL  
void HTMLParse(string & htmlResponse, vector<string> & imgurls, const string & host) 
{
	//找所有连接，加入queue中  
	const char *p = htmlResponse.c_str();
	const char *tag = "href=\"";
	const char *pos = strstr(p, tag);
	ofstream ofile("url.txt", ios::app);
	while (pos)
	{
		pos += strlen(tag);
		const char * nextQ = strstr(pos, "\"");
		if (nextQ) {
			char * url = new char[nextQ - pos + 1];
			//char url[100]; //固定大小的会发生缓冲区溢出的危险  
			sscanf_s(pos, "%[^\"]", url);
			string surl = url;  // 转换成string类型，可以自动释放内存  
			if (visitedUrl.find(surl) == visitedUrl.end()) 
			{
				visitedUrl.insert(surl);
				ofile << surl << endl;
				hrefUrl.push(surl);
			}
			pos = strstr(pos, tag);
			delete[] url;  // 释放掉申请的内存  
		}
	}
	ofile << endl << endl;
	ofile.close();

	tag = "<img ";
	const char* att1 = "src=\"";
	const char* att2 = "lazy-src=\"";
	const char *pos0 = strstr(p, tag);
	while (pos0) {
		pos0 += strlen(tag);
		const char* pos2 = strstr(pos0, att2);
		if (!pos2 || pos2 > strstr(pos0, ">")) {
			pos = strstr(pos0, att1);
			if (!pos) {
				pos0 = strstr(att1, tag);
				continue;
			}
			else {
				pos = pos + strlen(att1);
			}
		}
		else {
			pos = pos2 + strlen(att2);
		}

		const char * nextQ = strstr(pos, "\"");
		if (nextQ) {
			char * url = new char[nextQ - pos + 1];
			sscanf_s(pos, "%[^\"]", url);
			cout << url << endl;
			string imgUrl = url;
			if (visitedImg.find(imgUrl) == visitedImg.end()) {
				visitedImg.insert(imgUrl);
				imgurls.push_back(imgUrl);
			}
			pos0 = strstr(pos0, tag);
			delete[] url;
		}
	}
	cout << "end of Parse this html" << endl;
}

void HTMLParseV2(string & htmlResponse, vector<string> & imgurls, const string & host)
{
	//找所有连接，加入queue中

	WriteHTMLResponce(htmlResponse);

	ofstream ofile("url.txt", ios::app);

	string strHTML = htmlResponse;
	regex regexHTML("(href=\")([^ ]*)(\")");
	sregex_iterator itHTML(strHTML.begin(), strHTML.end(), regexHTML);
	sregex_iterator endHTML;
	for ( ; itHTML!=endHTML; ++itHTML)
	{
		string strNexURL = itHTML->str(2);
		if (strNexURL.size() < 10)
		{
			continue;
		}
		if (strNexURL.find("tag") != string::npos
			|| strNexURL.find("javascript") != string::npos)
		{
			continue;
		}
		if (visitedUrl.find(strNexURL) == visitedUrl.end())
		{
			visitedUrl.insert(strNexURL);
			ofile << strNexURL << endl;
			hrefUrl.push(strNexURL);
		}
	}

	ofile << endl << endl;
	ofile.close();


	regex regexIMG("(img )(src=\"|lazy-src=\")([^ ]*)(\")");
	sregex_iterator itIMG(strHTML.begin(), strHTML.end(), regexIMG);
	sregex_iterator endIMG;
	for (; itIMG != endIMG; ++itIMG)
	{
		string strIMGURL = itIMG->str(3);

		if (visitedImg.find(strIMGURL) == visitedImg.end()) {
			visitedImg.insert(strIMGURL);
			imgurls.push_back(strIMGURL);
		}
	}
	
	cout << "end of Parse this html" << endl;
}

//把URL转化为文件名  
string ToFileName(const string & strUrl, bool bFolder = false)
{
	string strFileName;
	strFileName.resize(strUrl.size());
	int k = 0;
	for (int i = 0; i<(int)strUrl.size(); i++) {
		char ch = strUrl[i];
		if (ch != '\\'&&ch != '/'&&ch != ':'&&ch != '*'&&ch != '?'&&ch != '"'&&ch != '<'&&ch != '>'&&ch != '|')
			strFileName[k++] = ch;
	}
	
	strFileName = strFileName.substr(0, k);
	if (!bFolder)
	{
		strFileName = strFileName + ".txt";
	}

	return  strFileName;
}

//下载图片到img文件夹  
void DownLoadImg(vector<string>& vecImgUrl, const string& strUrl) 
{
	//生成保存该url下图片的文件夹  
	string strFoldname = "./img/" + ToFileName(strUrl, true);
	if (!CreateDirectory(strFoldname.c_str(), NULL))
	{
		cout << "Can not create directory:" << strFoldname << endl;
	}
		

	char *image;
	int byteRead;
	for (int i = 0; i<vecImgUrl.size(); i++)
	{
		//判断是否为图片，bmp，jgp，jpeg，gif   
		string strImg = vecImgUrl.at(i);
		int pos = strImg.find_last_of(".");
		if (pos == string::npos)
		{
			continue;
		}
		else 
		{
			string ext = strImg.substr(pos + 1, strImg.size() - pos - 1);
			if (ext != "bmp"&& ext != "jpg" && ext != "jpeg"&& ext != "gif"&&ext != "png")
				continue;
		}
		//下载其中的内容  
		if (GetHttpResponse(strImg, image, byteRead))
		{
			if (strlen(image) == 0) {
				continue;
			}
			const char *p = image;
			const char * pos = strstr(p, "\r\n\r\n") + strlen("\r\n\r\n");
			int index = strImg.find_last_of("/");
			if (index != string::npos) 
			{
				string imgname = strImg.substr(index, strImg.size());
				ofstream ofile(strFoldname + imgname, ios::binary);
				if (!ofile.is_open())
					continue;
				cout << g_ImgCnt++ << strFoldname + imgname << endl;
				ofile.write(pos, byteRead - (pos - p));
				ofile.close();
			}
			free(image);
		}
	}
}



//广度遍历  
void BFS(const string& strUrl) 
{
	char * response;
	int bytes;
	// 获取网页的相应，放入response中。  
	if (!GetHttpResponse(strUrl, response, bytes))
	{
		cout << "The url is wrong! ignore." << endl;
		return;
	}
	string httpResponse = response;
	free(response);
	string filename = ToFileName(strUrl);
	ofstream ofile("./html/" + filename);
	if (ofile.is_open())
	{
		// 保存该网页的文本内容  
		ofile << httpResponse << endl;
		ofile.close();
	}
	vector<string> imgurls;
	//解析该网页的所有图片链接，放入imgurls里面  
	HTMLParseV2(httpResponse, imgurls, strUrl);

	//下载所有的图片资源  
	DownLoadImg(imgurls, strUrl);
}

int main()
{
	//初始化socket，用于tcp网络连接  
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
	{
		return 0;
	}

	//// 创建文件夹，保存图片和网页文本文件  
	CreateDirectory("./img", 0);
	CreateDirectory("./html", 0);

	// 遍历的起始地址  

	string urlStart = "http://jandan.net/ooxx";

	//// 使用广度遍历  
	//// 提取网页中的超链接放入hrefUrl中，提取图片链接，下载图片。  
	BFS(urlStart);

	////// 访问过的网址保存起来  
	visitedUrl.insert(urlStart);

	while (hrefUrl.size() != 0) 
	{
		string url = hrefUrl.front();  // 从队列的最开始取出一个网址  
		cout << url << endl;
		BFS(url);                   // 遍历提取出来的那个网页，找它里面的超链接网页放入hrefUrl，下载它里面的文本，图片  
		hrefUrl.pop();                 // 遍历完之后，删除这个网址  
	}

	WSACleanup();

	system("pause");
}
