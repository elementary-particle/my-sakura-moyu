#ifndef COMMON_FILELIST_H
#define COMMON_FILELIST_H

#include <vector>

enum MethodType {
  METHOD_REPL = 0,
  METHOD_DIFF = 1,
  METHOD_APPEND = 2,
};

struct PatchEntry {
  char method[5];
  MethodType methodType;
  char originalName[33];
  char patchName[33];
  int offsetX, offsetY;
#ifdef FPTOOL
  FvpPackage::FileEntry *fileEntryPtr;
#else
  void *aux;
#endif
};

std::vector<PatchEntry> defaultEntryList{
    {"", METHOD_DIFF, "graph/a_bu_cp1", "a_bu_cp1", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/a_bus_s1", "a_bus_s1", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/a_cg_p3", "a_cg_p3", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/a_cg_s1", "a_cg_s1", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/a_mem_s1", "a_mem_s1", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/a_mov_s1", "a_mov_s1", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/a_snd_s1", "a_snd_s1", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/a_top_s1", "a_top_s1", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/bl_char", "bl_char", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/bl_mask", "bl_mask", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/fc_back_help", "fc_back_help", 1050, 546, nullptr},
    {"", METHOD_REPL, "graph/fc_load_help", "fc_load_help", 1050, 546, nullptr},
    {"", METHOD_REPL, "graph/fc_lock_help", "fc_lock_help", 1050, 546, nullptr},
    {"", METHOD_REPL, "graph/fc_play_help", "fc_play_help", 1050, 546, nullptr},
    {"", METHOD_REPL, "graph/fc_qload_help", "fc_qload_help", 1050, 546,
     nullptr},
    {"", METHOD_REPL, "graph/fc_qsave_help", "fc_qsave_help", 1050, 546,
     nullptr},
    {"", METHOD_REPL, "graph/fc_save_help", "fc_save_help", 1050, 546, nullptr},
    {"", METHOD_REPL, "graph/fc_skip_help", "fc_skip_help", 1050, 546, nullptr},
    {"", METHOD_REPL, "graph/fc_system_help", "fc_system_help", 1050, 546,
     nullptr},
    {"", METHOD_REPL, "graph/fc_voice_help", "fc_voice_help", 1050, 546,
     nullptr},
    {"", METHOD_REPL, "graph/fc_wincls_help", "fc_wincls_help", 1050, 546,
     nullptr},
    {"", METHOD_DIFF, "graph/menu_bg", "menu_bg", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/menu_del_mes", "menu_del_mes", 541, 309, nullptr},
    {"", METHOD_REPL, "graph/menu_exit_mes", "menu_exit_mes", 541, 309,
     nullptr},
    {"", METHOD_REPL, "graph/menu_init_mes", "menu_init_mes", 541, 309,
     nullptr},
    {"", METHOD_REPL, "graph/menu_no_1", "menu_no_1", 648, 350, nullptr},
    {"", METHOD_REPL, "graph/menu_no_2", "menu_no_2", 648, 350, nullptr},
    {"", METHOD_REPL, "graph/menu_ow_mes", "menu_ow_mes", 541, 309, nullptr},
    {"", METHOD_REPL, "graph/menu_qload_mes", "menu_qload_mes", 541, 309,
     nullptr},
    {"", METHOD_REPL, "graph/menu_title_mes", "menu_title_mes", 541, 309,
     nullptr},
    {"", METHOD_REPL, "graph/menu_undo_mes", "menu_undo_mes", 508, 309,
     nullptr},
    {"", METHOD_REPL, "graph/menu_yes_1", "menu_yes_1", 487, 350, nullptr},
    {"", METHOD_REPL, "graph/menu_yes_2", "menu_yes_2", 487, 350, nullptr},
    {"", METHOD_DIFF, "graph/opt_m_pts1", "opt_m_pts1", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/opt_m_pts2", "opt_m_pts2", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/opt_o_pts1", "opt_o_pts1", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/opt_o_pts2", "opt_o_pts2", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/opt_parts", "opt_parts", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/opt_s_pts1", "opt_s_pts1", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/opt_s_pts2", "opt_s_pts2", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/opt_sheet1", "opt_sheet1", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/opt_sheet2", "opt_sheet2", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph/opt_sheet3", "opt_sheet3", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/startmoji_t01", "startmoji_t01", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/startmoji_t02", "startmoji_t02", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/startmoji_t10", "startmoji_t10", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/startmoji_t11", "startmoji_t11", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/sys_config_M", "sys_config_M", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/sys_exit_M", "sys_exit_M", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/sys_hidewin_M", "sys_hidewin_M", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/sys_load_M", "sys_load_M", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/sys_return_M", "sys_return_M", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/sys_save_M", "sys_save_M", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/sys_title_M", "sys_title_M", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/sys_undo_M", "sys_undo_M", 0, 0, nullptr},
    {"", METHOD_REPL, "graph/sys_window_front", "sys_window_front", 0, 0,
     nullptr},
    {"", METHOD_REPL, "graph/title_logo1", "title_logo1", 0, 0, nullptr},
    {"", METHOD_APPEND, "graph/warning", "warning", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph_vis/ETC_e000b", "ETC_e000b", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph_vis/ETC_e001a", "ETC_e001a", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph_vis/ETC_e012b", "ETC_e012b", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph_vis/ETC_e012c", "ETC_e012c", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph_vis/ETC_e012d", "ETC_e012d", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph_vis/ETC_e201a", "ETC_e201a", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph_vis/ETC_e201b", "ETC_e201b", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph_vis/ETC_e201c", "ETC_e201c", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph_vis/ETC_e201d", "ETC_e201d", 0, 0, nullptr},
    {"", METHOD_DIFF, "graph_vis/ETC_e302b", "ETC_e302b", 0, 0, nullptr},
};

#endif // COMMON_FILELIST_H
