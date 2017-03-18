#include "WaterWorld.h"
#include "pretrained.brc"
#include <plugin/bz2/bz2.h>

WaterWorld::WaterWorld() {
	Title("WaterWorld: Deep Q Learning");
	Icon(WaterWorldImg::icon());
	Sizeable().MaximizeBox().MinimizeBox();
	
	world.agents[0].world = this;
	
	running = false;
	stopped = true;
	ticking_running = false;
	ticking_stopped = true;
	simspeed = 2;
	
	t =		"{\n"
			"\t\"update\":\"qlearn\",\n"
			"\t\"gamma\":0.9,\n"
			"\t\"epsilon\":0.2,\n"
			"\t\"alpha\":0.005,\n"
			"\t\"experience_add_every\":5,\n"
			"\t\"experience_size\":10000,\n"
			"\t\"learning_steps_per_iteration\":5,\n"
			"\t\"tderror_clamp\":1.0,\n"
			"\t\"num_hidden_units\":100,\n"
			"}\n";
	
	
	agent_edit.SetData(t);
	agent_ctrl.Add(agent_edit.HSizePos().VSizePos(0,30));
	agent_ctrl.Add(reload_btn.HSizePos().BottomPos(0,30));
	reload_btn.SetLabel("Reload Agent");
	reload_btn <<= THISBACK(Reload);
	
	statusctrl.Add(status.HSizePos().VSizePos(0,30));
	statusctrl.Add(load_pretrained.HSizePos().BottomPos(0,30));
	load_pretrained.SetLabel("Load a Pretrained Agent");
	load_pretrained <<= THISBACK(LoadPretrained);
	
	Add(btnsplit.HSizePos().TopPos(0,30));
	Add(world.HSizePos().VSizePos(30,30));
	Add(lbl_eps.LeftPos(4,200-4).BottomPos(0,30));
	Add(eps.HSizePos(200,0).BottomPos(0,30));
	eps <<= THISBACK(RefreshEpsilon);
	eps.MinMax(0, +100);
	eps.SetData(50);
	lbl_eps.SetLabel("Exploration epsilon: ");
	btnsplit.Horz();
	btnsplit << reset << goveryfast << gofast << gonorm << goslow;
	goveryfast.SetLabel("Go Very Fast");
	gofast.SetLabel("Go Fast");
	gonorm.SetLabel("Go Normal");
	goslow.SetLabel("Go Slow");
	reset.SetLabel("Reset");
	goveryfast	<<= THISBACK1(SetSpeed, 3);
	gofast	<<= THISBACK1(SetSpeed, 2);
	gonorm	<<= THISBACK1(SetSpeed, 1);
	goslow	<<= THISBACK1(SetSpeed, 0);
	reset	<<= THISBACK2(Reset, true, true);
	
	SetSpeed(1);
	
	PostCallback(THISBACK2(Reset, true, true));
	PostCallback(THISBACK(Reload));
	PostCallback(THISBACK(Start));
	RefreshEpsilon();
}

WaterWorld::~WaterWorld() {
	ticking_running = false;
	while (!ticking_stopped) Sleep(100);
}

void WaterWorld::Tick() {
	// "Normal" is a little bit slower than fastest
	if      (simspeed == 2) Sleep(1);
	if      (simspeed == 1) Sleep(10);
	else if (simspeed == 0) Sleep(100);
	world.Tick();
}

void WaterWorld::Ticking() {
	while (ticking_running) {
		ticking_lock.Enter();
		Tick();
		ticking_lock.Leave();
	}
	ticking_stopped = true;
}

void WaterWorld::DockInit() {
	DockLeft(Dockable(agent_ctrl, "Edit Agent").SizeHint(Size(320, 240)));
	DockLeft(Dockable(statusctrl, "Status").SizeHint(Size(320, 240)));
	DockLeft(Dockable(reward, "Reward graph").SizeHint(Size(320, 240)));
}

void WaterWorld::Refresher() {
	world.Refresh();
	RefreshStatus();
	
	if (running) PostCallback(THISBACK(Refresher));
}

void WaterWorld::Start() {
	running = true;
	stopped = false;
	PostCallback(THISBACK(Refresher));
	
	ASSERT(!ticking_running);
	ticking_running = true;
	ticking_stopped = false;
	Thread::Start(THISBACK(Ticking));
}

void WaterWorld::Reset(bool init_reward, bool start) {
	WaterWorldAgent& agent = world.agents[0];
	
	if (init_reward) {
		int states = 152; // count of eyes
		int action_count = 4;
		
		agent.Init(1, states, 1, action_count);
		agent.Reset();
	}
	else {
		// Just reset values
		agent.ResetValues();
	}
}

void WaterWorld::Reload() {
	String param_str = agent_edit.GetData();
	WaterWorldAgent& agent = world.agents[0];
	
	ticking_lock.Enter();
	agent.LoadInitJSON(param_str);
	ticking_lock.Leave();
}

void WaterWorld::RefreshEpsilon() {
	double d = (double)eps.GetData() / 100.0;
	WaterWorldAgent& agent = world.agents[0];
	agent.SetEpsilon(d);
	lbl_eps.SetLabel("Exploration epsilon: " + FormatDoubleFix(d, 2));
}

void WaterWorld::SetSpeed(int i) {
	simspeed = i;
}

void WaterWorld::LoadPretrained() {
	WaterWorldAgent& agent = world.agents[0];
	
	// This is the pre-trained network from original ConvNetJS
	MemReadStream pretrained_mem(pretrained, pretrained_length);
	String json = BZ2Decompress(pretrained_mem);
	
	agent.do_training = false;
	agent.LoadJSON(json);
}

void WaterWorld::RefreshStatus() {
	WaterWorldAgent& agent = world.agents[0];
	
	String s;
	s << "Experience write pointer: " << agent.GetExperienceWritePointer() << "\n";
	s << "Latest TD error: " << FormatDoubleFix(agent.GetTDError(), 3);
	status.SetLabel(s);
}
