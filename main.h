#ifndef _BatchManager_main_h_
#define _BatchManager_main_h_

#include <CtrlLib/CtrlLib.h>
#include <SysInfo/SysInfo.h>
#include <Controls4U/Controls4U.h>

using namespace Upp;

//#include <Candidates/CandidatesGui.h>

#define LAYOUTFILE <BatchManager/BatchManager.lay>
#include <CtrlCore/lay.h>

class IdsArray {
public:
	void Add(int id) {
		if (id+1 > ids.size())
			ids.SetCount(id+1, Null);
		ids[id] = num++;
	}
	int operator()(int id) {return ids[id];}
	
private:
	int num = 0;
	Vector<int> ids;
};

class Batch : public WithBatch<StaticRect> {
typedef Batch CLASSNAME;
public:
	Batch();
	virtual ~Batch() {}
	
	void OnStop();
	void Stop();
	void OnPause();
	void OnFolder();
	void OnLater();
	bool OnProcess(double elapsed, const String &out, bool isEnd, bool &resetTimeout);
	
	int batId;
	bool isStarted;
	bool isEnded;
	LocalProcessX process;
	uint64 sumMem = 0;
	int numMem = 0;
};

class Main : public TopWindow {
typedef Main CLASSNAME;
public:
	Main();
	virtual ~Main();

	void Load();
	void Save();
	
	void OnStop();
	void OnSel(int who);
	void TimerFun();
	int RunCommand(const char *cmd);
	virtual void Close();
	void DoClose(bool prompt);
	void Jsonize(JsonIO& json);
	bool Init();
	void Start();
	void Stop();
	
	SplitterButton splitter, splitterH, splitter1_2, splitter12_3;
	WithMain_left1<StaticRect> left1;
	WithMain_left2<StaticRect> left2;
	WithMain_left3<StaticRect> left3;
	WithMain_right<StaticRect> right;
	ConsoleText console;
	
	//void ClearPending();
	void ClearRealized();
	
	//bool alreadyRunning;
	
	//void PauseTimer() {timerPaused = true;}
	
private:
	void DnD(PasteClip& d);
	void DnDInsertProc(int line, PasteClip& d);
	void DragProc();
	void DnDInsertPend(int line, PasteClip& d);
	void DragPend();
	void DragAndDrop(Point p, PasteClip& d);
	void DragDone();
	bool Key(dword key, int count);
	void DoDrop(String name);	
	void OnCPUSpin();
	void OnPendingsSel();
	void OnPendingsDelete();
	void OnPendingsCPU();
	void OnPendingsTime();
	void OnMove(int delta, int row = -1);
	
	void InitButtons();
	void UpdateLabs();
	
	Vector<Value> DoneToPend(const Vector<Value> &linedone);
		
	IdsArray proc, pend, done;
	enum {idId, idFile, idArgs, idFolder, idUser, idComputer, idCPU, idBegin, idTime, idMem, 
			idStatus, idEnd, idDuration, idMemory};
	
	Array<Batch> batches;
	Vector<String> pendingToJsonize;
	
	String configFile;
	
	int procid;
	
	TimeCallback timeCallback;
	int calcMemCounter = 0;
	int selected = 0;
	//bool timerPaused = false;
};

#endif
