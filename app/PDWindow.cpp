#include "PDWindow.h"
#include <future>
#include <mutex>
#include <functional>
#include <variant>
#include "core/stb_image.h"
#include <algorithm>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <unistd.h>

using namespace std;

namespace {
int kSocketPort = 8891;

struct DoubleFuzzingConfig {
  double from;
  double to;
  double step;
};

DoubleFuzzingConfig stub_config;

class FuzzingConfig {
 public:
  virtual ~FuzzingConfig() = default;
};

class NumericFuzzingConfig : public FuzzingConfig {


};

void DrawFuzzingConfig(DoubleFuzzingConfig* config, const char* unit) {
  constexpr float kInputWidget = 120.0f;

  ImGui::TextUnformatted("From");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(kInputWidget);
  ImGui::InputDouble("##from", &config->from);
  ImGui::SameLine(0.0, 1.0);
  ImGui::TextUnformatted(unit);
  ImGui::SameLine();
  ImGui::TextUnformatted("to");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(kInputWidget);
  ImGui::InputDouble("##to", &config->to);
  ImGui::SameLine(0.0, 1.0);
  ImGui::TextUnformatted(unit);
  ImGui::SameLine();
  ImGui::TextUnformatted("by");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(kInputWidget);
  ImGui::InputDouble("##step", &config->step);
  ImGui::SameLine(0.0, 1.0);
  ImGui::TextUnformatted(unit);
}

void DrawFuzzingTargetList() {
  if (ImGui::BeginCombo(u8"##fuzzing_target", u8"自车")) {

    ImGui::EndCombo();
  }
}

void DrawFuzzingFieldList() {
  if (ImGui::BeginCombo(u8"##fuzzing_field", u8"heading")) {

    ImGui::EndCombo();
  }
}

void DrawFuzzingPolicyList() {
  if (ImGui::BeginCombo(u8"##fuzzing_policy", u8"逻辑运算")) {

    ImGui::EndCombo();
  }
}

void DrawFuzzingConfigItem() {
  ImGui::TextUnformatted(u8"泛化对象:");
  ImGui::SameLine();
  DrawFuzzingTargetList();

  ImGui::TextUnformatted(u8"泛化参数:");
  ImGui::SameLine();
  DrawFuzzingFieldList();

  ImGui::TextUnformatted(u8"泛化机制:");
  ImGui::SameLine();
  ImGui::BeginGroup();
  DrawFuzzingPolicyList();
  DrawFuzzingConfig(&stub_config, "Km/h");
  ImGui::EndGroup();
}

}

PDWindow::~PDWindow() {}

PDWindow::PDWindow()
  : Window(u8"ProtoTyping") {
}

void PDWindow::onInitialize() {
  Window::onInitialize();

  ImGui::StyleColorsDark();

  loadTextures();

  socket_ = socket(AF_INET, SOCK_DGRAM, 0);

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(kSocketPort);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(socket_, (sockaddr*)&addr, sizeof(addr)) == -1) {
    printf("bind error");
  }
  std::thread([this]() {
    struct timeval timeout = {1, 0};  // 1s
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    while (true) {
      char buf[1024];
      int len = recv(socket_, buf, sizeof(buf), 0);
      if (len > 0) {
        std::string text;
        // to ascii
        for (int i = 0; i < len; i++) {
          if (buf[i] >= 0x20 && buf[i] <= 0x7e) {
            text += buf[i];
          }
        }
        printf("recv: %s\n", text.c_str());
      }
    }
  }).detach();
}

void PDWindow::onRender() {
  glClearColor(0.8, 0.8, 0.8, 1);
  glClear(GL_COLOR_BUFFER_BIT);
}

void PDWindow::onUIUpdate() {
  Window::onUIUpdate();

  ImGui::ShowDemoWindow();

  static int editing_signal_index = -1;
  static int trigger_type = 0;
  static glm::vec3 trigger_pos;
  static float trigger_distance;
  static char time_text[1024];
  static bool pinning = false;
  static function<void(glm::vec3)> on_position_selected;

  static int page_id = 0;
  constexpr const char* kPageName[] = {
    u8"参数配置",
    u8"泛化结果",
  };

  ImGui::Begin(u8"泛化");
  ImGui::TextUnformatted(u8"| 泛化配置");

  const float widget_width = ImGui::GetWindowContentRegionWidth();
  const float spacing = 20.0f;
  const float content_start_x = 200.0f;
  const float gizmo_start_x = widget_width - 50.0f;
  const float content_width = gizmo_start_x - content_start_x - spacing;
  const float table_row_height = ImGui::GetTextLineHeightWithSpacing();

  if (ImGui::BeginTabBar("##PageTab")) {
    for (int i = 0; i < 2; ++i) {
      bool opened = page_id == i;
      if (ImGui::BeginTabItem(kPageName[i], nullptr)) {
        page_id = i;
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
  }

  if (page_id == 0) {
    for (int i = 0; i < 2; ++i) {
      ImGui::PushID(i);
      DrawFuzzingConfigItem();
      ImGui::Separator();
      ImGui::PopID();
    }
  }

  if (page_id == 1) {
    ImGui::Columns(3, "##fuzzing_testcase");
    ImGui::Text(u8"Index");
    ImGui::NextColumn();
    ImGui::Text(u8"Config");
    ImGui::NextColumn();
    ImGui::Text(u8"Action");
    ImGui::NextColumn();

    for (int j = 0; j < 10; ++j) {
      for (int i = 0; i < 10; ++i) {
        int idx = j * 10 + i;
        ImGui::PushID(idx);
        ImGui::Text("#%d", idx);
        ImGui::NextColumn();
        ImGui::Text("ego speed xxx Km/h | obstacle #%d speed xxx Km/h", j);
        ImGui::NextColumn();
        ImGui::SmallButton(u8"预览");
        ImGui::SameLine();
        ImGui::SmallButton(u8"本地运行");
        ImGui::SameLine();
        ImGui::SmallButton(u8"移除");
        ImGui::NextColumn();
        ImGui::PopID();
      }
    }
    
  }

  int signal_count = 20;
  ImVec2 window_pos = ImGui::GetWindowPos();
  float window_scroll_x = ImGui::GetScrollX();
  float window_scroll_y = ImGui::GetScrollY();

  ImGui::End();
  
  if (pinning) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    if (ImGui::IsMouseDown(0)) {
      pinning = false;
      if (on_position_selected) {
        on_position_selected(glm::vec3(ImGui::GetMousePos().x, ImGui::GetMousePos().y, 0));
        on_position_selected = nullptr;
      }
    }
  }
}

void PDWindow::onUpdate() {
  Window::onUpdate();
}

void PDWindow::onResize(int w, int h) {
  Window::onResize(w, h);
}

void PDWindow::onEndFrame() { Window::onEndFrame(); }

void PDWindow::loadTextures() {
  {
    int w, h;
    auto data = stbi_load(u8"assets/ui/区域.png", &w, &h, 0, 4);
    if (data) {
      areaIcon.Create(1, w, h, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, data);
      free(data);
    }
  }
  {
    int w, h;
    auto data = stbi_load(u8"assets/ui/pin.png", &w, &h, 0, 4);
    if (data) {
      pinIcon.Create(1, w, h, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, data);
      free(data);
    }
  }
}

#undef min
#undef max
