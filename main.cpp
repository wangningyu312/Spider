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

//����URL������������������Դ��  
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

//ʹ��Get���󣬵õ���Ӧ  
bool GetHttpResponse(const string & strUrl, char * &response, int &bytesRead)
{
	string strHost;       ///����
	string strResource;   ///��Դ
	if (!ParseURL(strUrl, strHost, strResource))
	{
		cout << "Can not parse the url" << endl;
		return false;
	}

	//����socket  
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

	//������������ַ  
	SOCKADDR_IN sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(80);
	memcpy(&sa.sin_addr, hp->h_addr, 4);

	//��������  
	if (0 != connect(sock, (SOCKADDR*)&sa, sizeof(sa)))
	{
		cout << "Can not connect: " << strUrl << endl;
		closesocket(sock);
		return false;
	};

	//׼����������  
	string request = "GET " + strResource + " HTTP/1.1\r\nHost:" + strHost + "\r\nConnection:Close\r\n\r\n";

	//��������  
	if (SOCKET_ERROR == send(sock, request.c_str(), request.size(), 0))
	{
		cout << "send error" << endl;
		closesocket(sock);
		return false;
	}

	//��������  
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
			pageBuf = (char*)realloc(pageBuf, m_nContentLength);       //���·����ڴ�  
		}
		cout << ret << " ";
	}
	cout << endl;

	pageBuf[bytesRead] = '\0';
	response = pageBuf;
	closesocket(sock);
	return true;
}

//��ȡ���е�URL�Լ�ͼƬURL  
void HTMLParse(string & htmlResponse, vector<string> & imgurls, const string & host) 
{
	//���������ӣ�����queue��  
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
			//char url[100]; //�̶���С�Ļᷢ�������������Σ��  
			sscanf_s(pos, "%[^\"]", url);
			string surl = url;  // ת����string���ͣ������Զ��ͷ��ڴ�  
			if (visitedUrl.find(surl) == visitedUrl.end()) 
			{
				visitedUrl.insert(surl);
				ofile << surl << endl;
				hrefUrl.push(surl);
			}
			pos = strstr(pos, tag);
			delete[] url;  // �ͷŵ�������ڴ�  
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
	//���������ӣ�����queue��

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

//��URLת��Ϊ�ļ���  
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

//����ͼƬ��img�ļ���  
void DownLoadImg(vector<string>& vecImgUrl, const string& strUrl) 
{
	//���ɱ����url��ͼƬ���ļ���  
	string strFoldname = "./img/" + ToFileName(strUrl, true);
	if (!CreateDirectory(strFoldname.c_str(), NULL))
	{
		cout << "Can not create directory:" << strFoldname << endl;
	}
		

	char *image;
	int byteRead;
	for (int i = 0; i<vecImgUrl.size(); i++)
	{
		//�ж��Ƿ�ΪͼƬ��bmp��jgp��jpeg��gif   
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
		//�������е�����  
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



//��ȱ���  
void BFS(const string& strUrl) 
{
	char * response;
	int bytes;
	// ��ȡ��ҳ����Ӧ������response�С�  
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
		// �������ҳ���ı�����  
		ofile << httpResponse << endl;
		ofile.close();
	}
	vector<string> imgurls;
	//��������ҳ������ͼƬ���ӣ�����imgurls����  
	HTMLParseV2(httpResponse, imgurls, strUrl);

	//�������е�ͼƬ��Դ  
	DownLoadImg(imgurls, strUrl);
}

int main()
{
	//��ʼ��socket������tcp��������  
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
	{
		return 0;
	}

	//// �����ļ��У�����ͼƬ����ҳ�ı��ļ�  
	CreateDirectory("./img", 0);
	CreateDirectory("./html", 0);

	// ��������ʼ��ַ  

	string urlStart = "http://jandan.net/ooxx";

	//// ʹ�ù�ȱ���  
	//// ��ȡ��ҳ�еĳ����ӷ���hrefUrl�У���ȡͼƬ���ӣ�����ͼƬ��  
	BFS(urlStart);

	////// ���ʹ�����ַ��������  
	visitedUrl.insert(urlStart);

	while (hrefUrl.size() != 0) 
	{
		string url = hrefUrl.front();  // �Ӷ��е��ʼȡ��һ����ַ  
		cout << url << endl;
		BFS(url);                   // ������ȡ�������Ǹ���ҳ����������ĳ�������ҳ����hrefUrl��������������ı���ͼƬ  
		hrefUrl.pop();                 // ������֮��ɾ�������ַ  
	}

	WSACleanup();

	system("pause");
}
