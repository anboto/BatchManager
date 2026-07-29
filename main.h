#ifndef _BatchManager_main_h_
#define _BatchManager_main_h_

#include <CtrlLib/CtrlLib.h>
#include <SysInfo/SysInfo.h>
#include <Controls4U/Controls4U.h>
#include <ScatterCtrl/ScatterCtrl.h>

using namespace Upp;


#define LAYOUTFILE <BatchManager/BatchManager.lay>
#include <CtrlCore/lay.h>

String FormatBytes(uint64 bytes);
uint64 ScanBytes(const String& s);

class Batch;

class Task : public WithTask<StaticRect> {
typedef Task CLASSNAME;
public:
	Task();
	virtual ~Task() {}
	
	void Start(String folder, String file, String args, double maxTime);
	void Perform(bool updateScatter, bool updateSeries);
	
	void OnStop();
	//void Stop();
	//void Pause();
	
	bool OnProcess(double elapsed, const String &out, bool isEnd, bool &resetTimeout);
	
	int id;
	uint64 memMax = 0, memTotal = 0, memRam = 0;
	int paging_s;
	
	LocalProcessX process;
	Time lastPausedStarted = Null;
	
	Vector<int> vtime;
	Vector<int> vtotal, vram;
	Vector<int> vpaging_s;
	
	void Load();
	Task &SetParent(Batch &_parent)	{parent = &_parent;	return *this;}
	
	void Jsonize(JsonIO& json);
	
private:
	friend class Batch;
	
	Batch *parent = nullptr;
	
	void UpdateScatters();
};

class Batch {
typedef Batch CLASSNAME;
public:
	virtual ~Batch() {}

	Task &Add(int id) {
		Task &task = tasks.Add();
		task.id = id;
		task.parent = this;
		return task;
	}
	void Remove(int id)   {tasks.Remove(GetIndex(id));}
	Task &GetId(int id)   {return tasks[GetIndex(id)];}
	Task &GetIdx(int idx) {return tasks[idx];}
	int size()			  {return tasks.size();}
	void Clear()		  {tasks.Clear();}
	
	int GetShown() {
		for (int i = 0; i < tasks.size(); ++i) {
			if (tasks[i].IsShown())
				return i;
		}
		return -1;
	}
	void Show(int id) {
		for (Task &task : tasks)
			task.Show(id == task.id);
	}
	bool IsRunning() {
		for (Task &task : tasks) {
			if (task.process.IsRunning()) 
				return true;
		}
		return false;
	}
	void Stop() {
		for (Task &task : tasks)
			if (task.process.IsRunning())
				task.process.Stop();
	}
	int MaxId() {
		int id = 0;
		for (Task &task : tasks)
			id = max(id, task.id);
		return id;
	}
	String GetConsole(int id);
	void SetConsole(int id, String str);
	
	void Jsonize(JsonIO& json);
		
private:
	Array<Task> tasks;	
	
	int GetIndex(int id);
};


class Main : public TopWindow {
typedef Main CLASSNAME;
public:
	Main();
	virtual ~Main();

	void Load();
	void Save();
	void Clear();
	
	void OnSel();
	void TimerFun();
	int RunCommand(const char *cmd);
	virtual void Close();
	void DoClose(bool prompt);
	void Jsonize(JsonIO& json);
	bool Init();
	void StartCallBack();
	void StopCallBack();
	void TaskCount(bool onlySelected, int &pending, int &paused, int &running, int &ended);
		
	SplitterButton splitter, splitterH;
	WithMain<StaticRect> main;
	WithOutput<StaticRect> output;
	
	void ClearRealized();
	
private:
	void OnDropInsert(int line, PasteClip& d);
	void OnDrag();
	virtual void DragAndDrop(Point p, PasteClip& d);
	bool Key(dword key, int count);
	void DoDrop(String name);	
	
	void OnCPUSpin();
	void OnRAMSpin();
	void OnReStart();
	void OnEnd();
	void OnDelete();
	void OnMaxTime();
	void OnMove(int delta, int row);
	void OnFolder();
	void OnSort();
	     
	void InitButtons();
	void UpdateLabs();
	
	//Vector<Value> DoneToPend(const Vector<Value> &linedone);
	
	Index<int> GetSelectedRows();
	Index<int> GetSelectedIds();
	void SetSelectedIds(const Index<int> &ids);
	
	int idId, idStatus, idFile, idArgs, idFolder, idUser, idComputer, idStart, idEnd, idMaxTime, idTime, idMemActual, idMemMax;
	
	Index<String> status;
	
	Batch batch;
		
	String configFile;
	
	int procid;
	
	TimeCallback timeCallback;
	Time lastScatter, lastSeries;
};

#endif
