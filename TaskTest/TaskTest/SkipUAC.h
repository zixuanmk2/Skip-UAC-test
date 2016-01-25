#pragma once

#include <comdef.h>
#include <wincred.h>
//  Include the task header file.
#include <taskschd.h>
# pragma comment(lib, "taskschd.lib")
# pragma comment(lib, "comsupp.lib")
# pragma comment(lib, "credui.lib")


class SkipUAC
{
public:
	SkipUAC();
	~SkipUAC();
	int Init(CString TaskName);
	int Uninstall();
	BOOL StartUp();
	void RunAs();
private:

	BOOL IsSkipUACTaskExist();

	int SetSkipUACTask();

	int RunSkipUACTask();

	BOOL FristRun();

	BOOL NeedClose();

	ITaskFolder *m_pRootFolder;
	ITaskService *m_pService;
	CString m_TaskName;
	BOOL bNeedClose;
};

