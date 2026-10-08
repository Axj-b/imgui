// dear imgui: runtime smoke test for imgui_raii.h.
//
// The compile-only demo proves the wrappers instantiate. This proves they actually leave
// Dear ImGui's stacks balanced: every frame we snapshot the ID/style/clip/window stack sizes
// before and after the whole UI body, and any imbalance is a wrapper that pushed without
// popping (or vice versa).
//
// Build & run: see misc/cpp/imgui_raii_test.bat
// Runs headless against the null backend, so it needs no GPU or window.

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_null.h"
#include "misc/cpp/imgui_raii.h"         // Push*()/Pop*() state guards
#include "misc/cpp/imgui_raii_widgets.h" // Begin*()/End*() container guards

#include <stdio.h>

//---- Stack snapshot. Reading ImGuiContext internals is fine here: this is a test.

struct StackSizes
{
    int IDStack;
    int StyleVar;
    int StyleColor;
    int ItemWidth;
    int TextWrap;
    int Font;
    int ClipRect;
    int FocusScope;
    int ItemFlags;
    int WindowStack;
    int PopupStack;
    int TabBar;
    int Table;
    int Group;
    int DragDropTarget;

    static StackSizes Capture()
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* w = g.CurrentWindow;
        StackSizes s;
        s.IDStack         = w ? (int)w->IDStack.Size : 0;
        s.StyleVar        = (int)g.StyleVarStack.Size;
        s.StyleColor      = (int)g.ColorStack.Size;
        s.ItemWidth       = w ? (int)w->DC.ItemWidthStack.Size : 0;
        s.TextWrap        = w ? (int)w->DC.TextWrapPosStack.Size : 0;
        s.Font            = (int)g.FontStack.Size;
        // PushClipRect() backs onto the draw list's clip stack, not a window member.
        s.ClipRect        = w && w->DrawList ? (int)w->DrawList->_ClipRectStack.Size : 0;
        s.FocusScope      = (int)g.FocusScopeStack.Size;
        s.ItemFlags       = (int)g.ItemFlagsStack.Size;
        s.WindowStack     = (int)g.CurrentWindowStack.Size;
        s.PopupStack      = (int)g.BeginPopupStack.Size;
        s.TabBar          = (int)g.CurrentTabBarStack.Size;
        s.Table           = g.CurrentTable ? 1 : 0;
        s.Group           = (int)g.GroupStack.Size;
        s.DragDropTarget  = g.DragDropWithinTarget ? 1 : 0;
        return s;
    }
};

static bool operator==(const StackSizes& a, const StackSizes& b)
{
    return a.IDStack == b.IDStack         && a.StyleVar == b.StyleVar       && a.StyleColor == b.StyleColor
        && a.ItemWidth == b.ItemWidth     && a.TextWrap == b.TextWrap       && a.Font == b.Font
        && a.ClipRect == b.ClipRect       && a.FocusScope == b.FocusScope   && a.ItemFlags == b.ItemFlags
        && a.WindowStack == b.WindowStack && a.PopupStack == b.PopupStack   && a.TabBar == b.TabBar
        && a.Table == b.Table             && a.Group == b.Group             && a.DragDropTarget == b.DragDropTarget;
}

static int PrintDiff(const char* name, const StackSizes& a, const StackSizes& b)
{
    const struct { const char* Name; int A; int B; } fields[] = {
        { "IDStack", a.IDStack, b.IDStack }, { "StyleVar", a.StyleVar, b.StyleVar },
        { "StyleColor", a.StyleColor, b.StyleColor }, { "ItemWidth", a.ItemWidth, b.ItemWidth },
        { "TextWrap", a.TextWrap, b.TextWrap }, { "Font", a.Font, b.Font },
        { "ClipRect", a.ClipRect, b.ClipRect }, { "FocusScope", a.FocusScope, b.FocusScope },
        { "ItemFlags", a.ItemFlags, b.ItemFlags }, { "WindowStack", a.WindowStack, b.WindowStack },
        { "PopupStack", a.PopupStack, b.PopupStack }, { "TabBar", a.TabBar, b.TabBar },
        { "Table", a.Table, b.Table }, { "Group", a.Group, b.Group },
        { "DragDropTarget", a.DragDropTarget, b.DragDropTarget },
    };
    int bad = 0;
    for (const auto& f : fields)
        if (f.A != f.B) { printf("    %-16s %d -> %d\n", f.Name, f.A, f.B); bad++; }
    (void)name;
    return bad;
}

static int g_failures = 0;

// Same check, but repeated over several frames. Needed for anything that depends on persisted
// window state (e.g. collapsing a window requires it to already exist).
template <typename Fn>
static void ScopedCheckFrames(const char* label, int frames, Fn&& body)
{
    for (int i = 0; i < frames; i++)
    {
        ImGui_ImplNull_NewFrame();
        ImGui::NewFrame();
        const StackSizes before = StackSizes::Capture();
        body();
        const StackSizes after = StackSizes::Capture();
        ImGui::EndFrame();
        if (!(before == after))
        {
            printf("  FAIL %s (frame %d)\n", label, i);
            PrintDiff(label, before, after);
            g_failures++;
        }
    }
}

// Each ScopedCheck body must leave every stack exactly as it found it.
// Note the "after" snapshot is taken *before* EndFrame(): EndFrame() tears down the implicit
// fallback window and pops the base font/focus-scope/clip state, which would mask a real leak.
#define SCOPED_CHECK(label, body)                                          \
    do {                                                                    \
        ImGui_ImplNull_NewFrame();                                          \
        ImGui::NewFrame();                                                  \
        const StackSizes before = StackSizes::Capture();                    \
        { body }                                                            \
        const StackSizes after = StackSizes::Capture();                     \
        ImGui::EndFrame();                                                  \
        if (!(before == after)) {                                           \
            printf("  FAIL %s\n", label);                                   \
            PrintDiff(label, before, after);                                \
            g_failures++;                                                   \
        }                                                                   \
    } while (0)

//----------------------------------------------------------------------------

static void TestStateScopes()
{
    printf("State scopes\n");

    SCOPED_CHECK("StyleColor / StyleColors", {
        { auto a = ImGuiScoped::StyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
          auto b = ImGuiScoped::StyleColor(ImGuiCol_Text, IM_COL32_WHITE);
          auto c = ImGuiScoped::StyleColors({ { ImGuiCol_Text, ImVec4(0,1,0,1) }, { ImGuiCol_Button, IM_COL32(0,0,255,255) } }); }
    });

    SCOPED_CHECK("StyleVar / StyleVars", {
        { auto a = ImGuiScoped::StyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
          auto b = ImGuiScoped::StyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
          auto c = ImGuiScoped::StyleVarX(ImGuiStyleVar_FramePadding, 4.0f);
          auto d = ImGuiScoped::StyleVarY(ImGuiStyleVar_FramePadding, 4.0f);
          auto e = ImGuiScoped::StyleVars({ { ImGuiStyleVar_FrameRounding, 0.0f }, { ImGuiStyleVar_ItemSpacing, ImVec2(2,2) } }); }
    });

    SCOPED_CHECK("ID overloads", {
        int idx = 3;
        int obj = 0;
        // begin/end must be inside one contiguous buffer (see imgui_raii.h warning).
        static const char kBuf[] = "settings";
        { auto a = ImGuiScoped::ID("settings");
          auto b = ImGuiScoped::ID(kBuf, kBuf + 4);
          auto c = ImGuiScoped::ID(&obj);
          auto d = ImGuiScoped::ID(idx);
          // A pushed ID must actually change what an unlabeled widget resolves to.
          IM_ASSERT(ImGui::GetID("widget") != 0); }
    });

    SCOPED_CHECK("Font", {
        ImFont* f = ImGui::GetFont();
        { auto a = ImGuiScoped::Font(f, 20.0f);
          auto b = ImGuiScoped::Font(f);
          auto c = ImGuiScoped::Font(nullptr, 0.0f); }
    });

    SCOPED_CHECK("ItemWidth / TextWrapPos", {
        { auto a = ImGuiScoped::ItemWidth(200.0f);
          auto b = ImGuiScoped::TextWrapPos();
          auto c = ImGuiScoped::TextWrapPos(100.0f); }
    });

    SCOPED_CHECK("ItemFlag family", {
        { auto a = ImGuiScoped::ItemFlag(ImGuiItemFlags_ButtonRepeat, true);
          auto b = ImGuiScoped::TabStop();
          auto c = ImGuiScoped::ButtonRepeat(false); }
    });

    SCOPED_CHECK("ClipRect / Indent / Group / Disabled", {
        { auto a = ImGuiScoped::ClipRect(ImVec2(0, 0), ImVec2(10, 10), true);
          auto b = ImGuiScoped::Indent();
          auto c = ImGuiScoped::Indent(8.0f);
          auto d = ImGuiScoped::Group();
          auto e = ImGuiScoped::Disabled();
          auto f = ImGuiScoped::Disabled(false); }
    });

    SCOPED_CHECK("TreePush", {
        int obj = 0;
        { auto a = ImGuiScoped::TreePush("branch");
          auto b = ImGuiScoped::TreePush(&obj); }
    });

    SCOPED_CHECK("OnScopeExit", {
        { auto a = ImGuiScoped::OnScopeExit([] { ImGui::PopStyleColor(2); });
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,0,0,1));
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,0,0,1)); }
    });

    SCOPED_CHECK("dismiss() hands the pop to a callee", {
        auto a = ImGuiScoped::StyleColor(ImGuiCol_Text, ImVec4(1,0,0,1));
        a.dismiss();                 // deliberately unbalanced on purpose
        ImGui::PopStyleColor(1);     // ... so pop it by hand to stay balanced
    });

    SCOPED_CHECK("early return still pops", {
        struct Helper
        {
            static void Bails()
            {
                auto a = ImGuiScoped::StyleColor(ImGuiCol_Text, ImVec4(1,0,0,1));
                auto b = ImGuiScoped::ItemWidth(120.0f);
                (void)a; (void)b;
                if (true) return;      // both destructors must still run
            }
        };
        Helper::Bails();
    });

    SCOPED_CHECK("nested scopes unwind in order", {
        for (int i = 0; i < 4; i++)
        {
            auto outer = ImGuiScoped::StyleColor(ImGuiCol_Text, ImVec4(1,0,0,1));
            auto inner = ImGuiScoped::ItemWidth(100.0f + (float)i);
            if (i % 2 == 0) continue;  // exits both scopes each iteration
        }
    });
}

static void TestWindowAndChild()
{
    printf("Windows and children\n");

    SCOPED_CHECK("Window (End is unconditional)", {
        bool open = true;
        { auto w = ImGuiScoped::Window("W", &open);
          ImGui::TextUnformatted("x");
          ImGui::TextUnformatted("y"); }
    });

    // Force Begin() to return false by collapsing the window, then verify End() still ran and
    // the window stack did not leak. Run several frames: collapsing needs the window to already
    // exist, so frame 0 legitimately reports visible=true.
    ScopedCheckFrames("Window returning false (collapsed)", 4, [] {
        ImGui::SetNextWindowCollapsed(true, ImGuiCond_Always);
        auto w = ImGuiScoped::Window("Collapsed");
        if (!w)
            ImGui::TextUnformatted("unreachable: window is collapsed");
    });

    SCOPED_CHECK("nested windows", {
        bool open = true;
        { auto w = ImGuiScoped::Window("Outer", &open);
          { auto c = ImGuiScoped::Child("Inner", ImVec2(0, 50)); ImGui::TextUnformatted("i"); } }
    });

    // NOTE: BeginChild(ImGuiID) must be called inside a parent window. At the root of the
    // window stack it trips an upstream assert in this 1.93.0 WIP docking branch
    // (g.Windows.Size == g.WindowsTempSortBuffer.Size, imgui.cpp:6430) with raw ImGui too.
    SCOPED_CHECK("Child by ImGuiID", {
        bool open = true;
        if (auto w = ImGuiScoped::Window("ChildIDHost", &open))
        {
            ImGuiID id = ImGui::GetID("child_by_id");
            { auto c = ImGuiScoped::Child(id, ImVec2(0, 40)); ImGui::TextUnformatted("x"); }
        }
    });

    SCOPED_CHECK("Child inside a Table cell", {
        bool open = true;
        { auto w = ImGuiScoped::Window("TableHost", &open);
          if (auto t = ImGuiScoped::Table("##t", 2))
          {
              if (ImGuiScoped::Row())
              {
                  if (ImGuiScoped::NextColumn())
                  { auto c = ImGuiScoped::Child("cell_child", ImVec2(0, 30)); ImGui::TextUnformatted("c"); }
              }
          } }
    });
}

static void TestMenusAndPopups()
{
    printf("Menus and popups\n");

    SCOPED_CHECK("MainMenuBar", {
        if (auto bar = ImGuiScoped::MainMenuBar())
        {
            if (auto m = ImGuiScoped::Menu("File"))
            {
                if (auto sub = ImGuiScoped::Menu("Recent"))
                    ImGui::MenuItem("thing.cpp");
                ImGui::MenuItem("Open");
            }
        }
    });

    SCOPED_CHECK("closed submenu does not pop the parent", {
        // The dangerous case: a sub-menu that returns false while the current window is the
        // parent menu. EndMenu() would close the parent if it were called anyway.
        if (auto bar = ImGuiScoped::MainMenuBar())
        {
            if (auto m = ImGuiScoped::Menu("File"))
            {
                if (auto sub = ImGuiScoped::Menu("NotOpen"))  // closed -> false
                    ImGui::MenuItem("never");
                // parent must still be the current window here
                ImGui::MenuItem("Open");
            }
        }
    });

    SCOPED_CHECK("Popup not open", {
        bool open = true;
        if (auto w = ImGuiScoped::Window("PopupHost", &open))
        {
            // Never opened -> false -> EndPopup() must not run.
            { auto p = ImGuiScoped::Popup("NeverOpened"); }
        }
    });

    SCOPED_CHECK("Popup open", {
        bool open = true;
        if (auto w = ImGuiScoped::Window("PopupHost2", &open))
        {
            ImGui::OpenPopup("RealPopup");
            if (auto p = ImGuiScoped::Popup("RealPopup"))
            {
                ImGui::TextUnformatted("inside popup");
                if (auto inner = ImGuiScoped::Popup("NestedRealPopup"))  // closed
                    ImGui::TextUnformatted("unreachable");
                // a closed nested popup must not pop this one
                ImGui::TextUnformatted("still inside popup");
            }
        }
    });

    SCOPED_CHECK("PopupContextVoid when not clicked", {
        if (auto p = ImGuiScoped::PopupContextVoid())
            ImGui::TextUnformatted("should not appear");
    });
}

static void TestTooltips()
{
    printf("Tooltips\n");

    SCOPED_CHECK("Tooltip", {
        if (auto t = ImGuiScoped::Tooltip())
            ImGui::TextUnformatted("tooltip");
    });

    SCOPED_CHECK("ItemTooltip with nothing hovered", {
        ImGui::TextUnformatted("not hovered");
        if (auto t = ImGuiScoped::ItemTooltip())
            ImGui::TextUnformatted("should not appear");
    });
}

static void TestComboListTabTree()
{
    printf("Combo / ListBox / Tab / Tree\n");

    SCOPED_CHECK("Combo closed", {
        bool open = true;
        if (auto w = ImGuiScoped::Window("ComboHost", &open))
        {
            if (auto c = ImGuiScoped::Combo("Pick", "none"))
            {
                ImGui::Selectable("a", false);
                ImGui::Selectable("b", false);
            }
            // g.BeginComboDepth must be back to what it was; a stray EndCombo would corrupt it.
        }
    });

    SCOPED_CHECK("ListBox", {
        bool open = true;
        if (auto w = ImGuiScoped::Window("ListBoxHost", &open))
        {
            if (auto l = ImGuiScoped::ListBox("Items", ImVec2(0, 100)))
            {
                ImGui::Selectable("a", false);
                ImGui::Selectable("b", false);
            }
        }
    });

    SCOPED_CHECK("TabBar with non-selected tabs", {
        bool open = true;
        if (auto w = ImGuiScoped::Window("TabHost", &open))
        {
            if (auto tb = ImGuiScoped::TabBar("##tabs"))
            {
                // Only one tab returns true; the others must not reach EndTabItem().
                if (auto t0 = ImGuiScoped::TabItem("Zero"))   ImGui::TextUnformatted("0");
                if (auto t1 = ImGuiScoped::TabItem("One"))    ImGui::TextUnformatted("1");
                if (auto t2 = ImGuiScoped::TabItem("Two"))    ImGui::TextUnformatted("2");
            }
            // If EndTabItem() had run for the non-selected tabs it would have popped an ID here.
            ImGui::TextUnformatted("after tab bar");
        }
    });

    SCOPED_CHECK("Table rows/columns", {
        bool open = true;
        if (auto w = ImGuiScoped::Window("TableHost", &open))
        {
            if (auto t = ImGuiScoped::Table("##grid", 4, ImGuiTableFlags_Borders))
            {
                ImGui::TableSetupColumn("A"); ImGui::TableSetupColumn("B");
                ImGui::TableSetupColumn("C"); ImGui::TableSetupColumn("D");
                ImGui::TableHeadersRow();
                for (int r = 0; r < 5; r++)
                {
                    if (ImGuiScoped::Row())
                    {
                        if (ImGuiScoped::NextColumn()) ImGui::Text("r%d c0", r);
                        if (ImGuiScoped::NextColumn()) ImGui::Text("r%d c1", r);
                        if (ImGuiScoped::Column(2))   ImGui::Text("r%d c2", r);
                        if (ImGuiScoped::Column(3))   ImGui::Text("r%d c3", r);
                    }
                }
            }
        }
    });

    SCOPED_CHECK("Tree nodes", {
        bool open = true;
        if (auto w = ImGuiScoped::Window("TreeHost", &open))
        {
            if (auto n0 = ImGuiScoped::TreeNode("Parent"))
            {
                if (auto n1 = ImGuiScoped::TreeNode("Child"))
                    ImGui::TextUnformatted("deep");
                // NoTreePushOnOpen: Dear ImGui did not push, so we must not pop.
                if (auto n2 = ImGuiScoped::TreeNode("Header", ImGuiTreeNodeFlags_NoTreePushOnOpen))
                    ImGui::TextUnformatted("n/a");
                ImGui::TextUnformatted("still in parent");
            }
        }
    });
}

static void TestClipperAndDragDrop()
{
    printf("Clipper and drag-drop\n");

    SCOPED_CHECK("List clipper scope", {
        ImGuiListClipper clipper;
        {
            auto scope = ImGuiScoped::ScopeListClipper(clipper, 200, ImGui::GetTextLineHeightWithSpacing());
            while (clipper.Step())
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                    ImGui::TextUnformatted("row");
        }
    });

    SCOPED_CHECK("List clipper dismissed early", {
        ImGuiListClipper clipper;
        {
            auto scope = ImGuiScoped::ScopeListClipper(clipper, 200);
            scope.dismiss();                       // clipper's own dtor will End() later
        }
        clipper.End();
    });

    SCOPED_CHECK("DragDropSource/Target not active", {
        ImGui::Button("source");
        if (auto s = ImGuiScoped::DragDropSource()) { /* never enters here */ }

        ImGui::Button("target");
        if (auto t = ImGuiScoped::DragDropTarget()) { /* never enters here */ }
    });
}

static ImGuiID TestIndexToId(ImGuiSelectionBasicStorage* self, int idx)
{
    return ImGui::GetID((int*)(self->UserData) + idx);
}

static void TestMultiSelect()
{
    printf("Multi-select\n");

    static bool s_items[3] = { false, false, false };
    ImGuiSelectionBasicStorage selection;
    selection.UserData = (void*)s_items;
    selection.AdapterIndexToStorageId = TestIndexToId;
    selection.Clear();

    SCOPED_CHECK("MultiSelect + ApplyRequests after scope", {
        bool open = true;
        if (auto w = ImGuiScoped::Window("MsHost", &open))
        {
            ImGuiMultiSelectIO* ms_io = nullptr;
            {
                auto ms = ImGuiScoped::MultiSelect(&ms_io, ImGuiMultiSelectFlags_NoAutoSelect, 3, 3);
                if (ms)
                {
                    ImGui::Selectable("Row 0", &s_items[0]);
                    ImGui::Selectable("Row 1", &s_items[1]);
                    ImGui::Selectable("Row 2", &s_items[2]);
                }
            }
            if (ms_io != nullptr)
                selection.ApplyRequests(ms_io);
        }
    });

    SCOPED_CHECK("MultiSelect ended early", {
        bool open = true;
        if (auto w = ImGuiScoped::Window("MsHost2", &open))
        {
            ImGuiMultiSelectIO* ms_io = nullptr;
            {
                auto ms = ImGuiScoped::MultiSelect(&ms_io, ImGuiMultiSelectFlags_NoAutoSelect, 3, 3);
                ms.End();                          // early, and must be idempotent
                ms.End();                          // no-op
            }
            if (ms_io != nullptr)
                selection.ApplyRequests(ms_io);
        }
    });
}

static void TestDeepNesting()
{
    printf("Deep nesting and mixing\n");

    SCOPED_CHECK("everything nested at once", {
        bool open = true;
        { auto w = ImGuiScoped::Window("KitchenSink", &open);
          { auto disabled = ImGuiScoped::Disabled();
            { auto width = ImGuiScoped::ItemWidth(180.0f);
              { auto color = ImGuiScoped::StyleColor(ImGuiCol_Text, ImVec4(1,1,0,1));
                { auto indent = ImGuiScoped::Indent();
                  { auto id = ImGuiScoped::ID("deep");
                    { auto group = ImGuiScoped::Group();
                      { auto node = ImGuiScoped::TreeNode("Node");
                        // TabItem needs an enclosing TabBar; the guard skips EndTabItem()
                        // when BeginTabItem() returns false, which here it does.
                        if (auto tabs = ImGuiScoped::TabBar("##deep"))
                        {
                            if (auto tab = ImGuiScoped::TabItem("InnerTab"))
                                ImGui::TextUnformatted("deepest");
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
    });

    SCOPED_CHECK("1000 sequential scopes do not drift", {
        for (int i = 0; i < 1000; i++)
        {
            auto a = ImGuiScoped::StyleColor(ImGuiCol_Text, ImVec4(1,0,0,1));
            auto b = ImGuiScoped::ItemWidth(50.0f + (float)i);
            auto c = ImGuiScoped::ID(i);
            auto d = ImGuiScoped::Indent((float)(i % 3));
        }
    });
}

int main()
{
    // Unbuffered so a crash mid-test still shows which checks already passed.
    setvbuf(stdout, NULL, _IONBF, 0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplNull_Init())
    {
        printf("Failed to init null backend\n");
        return 1;
    }

    // The null backend draws nothing, so give it a size to lay out against.
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime = 1.0f / 60.0f;

    printf("imgui_raii.h + imgui_raii_widgets.h runtime smoke test (imgui %s)\n\n", IMGUI_VERSION);

    TestStateScopes();
    TestWindowAndChild();
    TestMenusAndPopups();
    TestTooltips();
    TestComboListTabTree();
    TestClipperAndDragDrop();
    TestMultiSelect();
    TestDeepNesting();

    ImGui_ImplNull_Shutdown();
    ImGui::DestroyContext();

    printf("\n%s (%d failing frame(s))\n", g_failures == 0 ? "PASS" : "FAIL", g_failures);
    return g_failures == 0 ? 0 : 1;
}
