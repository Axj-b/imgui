// dear imgui: RAII scope guards for the Begin*()/End*() container pairs.
//
// Companion to imgui_raii.h, which must be included for the ScopeBeginEnd primitive.
// Everything lives in the same ImGuiScoped namespace, so including this header is enough.
//
// Purpose: make it impossible to forget a matching End() call -- and impossible to call one
// when the Begin() did not actually take effect. Most Begin*() functions return false without
// pushing anything, and calling their End*() in that case is silent state corruption rather
// than a crash. The worst case is EndTabItem(), which has no soft guard at all and pops a
// parent ID unconditionally (imgui_widgets.cpp).
//
//   ImGui::BeginTabItem("General");  // if (false) ... ImGui::EndTabItem();  // pops parent ID!
//   if (ImGui::TabItem("General")) { ... }
//
// Only four pairs take the opposite rule, where Dear ImGui documents an UNCONDITIONAL End():
// Begin/End, BeginChild/EndChild, BeginMainMenuBar/EndMainMenuBar and BeginTooltip/EndTooltip.
// Getting this backwards in either direction is a bug, so the per-function comments below
// cite the specific imgui.cpp/imgui_widgets.cpp line that settles it.
//
// Usage:
//   #include "imgui.h"
//   #include "misc/cpp/imgui_raii_widgets.h"
//
//   void DrawStuff()
//   {
//       if (auto win = ImGuiScoped::Window("Settings"))
//       {
//           if (auto tab = ImGuiScoped::TabItem("General"))
//           {
//               auto width = ImGuiScoped::ItemWidth(200.0f);   // from imgui_raii.h
//               ImGui::Button("OK");
//           }
//       }
//   }
//
// Changelog:
//   - v0.10: Initial version.
//
// Requires C++17 (guaranteed copy elision; every guard is returned by value).
// Optional:
//   - #define IMGUI_RAII_USE_INTERNAL_API before including to additionally wrap
//     EndColumns() and BeginComboPreview()/EndComboPreview(), which are only
//     declared in imgui_internal.h. This pulls in imgui_internal.h.

#pragma once

#ifndef IMGUI_DISABLE

#include "imgui_raii.h"
#ifdef IMGUI_RAII_USE_INTERNAL_API
#include "imgui_internal.h"
#endif

namespace ImGuiScoped
{

//----------------------------------------------------------------------------
// Begin*()/End*() pairs with UNCONDITIONAL End*().
//
// These four are the documented exceptions: End() must run even when Begin() returned false.
// (imgui.h documents this for Begin/End and BeginChild/EndChild; BeginMainMenuBar and
// BeginTooltip self-end on their early-out paths but their End*() are written to tolerate
// the true case regardless, and matching Dear ImGui's own usage is the safe choice.)
//----------------------------------------------------------------------------

inline auto Window(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0)
{
    return MakeBeginEnd<false>(
        [=] { return ImGui::Begin(name, p_open, flags); },
        [] { ImGui::End(); });
}

inline auto Child(const char* str_id, const ImVec2& size = ImVec2(0, 0), ImGuiChildFlags child_flags = 0, ImGuiWindowFlags window_flags = 0)
{
    return MakeBeginEnd<false>(
        [=] { return ImGui::BeginChild(str_id, size, child_flags, window_flags); },
        [] { ImGui::EndChild(); });
}
inline auto Child(ImGuiID id, const ImVec2& size = ImVec2(0, 0), ImGuiChildFlags child_flags = 0, ImGuiWindowFlags window_flags = 0)
{
    return MakeBeginEnd<false>(
        [=] { return ImGui::BeginChild(id, size, child_flags, window_flags); },
        [] { ImGui::EndChild(); });
}

inline auto MainMenuBar()
{
    return MakeBeginEnd<false>(
        [] { return ImGui::BeginMainMenuBar(); },
        [] { ImGui::EndMainMenuBar(); });
}

inline auto Tooltip()
{
    return MakeBeginEnd<false>(
        [] { return ImGui::BeginTooltip(); },
        [] { ImGui::EndTooltip(); });
}

//----------------------------------------------------------------------------
// Begin*()/End*() pairs with CONDITIONAL End*().
//
// Every function below skips its End*() when Begin*() returned false, because on that path
// Dear ImGui pushed nothing. Calling End*() anyway is the bug this guard exists to prevent.
//----------------------------------------------------------------------------

// BeginItemTooltip() is conditional even though EndTooltip() is shared with Tooltip() above,
// because it early-outs when the preceding item is not hovered (imgui.cpp).
inline auto ItemTooltip()
{
    return MakeBeginEnd<true>(
        [] { return ImGui::BeginItemTooltip(); },
        [] { ImGui::EndTooltip(); });
}

inline auto Popup(const char* str_id, ImGuiWindowFlags flags = 0)
{
    return MakeBeginEnd<true>(
        [=] { return ImGui::BeginPopup(str_id, flags); },
        [] { ImGui::EndPopup(); });
}
inline auto PopupModal(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0)
{
    return MakeBeginEnd<true>(
        [=] { return ImGui::BeginPopupModal(name, p_open, flags); },
        [] { ImGui::EndPopup(); });
}
inline auto PopupContextItem(const char* str_id = nullptr, ImGuiPopupFlags popup_flags = 0)
{
    return MakeBeginEnd<true>(
        [=] { return ImGui::BeginPopupContextItem(str_id, popup_flags); },
        [] { ImGui::EndPopup(); });
}
inline auto PopupContextWindow(const char* str_id = nullptr, ImGuiPopupFlags popup_flags = 0)
{
    return MakeBeginEnd<true>(
        [=] { return ImGui::BeginPopupContextWindow(str_id, popup_flags); },
        [] { ImGui::EndPopup(); });
}
inline auto PopupContextVoid(const char* str_id = nullptr, ImGuiPopupFlags popup_flags = 0)
{
    return MakeBeginEnd<true>(
        [=] { return ImGui::BeginPopupContextVoid(str_id, popup_flags); },
        [] { ImGui::EndPopup(); });
}

// EndCombo() decrements g.BeginComboDepth before its own guard runs (imgui_widgets.cpp), so a
// stray EndCombo() corrupts subsequent combo window naming even in a build where the soft
// error guard catches it.
inline auto Combo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0)
{
    return MakeBeginEnd<true>(
        [=] { return ImGui::BeginCombo(label, preview_value, flags); },
        [] { ImGui::EndCombo(); });
}

// A closed sub-menu returns false with nothing pushed, while the current window is the *parent*
// menu, whose flags satisfy EndMenu()'s soft guard. A stray EndMenu() would close the parent.
inline auto Menu(const char* label, bool enabled = true)
{
    return MakeBeginEnd<true>(
        [=] { return ImGui::BeginMenu(label, enabled); },
        [] { ImGui::EndMenu(); });
}
inline auto MenuBar()
{
    return MakeBeginEnd<true>(
        [] { return ImGui::BeginMenuBar(); },
        [] { ImGui::EndMenuBar(); });
}

inline auto ListBox(const char* label, const ImVec2& size = ImVec2(0, 0))
{
    return MakeBeginEnd<true>(
        [=] { return ImGui::BeginListBox(label, size); },
        [] { ImGui::EndListBox(); });
}

// EndTable()'s soft guard only tests g.CurrentTable != NULL, which can still hold a previous
// frame's table; a false BeginTable() must not reach it.
inline auto Table(const char* str_id, int columns, ImGuiTableFlags flags = 0, const ImVec2& outer_size = ImVec2(0.0f, 0.0f), float inner_width = 0.0f)
{
    return MakeBeginEnd<true>(
        [=] { return ImGui::BeginTable(str_id, columns, flags, outer_size, inner_width); },
        [] { ImGui::EndTable(); });
}

inline auto TabBar(const char* str_id, ImGuiTabBarFlags flags = 0)
{
    return MakeBeginEnd<true>(
        [=] { return ImGui::BeginTabBar(str_id, flags); },
        [] { ImGui::EndTabBar(); });
}

// The worst case in the API: EndTabItem() has no soft guard whatsoever and calls PopID()
// unconditionally (imgui_widgets.cpp). A non-selected tab reaching it pops a parent ID.
inline auto TabItem(const char* label, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0)
{
    return MakeBeginEnd<true>(
        [=] { return ImGui::BeginTabItem(label, p_open, flags); },
        [] { ImGui::EndTabItem(); });
}

// TreeNode()/TreeNodeEx() already push TreePop() internally when they return true (unless
// ImGuiTreeNodeFlags_NoTreePushOnOpen), so this simply closes that scope on scope exit.
// CollapsingHeader() is not wrapped: it never pushes, so there is nothing to pop.
// IfOpened=true covers the visibility gate. The TreePop() itself is further gated on
// ImGuiTreeNodeFlags_NoTreePushOnOpen, which is decided at construction time from the flags.
inline auto TreeNode(const char* label, ImGuiTreeNodeFlags flags = 0)
{
    const bool no_tree_push_on_open = (flags & ImGuiTreeNodeFlags_NoTreePushOnOpen) != 0;
    return MakeBeginEnd<true>(
        [=] { return ImGui::TreeNodeEx(label, flags); },
        [=] { if (!no_tree_push_on_open) ImGui::TreePop(); });
}

inline auto DragDropSource(ImGuiDragDropFlags flags = 0)
{
    return MakeBeginEnd<true>(
        [=] { return ImGui::BeginDragDropSource(flags); },
        [] { ImGui::EndDragDropSource(); });
}
inline auto DragDropTarget()
{
    return MakeBeginEnd<true>(
        [] { return ImGui::BeginDragDropTarget(); },
        [] { ImGui::EndDragDropTarget(); });
}

//----------------------------------------------------------------------------
// ImGuiListClipper::Begin()/End().
//
// Not built on ScopeBeginEnd: End() is idempotent (it nulls TempData before returning) and
// ImGuiListClipper's own destructor already calls it, so this only bounds the clipper's
// lifetime to a block, or ends it early via dismiss().
//----------------------------------------------------------------------------

class ScopeListClipper
{
public:
    inline ScopeListClipper(ImGuiListClipper& clipper, int items_count, float items_height = -1.0f)
        : m_clipper(&clipper), m_active(true)
    {
        clipper.Begin(items_count, items_height);
    }
    inline ~ScopeListClipper() { if (m_active) m_clipper->End(); }

#ifdef IMGUI_RAII_DISABLE_COPY
    ScopeListClipper(const ScopeListClipper&) = default;
    ScopeListClipper& operator=(const ScopeListClipper&) = default;
#else
    ScopeListClipper(const ScopeListClipper&) = delete;
    ScopeListClipper& operator=(const ScopeListClipper&) = delete;
#endif
    ScopeListClipper& operator=(ScopeListClipper&&) = delete;

    void dismiss() { m_active = false; }
    bool is_active() const { return m_active; }

private:
    ImGuiListClipper* m_clipper;
    bool              m_active;
};

//----------------------------------------------------------------------------
// BeginMultiSelect()/EndMultiSelect() -- stateful, so not built on MakeBeginEnd().
//
// The Begin*() pointer is only valid up to End*(), while the End*() pointer is the one you must
// feed to ApplyRequests() afterwards. So the guard takes an out-parameter that the destructor
// (or an early End()) fills:
//   ImGuiMultiSelectIO* ms_io = nullptr;
//   {
//       auto ms = ImGuiScoped::MultiSelect(&ms_io, ImGuiMultiSelectFlags_NoAutoSelect);
//       ImGui::Selectable("Item", &selected);
//   }
//   ImGui::ApplyRequests(ms_io);
//
// The Begin*() pointer is reachable via operator->() for reading (IsItemToggledSelection() etc).
//----------------------------------------------------------------------------
class MultiSelect
{
public:
    inline MultiSelect(ImGuiMultiSelectIO** out_ms_io, ImGuiMultiSelectFlags flags, int selection_size = -1, int items_count = -1)
        : m_out_ms_io(out_ms_io), m_ms_io_begin(ImGui::BeginMultiSelect(flags, selection_size, items_count)), m_active(true)
    {
    }
    inline ~MultiSelect() { if (m_active) End(); }

#ifdef IMGUI_RAII_DISABLE_COPY
    MultiSelect(const MultiSelect&) = default;
    MultiSelect& operator=(const MultiSelect&) = default;
#else
    MultiSelect(const MultiSelect&) = delete;
    MultiSelect& operator=(const MultiSelect&) = delete;
#endif
    MultiSelect& operator=(MultiSelect&&) = delete;

    // Begin*() pointer, valid until this scope ends.
    ImGuiMultiSelectIO* operator->() const { return m_ms_io_begin; }
    ImGuiMultiSelectIO* io() const { return m_ms_io_begin; }
    explicit operator bool() const { return m_ms_io_begin != nullptr; }

    // End early and hand back the End*() pointer. Idempotent.
    inline ImGuiMultiSelectIO* End()
    {
        if (!m_active) return *m_out_ms_io;
        m_active = false;
        m_ms_io_begin = nullptr; // the Begin*() pointer is dead past this point
        *m_out_ms_io = ImGui::EndMultiSelect();
        return *m_out_ms_io;
    }
    bool is_active() const { return m_active; }

private:
    ImGuiMultiSelectIO** m_out_ms_io;
    ImGuiMultiSelectIO*  m_ms_io_begin;
    bool                 m_active;
};

//----------------------------------------------------------------------------
// Table helpers.
//
// These are not guards: TableNextRow()/TableNextColumn()/TableSetColumnIndex() have no matching
// End*(). They exist because the "for each column, skip if not visible" dance is verbose.
//----------------------------------------------------------------------------

// TableNextRow() + TableSetColumnIndex(0).
inline bool Row(ImGuiTableRowFlags row_flags = 0, float min_row_height = 0.0f)
{
    ImGui::TableNextRow(row_flags, min_row_height);
    return ImGui::TableSetColumnIndex(0);
}

// TableNextColumn(): true when the column is visible.
inline bool NextColumn() { return ImGui::TableNextColumn(); }

// TableSetColumnIndex(column_n): true when the column is visible.
inline bool Column(int column_n) { return ImGui::TableSetColumnIndex(column_n); }

//----------------------------------------------------------------------------
// Internal-API-only wrappers (IMGUI_RAII_USE_INTERNAL_API).
//----------------------------------------------------------------------------
#ifdef IMGUI_RAII_USE_INTERNAL_API

inline auto Columns(int count, const char* id = nullptr, bool borders = true)
{
    ImGui::Columns(count, id, borders);
    return MakeScope([] { ImGui::EndColumns(); });
}

inline auto ComboPreview()
{
    return MakeBeginEnd<true>(
        [] { return ImGui::BeginComboPreview(); },
        [] { ImGui::EndComboPreview(); });
}

#endif // IMGUI_RAII_USE_INTERNAL_API

} // namespace ImGuiScoped

#endif // #ifndef IMGUI_DISABLE
