#pragma once
#include "stdhdrs.h"
#include "vncsetauth.h"

#define DIALOG_MAKEINTRESOURCE MAKEINTRESOURCE
class vncServer;
class RulesListView;

class PropertiesDialog
{
private:
	HWND hTabControl, hTabAuthentication, hTabIncoming, hTabInput, hTabMisc, hTabNotifications,
		hTabReverse, hTabRules, hTabCapture, hTabLog, hTabAdministration, hTabService;
	BOOL		m_dlgvisible;
	BOOL bConnectSock = true;
	int ListPlugins(HWND hComboBox);
	RulesListView *rulesListView;
	vncServer* m_server = NULL;
	vncSetAuth	m_vncauth;	
	void onTabsOK(HWND hwnd);
	void onTabsAPPLY(HWND hwnd);
	void InitPortSettings(HWND hwnd);

	
	void ShowImpersonateDialog();	
	bool showAdminPanel = false;
	bool standalone = false;
	void setServiceStatusText(HWND hwnd);

public:
	PropertiesDialog();
	~PropertiesDialog();

	HWND PropertiesDialogHwnd = NULL;
	BOOL Init(vncServer* server);
	int ShowDialog(bool standalone = false);
	void UpdateServer();
	int HandleNotify(HWND hwndDlg, WPARAM wParam, LPARAM lParam);
	bool InitDialog(HWND hwnd);
	void onOK(HWND hwnd);
	void onApply(HWND hwnd);
	void onCancel(HWND hwnd);
	bool DlgInitDialog(HWND hwnd);
	bool onCommand(int command, HWND hwnd, int subcommand);

	static void Secure_Plugin(char* szPlugin);
	static void LogToEdit(const std::string& message);
	static char buffer[65536];
	static HWND hEditLog;
};

