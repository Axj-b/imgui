// dear imgui: RAII scope guards for the Push*()/Pop*() state pairs.
//
// This is an example of how you may wrap Dear ImGui with your own C++ helpers.
//
// Purpose: make it impossible to forget a matching Pop() call. Every guard here restores
// the previous state unconditionally, so there is no Begin*/End* semantics to get wrong.
//
//   ImGui::PushStyleColor(ImGuiCol_Text, red);
//   ImGui::Text("hi");
//   ImGui::PopStyleColor();
//
// becomes:
//
//   auto red_text = ImGuiScoped::StyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
//   ImGui::Text("hi");
//
// The Begin*()/End*() container pairs live in imgui_raii_widgets.h, which includes this
// header for the ScopeBeginEnd primitive.
//
// Usage:
//   #include "imgui.h"
//   #include "misc/cpp/imgui_raii.h"
//
// Changelog:
//   - v0.10: Initial version.
//
// Requires C++17 (guaranteed copy elision; every guard is returned by value).
// Optional:
//   - #define IMGUI_RAII_DISABLE_COPY to re-enable copying guards (not recommended,
//     a copied guard will pop a stack the original still believes it owns).

#pragma once

#ifndef IMGUI_DISABLE

#include "imgui.h"
#ifdef IMGUI_RAII_USE_INTERNAL_API
#include "imgui_internal.h"
#endif

#include <initializer_list>
#include <type_traits>
#include <utility>

namespace ImGuiScoped
{

//----------------------------------------------------------------------------
// Core primitives. Everything below is built from these two, so the semantics
// live in exactly one place.
//----------------------------------------------------------------------------

// Always-pop guard for Push*()/Pop*() pairs.
template <typename PopFn>
struct Scope
{
    PopFn m_pop;
    bool  m_active;

    explicit Scope(PopFn&& pop) : m_pop(std::move(pop)), m_active(true) {}
    Scope(Scope&& other) noexcept : m_pop(std::move(other.m_pop)), m_active(other.m_active) { other.m_active = false; }
    ~Scope() { if (m_active) m_pop(); }

#ifdef IMGUI_RAII_DISABLE_COPY
    Scope(const Scope&) = default;
    Scope& operator=(const Scope&) = default;
#else
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
#endif
    Scope& operator=(Scope&&) = delete;

    // Opt out of the Pop() call, e.g. to hand the pushed state to a callee.
    void dismiss() { m_active = false; }
    bool is_active() const { return m_active; }
};

// Begin/End guard.
//   IfOpened == true : End() runs only if Begin() returned true. (all Begin* but the four below)
//   IfOpened == false: End() runs unconditionally. (Begin, BeginChild, BeginMainMenuBar, BeginTooltip)
// Converts to bool so it can be used directly as the `if` condition, and reports
// Begin()'s raw return value in both modes so you can still early-out.
template <bool IfOpened, typename EndFn>
struct ScopeBeginEnd
{
    EndFn m_end;
    bool  m_opened;
    bool  m_active;

    ScopeBeginEnd(EndFn&& end, bool opened) : m_end(std::move(end)), m_opened(opened), m_active(true) {}
    ScopeBeginEnd(ScopeBeginEnd&& other) noexcept : m_end(std::move(other.m_end)), m_opened(other.m_opened), m_active(other.m_active) { other.m_active = false; }
    ~ScopeBeginEnd() { if (m_active && (!IfOpened || m_opened)) m_end(); }

#ifdef IMGUI_RAII_DISABLE_COPY
    ScopeBeginEnd(const ScopeBeginEnd&) = default;
    ScopeBeginEnd& operator=(const ScopeBeginEnd&) = default;
#else
    ScopeBeginEnd(const ScopeBeginEnd&) = delete;
    ScopeBeginEnd& operator=(const ScopeBeginEnd&) = delete;
#endif
    ScopeBeginEnd& operator=(ScopeBeginEnd&&) = delete;

    explicit operator bool() const { return m_opened; }
    bool opened() const { return m_opened; }
    void dismiss() { m_active = false; }
    bool is_active() const { return m_active; }
};

template <typename PopFn>
inline Scope<std::decay_t<PopFn>> MakeScope(PopFn&& pop)
{
    return Scope<std::decay_t<PopFn>>(std::forward<PopFn>(pop));
}

// Calls begin(args...) to obtain the visibility flag, then builds the matching guard.
// `opened` is consumed by the guard; the begin/end callables are invoked exactly once each.
template <bool IfOpened, typename BeginFn, typename EndFn, typename... BeginArgs>
inline ScopeBeginEnd<IfOpened, std::decay_t<EndFn>> MakeBeginEnd(BeginFn&& begin, EndFn&& end, BeginArgs&&... begin_args)
{
    const bool opened = begin(std::forward<BeginArgs>(begin_args)...);
    return ScopeBeginEnd<IfOpened, std::decay_t<EndFn>>(std::forward<EndFn>(end), opened);
}

// Generic escape hatch for anything not covered below:
//   auto restore = ImGuiScoped::OnScopeExit([]{ ImGui::PopStyleColor(3); });
template <typename PopFn>
inline Scope<std::decay_t<PopFn>> OnScopeExit(PopFn&& pop)
{
    return MakeScope(std::forward<PopFn>(pop));
}

//----------------------------------------------------------------------------
// State stacks: Push*() / Pop*()
//----------------------------------------------------------------------------

// PopStyleColor(1)
inline auto StyleColor(ImGuiCol idx, const ImVec4& col)
{
    ImGui::PushStyleColor(idx, col);
    return MakeScope([] { ImGui::PopStyleColor(1); });
}
inline auto StyleColor(ImGuiCol idx, ImU32 col)
{
    ImGui::PushStyleColor(idx, col);
    return MakeScope([] { ImGui::PopStyleColor(1); });
}

// PopStyleColor(N): push several colors, pop them in one call.
//   auto c = ImGuiScoped::StyleColors({ { ImGuiCol_Text, red }, { ImGuiCol_Button, blue } });
struct ColorOverride
{
    ImGuiCol Col;
    ImVec4   Value;

    ColorOverride(ImGuiCol col, const ImVec4& value) : Col(col), Value(value) {}
    ColorOverride(ImGuiCol col, ImU32 value) : Col(col), Value(ImGui::ColorConvertU32ToFloat4(value)) {}
};
inline auto StyleColors(std::initializer_list<ColorOverride> overrides)
{
    for (const ColorOverride& o : overrides)
        ImGui::PushStyleColor(o.Col, o.Value);
    const int count = (int)overrides.size();
    return MakeScope([count] { ImGui::PopStyleColor(count); });
}

// PopStyleVar(1). The three overloads mirror the float / ImVec2 / X-Y split of PushStyleVar().
inline auto StyleVar(ImGuiStyleVar idx, float val)
{
    ImGui::PushStyleVar(idx, val);
    return MakeScope([] { ImGui::PopStyleVar(1); });
}
inline auto StyleVar(ImGuiStyleVar idx, const ImVec2& val)
{
    ImGui::PushStyleVar(idx, val);
    return MakeScope([] { ImGui::PopStyleVar(1); });
}
inline auto StyleVarX(ImGuiStyleVar idx, float val_x)
{
    ImGui::PushStyleVarX(idx, val_x);
    return MakeScope([] { ImGui::PopStyleVar(1); });
}
inline auto StyleVarY(ImGuiStyleVar idx, float val_y)
{
    ImGui::PushStyleVarY(idx, val_y);
    return MakeScope([] { ImGui::PopStyleVar(1); });
}

// PopStyleVar(N). A style var is either a float or an ImVec2 depending on which
// ImGuiStyleVar_ constant it is, so the override carries both plus a discriminator.
//   auto v = ImGuiScoped::StyleVars({ { ImGuiStyleVar_FrameRounding, 0.0f }, { ImGuiStyleVar_ItemSpacing, ImVec2(8, 4) } });
struct StyleVarOverride
{
    ImGuiStyleVar Var;
    ImVec2        Value;
    bool          IsVec2;

    StyleVarOverride(ImGuiStyleVar var, float value) : Var(var), Value(value, value), IsVec2(false) {}
    StyleVarOverride(ImGuiStyleVar var, const ImVec2& value) : Var(var), Value(value), IsVec2(true) {}
};
inline auto StyleVars(std::initializer_list<StyleVarOverride> overrides)
{
    for (const StyleVarOverride& o : overrides)
    {
        if (o.IsVec2) ImGui::PushStyleVar(o.Var, o.Value);
        else          ImGui::PushStyleVar(o.Var, o.Value.x);
    }
    const int count = (int)overrides.size();
    return MakeScope([count] { ImGui::PopStyleVar(count); });
}

// PopID(). The four overloads mirror PushID()'s string / string-range / pointer / int.
inline auto ID(const char* str_id)
{
    ImGui::PushID(str_id);
    return MakeScope([] { ImGui::PopID(); });
}
// WARNING: 'str_id_begin' and 'str_id_end' must index into the SAME contiguous buffer.
// PushID() computes str_end - str, so two separate string literals give a garbage length:
//   ScopeID("set", "tings")   <-- WRONG, the two literals are not adjacent
//   ScopeID(kBuf, kBuf + 4)   <-- right
inline auto ID(const char* str_id_begin, const char* str_id_end)
{
    ImGui::PushID(str_id_begin, str_id_end);
    return MakeScope([] { ImGui::PopID(); });
}
inline auto ID(const void* ptr_id)
{
    ImGui::PushID(ptr_id);
    return MakeScope([] { ImGui::PopID(); });
}
inline auto ID(int int_id)
{
    ImGui::PushID(int_id);
    return MakeScope([] { ImGui::PopID(); });
}

// PopFont().
// `font_size_base_unscaled` < 0 is a sentinel meaning "use the size the font was added with"
// (font->LegacySize), which reproduces the pre-1.92 single-argument PushFont(font) behavior.
inline auto Font(ImFont* font, float font_size_base_unscaled = -1.0f)
{
    const float size = (font_size_base_unscaled < 0.0f && font != nullptr) ? font->LegacySize : font_size_base_unscaled;
    ImGui::PushFont(font, size);
    return MakeScope([] { ImGui::PopFont(); });
}

// PopItemWidth()
inline auto ItemWidth(float item_width)
{
    ImGui::PushItemWidth(item_width);
    return MakeScope([] { ImGui::PopItemWidth(); });
}

// PopTextWrapPos()
inline auto TextWrapPos(float wrap_local_pos_x = 0.0f)
{
    ImGui::PushTextWrapPos(wrap_local_pos_x);
    return MakeScope([] { ImGui::PopTextWrapPos(); });
}

// PopItemFlag()
inline auto ItemFlag(ImGuiItemFlags option, bool enabled)
{
    ImGui::PushItemFlag(option, enabled);
    return MakeScope([] { ImGui::PopItemFlag(); });
}
inline auto TabStop(bool tab_stop = true)
{
    ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, !tab_stop);
    return MakeScope([] { ImGui::PopItemFlag(); });
}
inline auto ButtonRepeat(bool repeat = true)
{
    ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, repeat);
    return MakeScope([] { ImGui::PopItemFlag(); });
}

// PopClipRect()
inline auto ClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect)
{
    ImGui::PushClipRect(clip_rect_min, clip_rect_max, intersect_with_current_clip_rect);
    return MakeScope([] { ImGui::PopClipRect(); });
}

// Unindent(). The same width is passed back so nested negative/zero values resolve to
// style.IndentSpacing symmetrically, exactly like a hand-written Indent()/Unindent() pair.
inline auto Indent(float indent_w = 0.0f)
{
    ImGui::Indent(indent_w);
    return MakeScope([indent_w] { ImGui::Unindent(indent_w); });
}

// EndGroup()
inline auto Group()
{
    ImGui::BeginGroup();
    return MakeScope([] { ImGui::EndGroup(); });
}

// EndDisabled(). No bool gate: BeginDisabled(false) is documented as a no-op that is still
// balanced by an EndDisabled(), and EndDisabled() is soft-guarded internally (imgui.cpp).
inline auto Disabled(bool disabled = true)
{
    ImGui::BeginDisabled(disabled);
    return MakeScope([] { ImGui::EndDisabled(); });
}

// TreePop(). ~ Unindent() + PopID(). TreeNode()/TreeNodeEx() already do this push for you when
// they return true; this is for the TreePush()/TreePop() pair used directly, e.g. with a filter.
inline auto TreePush(const char* str_id)
{
    ImGui::TreePush(str_id);
    return MakeScope([] { ImGui::TreePop(); });
}
inline auto TreePush(const void* ptr_id)
{
    ImGui::TreePush(ptr_id);
    return MakeScope([] { ImGui::TreePop(); });
}


} // namespace ImGuiScoped

#endif // #ifndef IMGUI_DISABLE
