#pragma once
#include <random>
#include <memory>
#include <vector>
#include <stack>
#include <map>
#include "core/Window.h"
#include "core/Texture2D.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>

class PDWindow : public Window {
  Texture2D areaIcon;
  Texture2D pinIcon;

  void loadTextures();

  int socket_ = -1;
public:
  PDWindow();
  ~PDWindow();
  virtual void onInitialize();
  virtual void onRender();
  virtual void onUIUpdate();
  virtual void onUpdate();
  virtual void onResize(int w, int h);
  virtual void onEndFrame();
};
