// dear imgui: usage sample for imgui_raii.h.
// This file is a compile-only smoke test: it exercises every wrapper so the header is known
// to instantiate, and it doubles as the reference for what the call sites look like.

#include "imgui.h"
#include "misc/cpp/imgui_raii.h"        // Push*()/Pop*() state guards
#include "misc/cpp/imgui_raii_widgets.h"  // Begin*()/End*() container guards

namespace {

//---- Style / state scopes. Everything restores itself at the closing brace.

void DrawStateScopes(ImFont* font, const void* item_ptr, int item_index)
{
    auto color = ImGuiScoped::StyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
    auto color_u32 = ImGuiScoped::StyleColor(ImGuiCol_Text, IM_COL32_WHITE);

    auto colors = ImGuiScoped::StyleColors({
        { ImGuiCol_Text,     ImVec4(1, 0, 0, 1) },
        { ImGuiCol_Button,   IM_COL32(0,0,255,255) },
    });

    auto var = ImGuiScoped::StyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    auto var2 = ImGuiScoped::StyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    auto varx = ImGuiScoped::StyleVarX(ImGuiStyleVar_FramePadding, 4.0f);
    auto vary = ImGuiScoped::StyleVarY(ImGuiStyleVar_FramePadding, 4.0f);

    auto vars = ImGuiScoped::StyleVars({
        { ImGuiStyleVar_FrameRounding, 0.0f },
        { ImGuiStyleVar_ItemSpacing,   ImVec2(8, 4) },
    });

    // begin/end must index into one contiguous buffer; two literals would not be adjacent.
    static const char kBuf[] = "settings";

    auto id_str = ImGuiScoped::ID("settings");
    auto id_range = ImGuiScoped::ID(kBuf, kBuf + 4);
    auto id_ptr = ImGuiScoped::ID(item_ptr);
    auto id_int = ImGuiScoped::ID(item_index);

    auto font_sized = ImGuiScoped::Font(font, 20.0f);
    auto font_legacy = ImGuiScoped::Font(font);          // size the font was added with
    auto font_keep = ImGuiScoped::Font(nullptr, 0.0f);    // keep font, change size only

    auto width = ImGuiScoped::ItemWidth(200.0f);
    auto wrap = ImGuiScoped::TextWrapPos();
    auto wrap_at = ImGuiScoped::TextWrapPos(100.0f);
    auto flag = ImGuiScoped::ItemFlag(ImGuiItemFlags_ButtonRepeat, true);
    auto tabstop = ImGuiScoped::TabStop();
    auto repeat = ImGuiScoped::ButtonRepeat(false);
    auto clip = ImGuiScoped::ClipRect(ImVec2(0, 0), ImVec2(100, 100), true);

    auto indent = ImGuiScoped::Indent();
    auto indent_w = ImGuiScoped::Indent(8.0f);
    auto group = ImGuiScoped::Group();
    auto disabled = ImGuiScoped::Disabled();
    auto disabled_off = ImGuiScoped::Disabled(false);

    auto tree = ImGuiScoped::TreePush("branch");
    auto tree_ptr = ImGuiScoped::TreePush(item_ptr);

    // Hand the pushed state to someone else: dismiss() opts out of the pop.
    auto handed_off = ImGuiScoped::StyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1));
    handed_off.dismiss();

    auto anything = ImGuiScoped::OnScopeExit([] { ImGui::PopStyleColor(3); });

    // Unused-variable warnings are expected here; this is a compile test.
    (void)color; (void)color_u32; (void)colors; (void)var; (void)var2; (void)varx; (void)vary;
    (void)vars; (void)id_str; (void)id_range; (void)id_ptr; (void)id_int;
    (void)font_sized; (void)font_legacy; (void)font_keep; (void)width; (void)wrap; (void)wrap_at;
    (void)flag; (void)tabstop; (void)repeat; (void)clip; (void)indent; (void)indent_w;
    (void)group; (void)disabled; (void)disabled_off; (void)tree; (void)tree_ptr;
    (void)handed_off; (void)anything;
}

//---- A window. Note End() runs even when Begin() returns false, so the clip is balanced either way.

void DrawWindow(const char* name, bool* p_open)
{
    if (auto win = ImGuiScoped::Window(name, p_open))
    {
        // `win` converts to false when the window is collapsed/fully clipped, so you can
        // still early-out. The destructor runs either way.
        ImGui::Text("hello");
    }
}

void DrawChild()
{
    if (ImGuiScoped::Child("left", ImVec2(0, 200), ImGuiChildFlags_Borders))
        ImGui::Text("left pane");

    if (ImGuiScoped::Child(ImGui::GetID("right"), ImVec2(0, 200)))
        ImGui::Text("right pane");
}

//---- Menus.

void DrawMainMenuBar()
{
    if (auto bar = ImGuiScoped::MainMenuBar())
    {
        if (auto file = ImGuiScoped::Menu("File"))
        {
            if (ImGui::MenuItem("Open")) { /* ... */ }
            if (auto recent = ImGuiScoped::Menu("Recent"))
                ImGui::MenuItem("project.cpp");
        }
    }
}

void DrawMenuBar()
{
    if (ImGuiScoped::MenuBar())
    {
        if (auto help = ImGuiScoped::Menu("Help"))
            ImGui::MenuItem("About");
    }
}

//---- Popups.

void DrawPopups(bool* modal_open)
{
    if (auto popup = ImGuiScoped::Popup("details"))
        ImGui::Text("popup contents");

    if (auto modal = ImGuiScoped::PopupModal("Confirm", modal_open))
    {
        if (ImGui::Button("OK")) { /* ... */ }
        if (auto ctx = ImGuiScoped::PopupContextWindow())
            ImGui::MenuItem("Item");
    }

    ImGui::Text("right-click me");
    if (auto ctx_item = ImGuiScoped::PopupContextItem())
        ImGui::MenuItem("Action");

    if (auto ctx_void = ImGuiScoped::PopupContextVoid())
        ImGui::Text("void context");
}

//---- Tooltips.

void DrawTooltips()
{
    ImGui::Button("hover me");
    if (auto tip = ImGuiScoped::Tooltip())
        ImGui::Text("an explicit tooltip");

    ImGui::Button("hover me too");
    if (auto tip = ImGuiScoped::ItemTooltip())
        ImGui::Text("only when hovered");
}

//---- Combo / ListBox.

void DrawComboAndListBox(const char* const items[], int items_count)
{
    if (auto combo = ImGuiScoped::Combo("Color", "Red"))
    {
        for (int i = 0; i < items_count; i++)
            ImGui::Selectable(items[i], i == 0);
    }

    if (auto list = ImGuiScoped::ListBox("Items", ImVec2(0, 120)))
    {
        for (int i = 0; i < items_count; i++)
            ImGui::Selectable(items[i], i == 0);
    }
}

//---- Tables.

void DrawTable()
{
    if (auto table = ImGuiScoped::Table("##inventory", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Qty");
        ImGui::TableSetupColumn("Price");
        ImGui::TableHeadersRow();

        for (int row = 0; row < 10; row++)
        {
            if (ImGuiScoped::Row())
            {
                // TableNextColumn() returns false when the column is clipped out.
                if (ImGuiScoped::NextColumn()) ImGui::Text("Item %d", row);
                if (ImGuiScoped::NextColumn()) ImGui::TextUnformatted("1");
                if (ImGuiScoped::NextColumn()) ImGui::TextUnformatted("9.99");
            }
        }
    }

    if (auto table2 = ImGuiScoped::Table("##cells", 3, 0, ImVec2(0, 120)))
    {
        if (ImGuiScoped::Column(0)) ImGui::TextUnformatted("a");
        if (ImGuiScoped::Column(1)) ImGui::TextUnformatted("b");
        if (ImGuiScoped::Column(2)) ImGui::TextUnformatted("c");
    }
}

//---- Tabs.

void DrawTabs()
{
    if (auto tabs = ImGuiScoped::TabBar("##tabs"))
    {
        // Only the selected tab's contents run, and EndTabItem() only for that one.
        if (auto t1 = ImGuiScoped::TabItem("General")) ImGui::TextUnformatted("general");
        if (auto t2 = ImGuiScoped::TabItem("Advanced")) ImGui::TextUnformatted("advanced");
    }
}

//---- Trees.

void DrawTree()
{
    if (auto node = ImGuiScoped::TreeNode("Parent"))
    {
        ImGui::TextUnformatted("contents");
        if (auto child_node = ImGuiScoped::TreeNode("Child", ImGuiTreeNodeFlags_DefaultOpen))
            ImGui::TextUnformatted("child contents");

        // NoTreePushOnOpen means Dear ImGui did not push, so we must not pop.
        if (auto header = ImGuiScoped::TreeNode("Just a header", ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Leaf))
            ImGui::TextUnformatted("unreachable-ish, but balanced");
    }
}

//---- Drag and drop.

void DrawDragDrop(const char* payload_type, const void* data, size_t size, const char** received)
{
    ImGui::Button("drag me");
    if (auto source = ImGuiScoped::DragDropSource())
    {
        ImGui::SetDragDropPayload(payload_type, data, size);
        ImGui::TextUnformatted("drag in progress");
    }

    ImGui::Button("drop here");
    if (auto target = ImGuiScoped::DragDropTarget())
    {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(payload_type))
            *received = static_cast<const char*>(p->Data);
    }
}

//---- Multi-select. ApplyRequests() must run after the scope, hence the out-param.

// ApplyRequests() is a method on the selection storage, not a free ImGui:: function.
static ImGuiID IndexToId(ImGuiSelectionBasicStorage* self, int idx)
{
    return ImGui::GetID((int*)(self->UserData) + idx);
}

void DrawMultiSelect(bool* selected, bool* selected2)
{
    bool items[2] = { false, false };
    ImGuiSelectionBasicStorage selection;
    selection.UserData = (void*)items;
    selection.AdapterIndexToStorageId = IndexToId;

    ImGuiMultiSelectIO* ms_io = nullptr;
    {
        auto ms = ImGuiScoped::MultiSelect(&ms_io, ImGuiMultiSelectFlags_NoAutoSelect, 2, 2);
        if (ms)
        {
            ImGui::Selectable("Row 0", &items[0]);
            ImGui::Selectable("Row 1", &items[1]);
            // ms->RangeSrcItem etc. are readable through operator->().
        }
    }
    // ms_io is only valid after the scope; ApplyRequests() must see the End*() copy.
    if (ms_io != nullptr)
        selection.ApplyRequests(ms_io);

    *selected = items[0];
    *selected2 = items[1];
}

//---- List clipper.

void DrawClippedList(const char* const items[], int items_count)
{
    ImGuiListClipper clipper;
    {
        auto scope = ImGuiScoped::ScopeListClipper(clipper, items_count, ImGui::GetTextLineHeightWithSpacing());
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                ImGui::TextUnformatted(items[i]);
        }
    }
}

void DrawEverything()
{
    DrawStateScopes(ImGui::GetFont(), nullptr, 0);
    DrawWindow("Example", nullptr);
    DrawChild();
    DrawMainMenuBar();
    DrawPopups(nullptr);
    DrawTooltips();
    DrawTabs();
    DrawTree();
    bool s = false, s2 = false;
    DrawMultiSelect(&s, &s2);
}

#ifdef IMGUI_RAII_USE_INTERNAL_API

// Only reachable when compiled with IMGUI_RAII_USE_INTERNAL_API; these End*() live in
// imgui_internal.h rather than imgui.h.
void DrawInternalApiWrappers()
{
    auto cols = ImGuiScoped::Columns(2, "##cols", true);
    ImGui::TextUnformatted("left");
    ImGui::NextColumn();
    ImGui::TextUnformatted("right");

    if (auto preview = ImGuiScoped::ComboPreview())
        ImGui::TextUnformatted("combo preview contents");
    else
        ImGui::TextUnformatted("closed");
}

#endif // IMGUI_RAII_USE_INTERNAL_API

} // namespace
