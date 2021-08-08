#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "advapi32.lib")
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <Windows.h>


WCHAR servicePath[1024] = { 0 };
LPCWSTR serviceName = L"MyService3";

SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE serviceStatusHandle;

int addLogMessage(const char* str) {
	errno_t err;
	FILE* log;
	if ((err = fopen_s(&log, "C:\\BSIT_lab3\\log.txt", "a+")) != 0) {
		return -1;
	}
	fprintf(log, "%s", str);
	fclose(log);
	return 0;
}


int InstallService() {
	if (GetModuleFileName(0, servicePath, sizeof(servicePath) / sizeof(servicePath[0])) > 0)
	{
		SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
		if (!hSCManager) {
			addLogMessage("Error: Can't open Service Control Manager\n");
			return -1;
		}

		SC_HANDLE hService = CreateService(
			hSCManager,
			serviceName,
			serviceName,
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_OWN_PROCESS,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_NORMAL,
			servicePath,
			NULL, NULL, NULL, NULL, NULL
		);
		if (!hService) {
			int err = GetLastError();
			switch (err) {
			case ERROR_ACCESS_DENIED:
				addLogMessage("Error: ERROR_ACCESS_DENIED\n");
				break;
			case ERROR_CIRCULAR_DEPENDENCY:
				addLogMessage("Error: ERROR_CIRCULAR_DEPENDENCY\n");
				break;
			case ERROR_DUPLICATE_SERVICE_NAME:
				addLogMessage("Error: ERROR_DUPLICATE_SERVICE_NAME\n");
				break;
			case ERROR_INVALID_HANDLE:
				addLogMessage("Error: ERROR_INVALID_HANDLE\n");
				break;
			case ERROR_INVALID_NAME:
				addLogMessage("Error: ERROR_INVALID_NAME\n");
				break;
			case ERROR_INVALID_PARAMETER:
				addLogMessage("Error: ERROR_INVALID_PARAMETER\n");
				break;
			case ERROR_INVALID_SERVICE_ACCOUNT:
				addLogMessage("Error: ERROR_INVALID_SERVICE_ACCOUNT\n");
				break;
			case ERROR_SERVICE_EXISTS:
				addLogMessage("Error: ERROR_SERVICE_EXISTS\n");
				break;
			default:
				addLogMessage("Error: Undefined\n");
			}
			CloseServiceHandle(hSCManager);
			return -1;
		}
		CloseServiceHandle(hService);

		CloseServiceHandle(hSCManager);
		addLogMessage("Success install service!\n");
	}
	else
		addLogMessage("Oops\n");
	return 0;
}

int RemoveService() {
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager) {
		addLogMessage("Error: Can't open Service Control Manager\n");
		return -1;
	}
	SC_HANDLE hService = OpenService(hSCManager, serviceName, SERVICE_STOP | DELETE);
	if (!hService) {
		addLogMessage("Error: Can't remove service\n");
		CloseServiceHandle(hSCManager);
		return -1;
	}

	DeleteService(hService);
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	addLogMessage("Success remove service!\n");
	return 0;
}

int StartService() {
	SC_HANDLE hSCManager = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
	if (!hSCManager)
	{
		addLogMessage("Error: Can't open Service Control Manager\n");
		return -1;
	}

	SC_HANDLE hService = OpenService(hSCManager, serviceName, SERVICE_START);
	if (!hService) {
		addLogMessage("Error: Can't open Service\n");
		CloseServiceHandle(hSCManager);
		return -1;
	}
	if (!StartService(hService, NULL, NULL)) {
		CloseServiceHandle(hSCManager);
		addLogMessage("Error: Can't start service\n");
		return -1;
	}

	addLogMessage("Success start service!\n");
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	return 0;
}

int StopService()
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	SC_HANDLE hService = OpenService(hSCManager, serviceName, SERVICE_QUERY_STATUS | SERVICE_STOP);

	if (QueryServiceStatus(hService, &serviceStatus))
		if (serviceStatus.dwCurrentState == SERVICE_RUNNING)
			ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus);
		else
		{
			addLogMessage("Error: Can't stop service\n");
			return -1;
		}

	addLogMessage("Success stop service!\n");
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	return 0;
}


void ControlHandler(DWORD request) {
	switch (request)
	{
	case SERVICE_CONTROL_STOP:
		addLogMessage("Stopped.\n");
		serviceStatus.dwWin32ExitCode = 0;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		return;
	case SERVICE_CONTROL_SHUTDOWN:
		addLogMessage("Shutdown.\n");
		serviceStatus.dwWin32ExitCode = 0;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		return;

	default:
		break;
	}
	SetServiceStatus(serviceStatusHandle, &serviceStatus);
	return;
}

void ServiceMain(int argc, char** argv) {
	int error;
	int i = 0;
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
		SERVICE_ACCEPT_SHUTDOWN;
	serviceStatus.dwWin32ExitCode = 0;
	serviceStatus.dwServiceSpecificExitCode = 0;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;
	serviceStatusHandle = RegisterServiceCtrlHandler(serviceName,
		(LPHANDLER_FUNCTION)ControlHandler);
	if (serviceStatusHandle == (SERVICE_STATUS_HANDLE)0) {
		return;
	}


	serviceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(serviceStatusHandle, &serviceStatus);


	FILE* config;
	while (serviceStatus.dwCurrentState == SERVICE_RUNNING)
	{
		if ((error = fopen_s(&config, "C:\\BSIT_lab3\\config.txt", "r")) != 0) {
			serviceStatus.dwCurrentState = SERVICE_STOPPED;
			serviceStatus.dwWin32ExitCode = -1;
			SetServiceStatus(serviceStatusHandle, &serviceStatus);
			return;
		}
		char folder_path[150] = { 0 }, zip_path[150] = { 0 }, zip_command[1000] = { 0 }, all_mask[600] = { 0 };

		fgets(folder_path, 150, config);
		folder_path[strlen(folder_path)-1] = 0;

		fgets(zip_path, 150, config);
		zip_path[strlen(zip_path)-1] = 0;

		do
		{
			char mask[100] = { 0 };
			strcat(mask, "-ir!");
			strcat(mask, zip_path);
			fgets(mask+strlen(mask), 50, config);
			mask[strlen(mask) - 1] = 0;
			strcat(all_mask, mask);
			strcat(all_mask, " ");
		} while (!feof(config));

		sprintf(zip_command, "\"C://Program Files/7-Zip/7z.exe\" a -tzip -ssw -mx=1 %s %s", folder_path, all_mask);// -spf2 -ir!*.txt
		addLogMessage("\ncommand:");
		addLogMessage(all_mask);
		addLogMessage("\n");
		system(zip_command);
		Sleep(10000);
	}
	return;
}

int _tmain(int argc, _TCHAR* argv[]) {
	
	if (argc - 1 == 0) {
		SERVICE_TABLE_ENTRY ServiceTable[] =
		{ { (LPWSTR)serviceName,                        //имя сервиса
			(LPSERVICE_MAIN_FUNCTION)ServiceMain }, //главная функция сервиса
			{ NULL, NULL }
		};
		if (!StartServiceCtrlDispatcher(ServiceTable))
		{
			addLogMessage("Error: StartServiceCtrlDispatcher");
		}
	}
	else if (wcscmp(argv[argc - 1], _T("install")) == 0) {
		InstallService();
	}
	else if (wcscmp(argv[argc - 1], _T("remove")) == 0) {
		RemoveService();
	}
	else if (wcscmp(argv[argc - 1], _T("start")) == 0) {
		StartService();
	}
	else if (wcscmp(argv[argc - 1], _T("stop")) == 0) {
		StopService();
	}
}

