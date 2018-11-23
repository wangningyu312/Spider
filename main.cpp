// #include <Windows.h>  

#define _CRT_NONSTDC_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include"CWebSpider.h"
 
using namespace std;

int main()
{
	CWebSpider MySpider;
	//string strTest = "http://ww3.sinaimg.cn/mw600/0073tLPGgy1fxh4memhvnj30k00k0gox.jpg";
	//MySpider.TestDownLoad(strTest);

	string strUrl = "https://www.biqudu.com/43_43821/3013548.html";
	MySpider.RunSpider(strUrl);

	system("pause");
}
