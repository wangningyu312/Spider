#include "CWebSpider.h"

CWebSpider::CWebSpider()
{
	m_strLogFilePath = "LogFile.txt";
	m_strURLFilePath = "URLFile.text";
}

CWebSpider::~CWebSpider()
{
	
}

void CWebSpider::RunSpider(const string& strUrl)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// 创建文件夹，保存图片和网页文本文件  
	CreateDirectory("./img", 0);
	CreateDirectory("./html", 0);

	m_VisitingUrl.push(strUrl);

	while (m_VisitingUrl.size() > 0)
	{
		string strCurUrl = m_VisitingUrl.front();
		m_VisitingUrl.pop();

		BFS_Url(strCurUrl);
	}

	WSACleanup();
}

void CWebSpider::TestDownLoad(const string& strUrl)
{
	vector<string> vecImgUrl;

	vecImgUrl.push_back(strUrl);
	vecImgUrl.push_back(strUrl);
	vecImgUrl.push_back(strUrl);

	DownLoadImg(vecImgUrl, strUrl);
}

//解析URL，解析出主机名，资源名  
bool CWebSpider::ParseURL(const string & strUrl, string & strHost, string & strResource)
{
	bool bRet = false;
	if (strUrl.size() > 2000)
	{
		return bRet;
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
		return bRet;
	}

	regex regexUrl("/");
	smatch smatchUrl;

	bool bFind = regex_search(strUrlValue, smatchUrl, regexUrl);
	if (bFind)
	{
		strHost = smatchUrl.prefix().str();
		strResource = strSeparator + smatchUrl.suffix().str();
	}

	bRet = true;

	return bRet;
}

bool CWebSpider::GetHttpResponse(const string & strUrl, string& strResponse)
{
	bool bRet = false;

	m_FileOpen.open(m_strLogFilePath, ios::app);

	string strHost;       ///主机
	string strResource;   ///资源
	if (!ParseURL(strUrl, strHost, strResource))
	{
		m_FileOpen << "Can not parse the url" << endl;
		m_FileOpen.close();

		return bRet;
	}

	//建立socket  
	struct hostent* pHost= gethostbyname(strHost.c_str());
	if (pHost == NULL)
	{
		m_FileOpen << "Can not find host address" << endl;
		m_FileOpen.close();

		return bRet;
	}

	SOCKET socketValue = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketValue == -1 || socketValue == -2)
	{
		m_FileOpen << "Can not create sock." << endl;
		m_FileOpen.close();

		return bRet;
	}

	//建立服务器地址  
	SOCKADDR_IN socketAddr;
	socketAddr.sin_family = AF_INET;
	socketAddr.sin_port = htons(80);
	memcpy(&socketAddr.sin_addr, pHost->h_addr, 4);

	//建立连接  
	if (0 != connect(socketValue, (SOCKADDR*)&socketAddr, sizeof(socketAddr)))
	{
		m_FileOpen << "Can not connect: " << strUrl << endl;
		m_FileOpen.close();

		closesocket(socketValue);

		return bRet;
	};

	//准备发送数据  
	string strRequest = "GET " + strResource + " HTTP/1.1\r\nHost:" + strHost + "\r\nConnection:Close\r\n\r\n";

	//发送数据  
	if (SOCKET_ERROR == send(socketValue, strRequest.c_str(), strRequest.size(), 0))
	{
		m_FileOpen << "send error" << endl;
		m_FileOpen.close();

		closesocket(socketValue);

		return false;
	}

	//接收数据  
	int iContentLength = DEFAULT_PAGE_BUF_SIZE;
	char* pageBuf = (char *)malloc(iContentLength);
	memset(pageBuf, 0, iContentLength);

	int iByteRead = 0;
	int iRet = 1;
	m_FileOpen << "Read: ";
	while (iRet > 0)
	{
		iRet = recv(socketValue, pageBuf + iByteRead, iContentLength - iByteRead, 0);

		if (iRet > 0)
		{
			iByteRead += iRet;
		}

		if (iContentLength - iByteRead<100)
		{
			m_FileOpen << "\nRealloc memorry" << endl;
			iContentLength *= 2;
			pageBuf = (char*)realloc(pageBuf, iContentLength);       //重新分配内存  
		}
		m_FileOpen << iRet << " ";
	}
	m_FileOpen << endl;

	pageBuf[iByteRead] = '\0';

	strResponse = pageBuf;
	free(pageBuf);

	closesocket(socketValue);
	m_FileOpen.close();
	
	bRet = true;

	return bRet;
}

void CWebSpider::HTMLParse(const string & strHTMLValue, vector<string>& vecImgUrl)
{
	//找所有连接，加入queue中

	//WriteHTMLResponce(htmlResponse);

	m_FileOpen.open(m_strURLFilePath, ios::app);

	regex regexHtml("(href=\")([^ ]*)(\")");
	sregex_iterator itHtml(strHTMLValue.begin(), strHTMLValue.end(), regexHtml);
	sregex_iterator endHtml;

	for (; itHtml != endHtml; ++itHtml)
	{
		string strNexURL = itHtml->str(2);
		if (strNexURL.size() < 10)
		{
			continue;
		}
		if (   strNexURL.find("tag") != string::npos
			|| strNexURL.find("javascript") != string::npos)
		{
			continue;
		}
		if (m_setVisitedUrl.find(strNexURL) == m_setVisitedUrl.end())
		{
			m_setVisitedUrl.insert(strNexURL);
			m_VisitingUrl.push(strNexURL);

			m_FileOpen << strNexURL << endl;
		}
	}

	m_FileOpen << endl << endl;
	m_FileOpen.close();


	regex regexIMG("(img )(src=\"|lazy-src=\")([^ ]*)(\")");
	sregex_iterator itImg(strHTMLValue.begin(), strHTMLValue.end(), regexIMG);
	sregex_iterator endImg;

	for (; itImg != endImg; ++itImg)
	{
		string strImgUrl = itImg->str(3);

		if (m_setVisitedImg.find(strImgUrl) == m_setVisitedImg.end())
		{
			m_setVisitedImg.insert(strImgUrl);
			vecImgUrl.push_back(strImgUrl);
		}
	}
}

void CWebSpider::DownLoadImg(vector<string>& vecImgUrl, const string& strUrl)
{
	//生成保存该url下图片的文件夹  

	string strFoldname = "./img/" + ToFileName(strUrl, true);
	CreateDirectory(strFoldname.c_str(), NULL);


	string strHttpResponse;
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
		if (GetHttpResponse(strImg, strHttpResponse))
		{
			if (strHttpResponse.size() < 1) 
			{
				continue;
			}
			
			const char *p = strHttpResponse.c_str();
			int byteRead = strlen(p);
			const char * pos = strstr(p, "\r\n\r\n") + strlen("\r\n\r\n");
			int index = strImg.find_last_of("/");
			if (index != string::npos)
			{
				string imgname = strImg.substr(index, strImg.size());
				ofstream ofile(strFoldname + imgname, ios::binary);
				if (!ofile.is_open())
					continue;
				//ofile.write(pos, byteRead - (pos - p));
				ofile << strHttpResponse;
				ofile.close();
			}
		}
	}
}

void CWebSpider::BFS_Url(const string& strUrl)
{
	string strHttpResponse;
	// 获取网页的相应，放入response中。  
	if (!GetHttpResponse(strUrl, strHttpResponse))
	{
		return;
	}

	string filename = ToFileName(strUrl, false);
	ofstream ofile("./html/" + filename);
	if (ofile.is_open())
	{
		// 保存该网页的文本内容  
		ofile << strHttpResponse << endl;
		ofile.close();
	}

	vector<string> vecImgUrl;
	//解析该网页的所有图片链接，放入imgurls里面  
	HTMLParse(strHttpResponse, vecImgUrl);

	//下载所有的图片资源  
	DownLoadImg(vecImgUrl, strUrl);
}

string CWebSpider::ToFileName(const string & strUrl, bool bFolder)
{
	string strFileName;
	strFileName.resize(strUrl.size());
	int k = 0;
	for (int i = 0; i<(int)strUrl.size(); ++i)
	{
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