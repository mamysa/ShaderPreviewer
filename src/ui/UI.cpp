#include "Logger.h"
#include "imgui.h"
#include "UI.h"

static const ImVec4 ColorLogSuccess(0.039, 0.901, 0.047, 1.0);
static const ImVec4 ColorLogFailure(0.9, 0.05, 0.05, 1.0);
static const ImVec4 ColorLogInfo(0.9, 0.9, 0.9, 1.0);

static int ControlPanelHeight = 35;

// get font color for given log type 
static const ImVec4& colorForLogType(LogType type) {
	if (type == LogType::SUCCESS) return ColorLogSuccess; 
	if (type == LogType::FAILURE) return ColorLogFailure;
	if (type == LogType::INFO)    return ColorLogInfo; 
	return ColorLogInfo; 
}

// create window to which log output is written
static void makeLogWindow(int fbw, int fbh) {
	ImGui::Begin("Log", NULL, 0);
	//ImGui::SetWindowPos(ImVec2(0.0, 300.0));

	const std::list<LogEntry>& messages = Logger::getBuf();
	for (auto it = messages.begin(); it != messages.end(); it++) {
		ImGui::TextColored(colorForLogType(it->second), it->first.c_str());
	}

	ImGui::End();
}

static void makeControlWindow(int fbw, int fbh) {
	ImGui::Begin("Controls", NULL, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove);
	ImGui::SetWindowPos(ImVec2(0, fbh - ControlPanelHeight));
	ImGui::SetWindowSize(ImVec2(fbw, ControlPanelHeight));
	ImGui::SameLine();
	ImGui::Button("Log (F1)", ImVec2(120, 0));
	ImGui::SameLine();
	ImGui::Button("Textures (F2)", ImVec2(120, 0));
	ImGui::End();
}

void makeUI(int fbw, int fbh) {
	makeControlWindow(fbw, fbh);
	makeLogWindow(fbw, fbh);
}