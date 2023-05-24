#include "app/RenderObject.h"

namespace {

using factory_func = std::function<std::unique_ptr<RenderObject>()>;
const std::map<std::string, factory_func> kRegisteredFacotryFuncMap{
    {"LineObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<LineObject>();
     }},
    {"DashedStripeObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<DashedStripeObject>();
     }},
    {"SimpleTexturedObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<SimpleTexturedObject>();
     }},
    {"LaneRenderObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<RoadElementObject>();
     }},
    {"JunctionRenderObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<RoadElementObject>();
     }},
    {"LaneCenterCurveRenderObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<RoadElementObject>();
     }},
    {"PolygonObjectRenderObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<RoadElementObject>();
     }},
    {"SidewalkRenderObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<RoadElementObject>();
     }},
    {"SpeedBumpRenderObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<RoadElementObject>();
     }},
    {"SignalLaneRenderObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<RoadElementObject>();
     }},
    {"SignalStopLineRenderObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<RoadElementObject>();
     }},
    {"CrosswalkRenderObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<RoadElementObject>();
     }},
    {"GroupObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<RoadElementObject>();
     }},
    {"ClearAreaRenderObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<RoadElementObject>();
     }},
    {"StopLineRenderObject",
     []() -> std::unique_ptr<RenderObject> {
       return std::make_unique<RoadElementObject>();
     }},
};

}  // namespace

std::unique_ptr<RenderObject> CreateRenderObjectFromJson(
    const nlohmann::json& json) {
  std::string type_name = json["type"];
  if (kRegisteredFacotryFuncMap.find(type_name) ==
      kRegisteredFacotryFuncMap.end()) {
    printf("Not registered type name: %s\n", type_name.c_str());
    return nullptr;
  }
  std::unique_ptr<RenderObject> object =
      kRegisteredFacotryFuncMap.at(type_name)();
  object->SerializeFromJson(json);
  return object;
}
