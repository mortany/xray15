[!output TEMPLATESTRING_COPYRIGHT]

#include "[!output PROJECT_NAME].h"

[!if QT_UI != 0]
#include "ui_plugin_form.h"
#include "qmessagebox.h"
#include "qobject.h"
[!endif]

#define [!output CLASS_NAME]_CLASS_ID Class_ID([!output CLASSID1], [!output CLASSID2])

class [!output CLASS_NAME] : public [!output SUPER_CLASS_NAME][!if QT_UI != 0], public QObject [!endif]

{
public:
	// Constructor/Destructor
	[!output CLASS_NAME]();
	virtual ~[!output CLASS_NAME]();

	virtual void DeleteThis() override {}
	
	virtual void BeginEditParams(Interface *ip,IUtil *iu) override;
	virtual void EndEditParams(Interface *ip,IUtil *iu) override;

	virtual void Init(HWND hWnd);
	virtual void Destroy(HWND hWnd);
	
	// Singleton access
	static [!output CLASS_NAME]* GetInstance() { 
		static [!output CLASS_NAME] the[!output CLASS_NAME];
		return &the[!output CLASS_NAME]; 
	}

private:
[!if QT_UI != 0]
	void DoSomething();
	QWidget *widget;
	Ui::PluginRollup ui;
[!else]
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	HWND hPanel;
[!endif]
	IUtil* iu;
};

[!output TEMPLATESTRING_CLASSDESC]

[!if PARAM_MAPS != 0]
[!output TEMPLATESTRING_PARAMBLOCKDESC]
[!endif]

//--- [!output CLASS_NAME] -------------------------------------------------------
[!output CLASS_NAME]::[!output CLASS_NAME]()
	: iu(nullptr)
[!if QT_UI ==0]
	, hPanel(nullptr)
[!endif]
{

}

[!output CLASS_NAME]::~[!output CLASS_NAME]()
{

}

void [!output CLASS_NAME]::BeginEditParams(Interface* ip,IUtil* iu) 
{
	this->iu = iu;
[!if QT_UI != 0]
	widget = new QWidget;
	ui.setupUi(widget);

	// We can connect UI signals here using Qt Functor syntax
	QObject::connect(ui.pushButton, &QPushButton::clicked, this, &[!output CLASS_NAME]::DoSomething);
	ip->AddRollupPage(*widget, L"Plug-in Rollup");
[!else]
	hPanel = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_PANEL),
		DlgProc,
		GetString(IDS_PARAMS),
		0);
[!endif]
}

void [!output CLASS_NAME]::EndEditParams(Interface* ip,IUtil*)
{
	this->iu = nullptr;
[!if QT_UI != 0]
	ip->DeleteRollupPage(*widget);
[!else]
	ip->DeleteRollupPage(hPanel);
	hPanel = nullptr;
[!endif]
}

void [!output CLASS_NAME]::Init(HWND /*handle*/)
{

}

void [!output CLASS_NAME]::Destroy(HWND /*handle*/)
{

}

[!if QT_UI != 0]
void [!output CLASS_NAME]::DoSomething()
{
	int spin_value = ui.spinBox->value();
	QMessageBox::information(widget, "Dialog", QString("Spinner value: %1").arg(spin_value));
}
[!else]
INT_PTR CALLBACK [!output CLASS_NAME]::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) 
	{
		case WM_INITDIALOG:
			[!output CLASS_NAME]::GetInstance()->Init(hWnd);
			break;

		case WM_DESTROY:
			[!output CLASS_NAME]::GetInstance()->Destroy(hWnd);
			break;

		case WM_COMMAND:
			#pragma message(TODO("React to the user interface commands.  A utility plug-in is controlled by the user from here."))
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			GetCOREInterface()->RollupMouseMessage(hWnd,msg,wParam,lParam);
			break;

		default:
			return 0;
	}
	return 1;
}
[!endif]
