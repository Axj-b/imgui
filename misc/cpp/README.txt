
imgui_stdlib.h + imgui_stdlib.cpp
  InputText() wrappers for C++ standard library (STL) type: std::string.
  This is also an example of how you may wrap your own similar types.

imgui_raii.h + imgui_raii_widgets.h
  RAII scope guards so a matching Pop()/End() cannot be forgotten. Header-only, C++17.
  Split by the semantics that actually differ:
    - imgui_raii.h         Push*()/Pop*() state. Every guard restores unconditionally.
    - imgui_raii_widgets.h Begin*()/End*() containers. Includes imgui_raii.h. Guards skip
      their End*() when Begin*() returned false, since calling End*() on that path is silent
      state corruption rather than a crash -- except for the four pairs Dear ImGui documents
      as unconditional (Begin/End, BeginChild, BeginMainMenuBar, BeginTooltip).
  Both declare namespace ImGuiScoped, so including imgui_raii_widgets.h alone is enough.
  imgui_raii_demo.cpp shows the call sites; imgui_raii_test.cpp + imgui_raii_test.bat run a
  headless stack-balance smoke test.

imgui_scoped.h
  [Experimental, not currently in main repository]
  Additional header file with some RAII-style wrappers for common Dear ImGui functions.
  Try by merging: https://github.com/ocornut/imgui/pull/2197
  Discuss at: https://github.com/ocornut/imgui/issues/2096

imgui-module:
  C++20 module binding
  https://github.com/stripe2933/imgui-module

See more C++ related extension (fmt, RAII, syntactic sugar) on Wiki:
  https://github.com/ocornut/imgui/wiki/Useful-Extensions#cness
