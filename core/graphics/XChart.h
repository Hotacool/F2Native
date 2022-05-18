#ifndef XG_GRAPHICS_XCHART_H
#define XG_GRAPHICS_XCHART_H

#if defined(__APPLE__)
#include "../ios/CoreGraphicsContext.h"
#include <TargetConditionals.h>
#endif

#if defined(ANDROID)
#include "android/BitmapCanvasContext.h"
#endif

#if defined(__EMSCRIPTEN__)
#include "webassembly/WebCanvasContext.h"
#endif

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "../utils/Tracer.h"
#include "animate/GeomAnimate.h"
#include "animate/TimeLine.h"
#include "axis/AxisController.h"
#include "canvas/Canvas.h"
#include "canvas/CanvasContext.h"
#include "canvas/Coord.h"
#include "event/EventController.h"
#include "func/Command.h"
#include "geom/Area.h"
#include "geom/Candle.h"
#include "geom/Interval.h"
#include "geom/Line.h"
#include "geom/Point.h"
#include "geom/shape/GeomShapeFactory.h"
#include "guide/GuideController.h"
#include "interaction/InteractionBase.h"
#include "interaction/InteractionContext.h"
#include "interaction/Pan.h"
#include "interaction/Pinch.h"
#include "legend/LegendController.h"
#include "scale/ScaleController.h"
#include "tooltip/TooltipController.h"
#include "../nlohmann/json.hpp"


#define XG_MEMBER_CALLBACK(funPtr) std::bind(&funPtr, this)
#define XG_MEMBER_CALLBACK_1(funPtr) std::bind(&funPtr, this, std::placeholders::_1)
#define XG_MEMBER_OWNER_CALLBACK_1(funPtr, owner) std::bind(&funPtr, owner, std::placeholders::_1)

#define XG_RELEASE_POINTER(ptr)                                                                                                \
    if(ptr != nullptr)                                                                                                         \
        delete ptr;                                                                                                            \
    ptr = nullptr;

#define ACTION_CHART_AFTER_INIT "ChartAfterInit"
#define ACTION_CHART_AFTER_GEOM_DRAW "ChartAfterGeomDraw"
#define ACTION_CHART_AFTER_RENDER "ChartAfterRender"
#define ACTION_CHART_CLEAR_INNER "ChartClearInner"
#define ACTION_CHART_BEFORE_CANVAS_DRAW "ChartBeforeCanvasDraw"
#define ACTION_CHART_AFTER_CANVAS_DESTROY "ChartAfterCanvasDestroy"

using namespace std;

namespace xg {

typedef std::function<void()> ChartActionCallback;

class XChart {
    friend axis::AxisController;
    friend geom::AbstractGeom;
    friend geom::Line;
    friend geom::Interval;
    friend geom::Area;
    friend geom::Point;
    friend geom::Candle;
    friend legend::LegendController;
    friend event::EventController;
    friend tooltip::ToolTipController;
    friend interaction::Pan;
    friend interaction::Pinch;
    friend interaction::InteractionContext;
    friend animate::TimeLine;
    friend animate::GeomAnimate;

  public:
    XChart(const std::string &name, double width, double height, double ratio = 1.0);
    virtual ~XChart();

    // emscript 编译需要
    XChart(const XChart &){};

    XChart &Source(const std::string &json);

#if defined(ANDROID)
    ///方便降级 稳定后改回SetCanvasContext
    XChart &SetAndroidCanvasContext(jobject context) {
        XG_RELEASE_POINTER(canvasContext_);
        canvasContext_ = new canvas::BitmapCanvasContext(context, static_cast<float>(ratio_));
        return *this;
    }
#endif

#if defined(__APPLE__)
    ///方便降级 稳定后改回SetCanvasContext
    /// context == CGContextRef
    XChart &SetCoreGraphicsContext(void *context) {
        XG_RELEASE_POINTER(canvasContext_);
        canvasContext_ = new canvas::CoreGraphicsContext(context, width_, height_, static_cast<float>(ratio_), nullptr);
        return *this;
    }
#endif

#if defined(__EMSCRIPTEN__)
    XChart &SetCanvasContext(const std::string &canvasName) {
        XG_RELEASE_POINTER(canvasContext_);
        canvasContext_ = new canvas::WebCanvasContext(canvasName, width_, height_, ratio_);
        return *this;
    }
#endif // __EMSCRIPTEN__
    
    /// 设置chart的canvasConext，这里和上面的方法增加了一个ownCanvasContext_来区分谁创建的canvasContext
    /// @param canvasContext 绘制的context
    XChart &SetCanvasContext(canvas::CanvasContext *canvasContext) {
        ownCanvasContext_ = false;
        canvasContext_ = canvasContext;
        return *this;
    }

    XChart &Padding(double left = 0.f, double top = 0.f, double right = 0.0, double bottom = 0.);
    XChart &Margin(double left = 0.f, double top = 0.f, double right = 0.0, double bottom = 0.);

    XChart &Scale(const std::string &field, const std::string &json);

    XChart &Axis(const std::string &field, const std::string &json = "");

    XChart &Legend(const std::string &field, const std::string &json = "");

    guide::GuideController &Guide() { return *this->guideController_; }

    XChart &Interaction(const std::string &type, const std::string &json = "");

    XChart &Tooltip(const std::string &json = "");

    XChart &Coord(const std::string &json = "");

    XChart &Animate(const std::string &json = "");

    bool OnTouchEvent(const std::string &json = "");

    geom::Line &Line();
    geom::Interval &Interval();
    geom::Area &Area();
    geom::Point &Point();
    geom::Candle &Candle();

    ///渲染是否成功, 渲染失败的原因有
    ///1.未设置canvasContext
    ///2.设置了canvasContext， 但canvasContext->isValid 不合法
    ///3.传进来的source不是array或者数据长度为0
    ///@return true 渲染成功 false 失败
    bool Render();

    void Repaint();

    void Clear();

    std::size_t RequestAnimationFrame(func::Command *c, long delay = 16);

    scale::AbstractScale &GetScale(const std::string &field);

    /// 暴露共有属性
    inline nlohmann::json &GetData() { return data_; }
    inline canvas::coord::AbstractCoord &GetCoord() { return *(coord_.get()); }
    inline void SetColDefs() {}

    inline std::string GetXScaleField() { return geoms_[0]->GetXScaleField(); }

    std::vector<std::string> getYScaleFields();

    inline const std::string &GetChartName() { return this->chartName_; }
    ///chart的唯一id chartName_+ uniqueId
    ///@return chartId_
    inline const std::string &GetChartId() { return this->chartId_; }

    inline utils::Tracer *GetLogTracer() const { return this->logTracer_; }

    inline const double GetWidth() const noexcept { return width_; }
    inline const double GetHeight() const noexcept { return height_; }

    std::string GetScaleTicks(const std::string &field) noexcept;

    // 计算某一项数据对应的坐标位置
    const util::Point GetPosition(const nlohmann::json &item);

    // 计算某个位置对应的数据信息
    const std::string GetTooltipInfos(float touchX, float touchY, int geomIndex);

    std::string GetRenderInfo() const;

    canvas::CanvasContext &GetCanvasContext() const { return *canvasContext_; }

    void AddMonitor(const std::string &action, ChartActionCallback callback, bool forChart = true) {
        if(forChart) {
            chartActionListeners_[action].push_back(callback);
        } else {
            renderActionListeners_[action].push_back(callback);
        }
    }

    inline const std::array<double, 4> GetPadding() { return userPadding_; }
    inline const std::array<double, 4> GetMargin() { return margin_; }

    inline void SetRequestFrameFuncId(std::string funcId) noexcept { this->requestFrameHandleId_ = funcId; }

    /// bind long long to webassembly会报错，所以改成long,渲染时间用long表示也够用了
    inline long GetRenderDurationMM() const { return renderDurationMM_;}
    inline long GetRenderCmdCount() const { return canvasContext_->GetRenderCount();}
    
    void Redraw();

// webassembly返回指针
#if defined(__EMSCRIPTEN__)
    XChart *SetCanvasContextWasm(const std::string &canvasName) { return &SetCanvasContext(canvasName); }
    XChart *SourceWasm(const std::string &json) { return &Source(json); }
    canvas::CanvasContext *GetCanvasContextWasm() const { return canvasContext_; }
    XChart *PaddingWasm(double left = 0.f, double top = 0.f, double right = 0.0, double bottom = 0.) {
        return &Padding(left, top, right, bottom);
    }
    XChart *MarginWasm(double left = 0.f, double top = 0.f, double right = 0.0, double bottom = 0.) {
        return &Margin(left, top, right, bottom);
    }
    XChart *ScaleWasm(const std::string &field, const std::string &json) { return &Scale(field, json); }
    XChart *AxisWasm(const std::string &field, const std::string &json) { return &Axis(field, json); }
    XChart *LegendWasm(const std::string &field, const std::string &json) { return &Legend(field, json); }
    guide::GuideController *GuideWasm() { return &Guide(); }
    XChart *InteractionWasm(const std::string &type, const std::string &json) { return &Interaction(type, json); }
    XChart *TooltipWasm(const std::string &json) { return &Tooltip(json); }
    XChart *CoordWasm(const std::string &json) { return &Coord(json); }
    XChart *AnimateWasm(const std::string &json) { return &Animate(json); }
    geom::Line *LineWasm() { return &Line(); }
    geom::Interval *IntervalWasm() { return &Interval(); }
    geom::Area *AreaWasm() { return &Area(); }
    geom::Point *PointWasm() { return &Point(); }
    geom::Candle *CandleWasm() { return &Candle(); }
#endif
  private:
    static long long CreateChartId() {
        static std::atomic<long long> id(1);
        return id.fetch_add(+1);
    }

    // 初始化布局边界和三层视图容器
    void InitLayout();
    // 初始化轴管理器
    // void InitAixsController();

    // 初始化坐标系
    void InitCoord();
    // 初始化图元
    //    void InitGeoms();

    void UpdateLayout(std::array<double, 4> newPadding);

    std::map<std::string, std::vector<legend::LegendItem>> GetLegendItems();

    void ClearInner();

    void NotifyAction(const std::string &action) {
        auto &chartCallbacks = chartActionListeners_[action];
        std::for_each(chartCallbacks.begin(), chartCallbacks.end(), [&](ChartActionCallback &callback) -> void { callback(); });

        auto &renderCallbacks = renderActionListeners_[action];
        std::for_each(renderCallbacks.begin(), renderCallbacks.end(), [&](ChartActionCallback &callback) -> void { callback(); });
    }

  protected:
    bool rendered_ = false;
    nlohmann::json data_;
    std::unique_ptr<canvas::coord::AbstractCoord> coord_ = nullptr;
    scale::ScaleController *scaleController_ = nullptr;
    canvas::Canvas *canvas_ = nullptr;
    axis::AxisController *axisController_ = nullptr;
    guide::GuideController *guideController_ = nullptr;
    event::EventController *eventController_ = nullptr;
    tooltip::ToolTipController *tooltipController_ = nullptr;
    interaction::InteractionContext *interactionContext_ = nullptr;
    legend::LegendController *legendController_ = nullptr;
    animate::GeomAnimate *geomAnimate_ = nullptr;
    std::array<double, 4> padding_ = {{0, 0, 0, 0}};
    std::array<double, 4> userPadding_ = {{0, 0, 0, 0}};
    std::array<double, 4> margin_ = {{0, 0, 0, 0}};

    // 析构时要析构对应的 Geom 对象
    std::vector<std::unique_ptr<geom::AbstractGeom>> geoms_;

    std::string chartName_;
    double width_;
    double height_;
    double ratio_ = 1.0f;
    long long renderDurationMM_ = 0;

    // 默认三层渲染
    shape::Group *backLayout_;
    shape::Group *midLayout_;
    shape::Group *frontLayout_;

    canvas::CanvasContext *canvasContext_ = nullptr;
    bool ownCanvasContext_ = true;

    utils::Tracer *logTracer_ = nullptr;
    std::unique_ptr<geom::shape::GeomShapeFactory> geomShapeFactory_ = nullptr;

    nlohmann::json coordCfg_ = {{"type", "cartesian"}, {"transposed", false}};

    std::map<std::string, std::vector<ChartActionCallback>> renderActionListeners_{}; // 针对每次渲染的监听事件， clear 时会清除
    std::map<std::string, std::vector<ChartActionCallback>> chartActionListeners_{}; // 针对 chart 实例的监听事件，只有在 chart 回收时才清除
    std::vector<std::unique_ptr<xg::interaction::InteractionBase>> interactions_{};

    std::string chartId_;
    std::string requestFrameHandleId_ = "";
    nlohmann::json animateCfg_ = false;
};
} // namespace xg

#endif // XG_GRAPHICS_XCHART_H