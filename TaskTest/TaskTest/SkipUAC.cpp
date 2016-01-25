#include "stdafx.h"
#include "SkipUAC.h"



SkipUAC::SkipUAC()
{
	
	bNeedClose = FALSE;
	m_pRootFolder = NULL;
	m_pService = NULL;
}


SkipUAC::~SkipUAC()
{
	Uninstall();
}
BOOL SkipUAC::StartUp()
{
	if (IsUserAnAdmin() == FALSE)
	{
		FristRun();
	}
	else
	{
		if (IsSkipUACTaskExist() == FALSE)
			SetSkipUACTask();
	}
	return NeedClose();
}
int SkipUAC::Init(CString TaskName)
{
	m_TaskName = TaskName;
	//  Initialize COM.
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		return 1;
	}
	hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);

	if (FAILED(hr))
	{
		CoUninitialize();
		return 1;
	}

	hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&m_pService);
	if (FAILED(hr))
	{
		printf("Failed to create an instance of ITaskService: %x", hr);
		CoUninitialize();
		return 1;
	}
	hr = m_pService->Connect(_variant_t(), _variant_t(),
		_variant_t(), _variant_t());
	if (FAILED(hr))
	{
		printf("ITaskService::Connect failed: %x", hr);
		m_pService->Release();
		CoUninitialize();
		return 1;
	}

	m_pRootFolder = NULL;
	hr = m_pService->GetFolder(_bstr_t(L"\\"), &m_pRootFolder);
	if (FAILED(hr))
	{
		printf("Cannot get Root Folder pointer: %x", hr);
		m_pService->Release();
		CoUninitialize();
		return 1;
	}

	return 0;
}

int SkipUAC::Uninstall()
{
	if (m_pRootFolder)
		m_pRootFolder->Release();
	if (m_pService)
		m_pService->Release();
	CoUninitialize();
	return 1;
}

BOOL SkipUAC::IsSkipUACTaskExist()
{
	HRESULT hr;
	LPCWSTR wszTaskName = m_TaskName;
	IRegisteredTask *pTask;
	hr = m_pRootFolder->GetTask(_bstr_t(wszTaskName), &pTask);
	if (SUCCEEDED(hr))
	{
		pTask->Release();		
		return TRUE;
	}
	return FALSE;
}

int SkipUAC::SetSkipUACTask()
{
	HRESULT hr ;

	LPCWSTR wszTaskName = m_TaskName;

	//hr = pRootFolder->DeleteTask(_bstr_t(wszTaskName), 0);

	//  Create the task builder object to create the task.
	ITaskDefinition *pTask = NULL;
	hr = m_pService->NewTask(0, &pTask);

	if (FAILED(hr))
	{
		return 1;
	}

	//  ------------------------------------------------------
	//  Get the registration info for setting the identification.
	IRegistrationInfo *pRegInfo = NULL;
	hr = pTask->get_RegistrationInfo(&pRegInfo);
	if (FAILED(hr))
	{
		pTask->Release();
		return 1;
	}

	hr = pRegInfo->put_Author(L"SnailSoft");
	pRegInfo->Release();
	if (FAILED(hr))
	{
		pTask->Release();
		return 1;
	}

	//  ------------------------------------------------------
	//  Create the principal for the task
	IPrincipal *pPrincipal = NULL;
	hr = pTask->get_Principal(&pPrincipal);
	if (FAILED(hr))
	{
		pTask->Release();
		return 1;
	}

	//  Set up principal information: 
	hr = pPrincipal->put_Id(_bstr_t(L"Principal1"));
	if (FAILED(hr))
		printf("\nCannot put the principal ID: %x", hr);

	hr = pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
	if (FAILED(hr))
		printf("\nCannot put principal logon type: %x", hr);

	//  Run the task with the least privileges (LUA) 
	hr = pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
	pPrincipal->Release();
	if (FAILED(hr))
	{
		printf("\nCannot put principal run level: %x", hr);		
		pTask->Release();
		return 1;
	}


	//  ------------------------------------------------------
	//  Create the settings for the task
	ITaskSettings *pSettings = NULL;
	hr = pTask->get_Settings(&pSettings);
	if (FAILED(hr))
	{
		printf("\nCannot get settings pointer: %x", hr);
		pTask->Release();
		return 1;
	}

	//  Set setting values for the task.
	hr = pSettings->put_StartWhenAvailable(VARIANT_TRUE);
	//pSettings->Release();
	if (FAILED(hr))
	{
		printf("\nCannot put setting info: %x", hr);
		pSettings->Release();
		pTask->Release();
		return 1;
	}

	hr = pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);

	if (FAILED(hr))
	{
		pSettings->Release();
		pTask->Release();
		return 1;
	}
	hr = pSettings->put_StartWhenAvailable(VARIANT_FALSE);
	if (FAILED(hr))
	{
		pSettings->Release();
		pTask->Release();	
		return 1;
	}


	hr = pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
	if (FAILED(hr))
	{
		pSettings->Release();
		pTask->Release();
		return 1;
	}

	IIdleSettings *pIdleSettings = NULL;
	hr = pSettings->get_IdleSettings(&pIdleSettings);
	if (FAILED(hr))
	{
		pSettings->Release();
		pTask->Release();
		return 1;
	}
	hr = pIdleSettings->put_StopOnIdleEnd(VARIANT_FALSE);
	pIdleSettings->Release();
	pSettings->Release();
	if (FAILED(hr))
	{
		
		pTask->Release();
		return 1;
	}

	//  ------------------------------------------------------
	//  Add an Action to the task. This task will execute notepad.exe.     
	IActionCollection *pActionCollection = NULL;

	//  Get the task action collection pointer.
	hr = pTask->get_Actions(&pActionCollection);
	if (FAILED(hr))
	{
		pTask->Release();
		return 1;
	}

	//  Create the action, specifying that it is an executable action.
	IAction *pAction = NULL;
	hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
	pActionCollection->Release();
	if (FAILED(hr))
	{
		pTask->Release();
		return 1;
	}

	IExecAction *pExecAction = NULL;

	//  QI for the executable task pointer.
	hr = pAction->QueryInterface(
		IID_IExecAction, (void**)&pExecAction);
	pAction->Release();
	if (FAILED(hr))
	{
		pTask->Release();
		return 1;
	}

	//  Set the path of the executable to notepad.exe.
	TCHAR path[MAX_PATH];
	ZeroMemory(path, MAX_PATH * 2);
	GetModuleFileName(NULL, path, MAX_PATH);
	CString strPath = path;
	BSTR bstr = strPath.AllocSysString();
	hr = pExecAction->put_Path(bstr);
	pExecAction->Release();
	SysFreeString(bstr);
	if (FAILED(hr))
	{
		pTask->Release();
		return 1;
	}


	//  ------------------------------------------------------
	//  Save the task in the root folder.
	IRegisteredTask *pRegisteredTask = NULL;
	hr = m_pRootFolder->RegisterTaskDefinition(
		_bstr_t(wszTaskName),
		pTask,
		TASK_CREATE_OR_UPDATE,
		_variant_t(),
		_variant_t(),
		TASK_LOGON_INTERACTIVE_TOKEN,
		_variant_t(L""),
		&pRegisteredTask);
	if (FAILED(hr))
	{
		pTask->Release();
		return 2;
	}


	//  Clean up.
	pTask->Release();
	pRegisteredTask->Release();
	return 0;
}

int SkipUAC::RunSkipUACTask()
{
	HRESULT hr;
	LPCWSTR wszTaskName = m_TaskName;
	IRegisteredTask *pTask;
	hr = m_pRootFolder->GetTask(_bstr_t(wszTaskName), &pTask);
	if (SUCCEEDED(hr))
	{
		if (IsUserAnAdmin() == FALSE)
		{
			VARIANT var;
			VariantInit(&var);
			IRunningTask   *pRunningTask;
			hr = pTask->Run(var, &pRunningTask);
			if (SUCCEEDED(hr))
			{
				pRunningTask->Release();
				return 0;
			}			
		}
		pTask->Release();
	}
	return 1;
}

BOOL SkipUAC::FristRun()
{
	if (IsSkipUACTaskExist())
	{
		if (RunSkipUACTask() == 0)
			bNeedClose = TRUE;
		else
		{
			RunAs();
			bNeedClose = TRUE;
		}
	}
	else
	{
		RunAs();
		bNeedClose = TRUE;
		return TRUE;
	}
}

BOOL SkipUAC::NeedClose()
{
	return bNeedClose;
}

void SkipUAC::RunAs()
{
	SHELLEXECUTEINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SEE_MASK_NOCLOSEPROCESS;    /////////代表需要返回进程Handle
	si.lpVerb = _T("runas");

	TCHAR buf[MAX_PATH];
	GetModuleFileName(NULL, buf, MAX_PATH);
	CString strFilename = buf;
	CString Directory = strFilename;
	CString Path;
	si.lpFile = strFilename;
	si.lpDirectory = Directory;
	si.nShow = SW_SHOW;
	si.lpParameters = Path;
	ShellExecuteEx(&si);
}