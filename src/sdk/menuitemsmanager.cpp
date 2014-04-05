/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU Lesser General Public License, version 3
 * http://www.gnu.org/licenses/lgpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#ifndef CB_PRECOMP
    #include <wx/frame.h> // GetMenuBar
#endif

#include "sdk_precomp.h"
#include "manager.h"
#include "menuitemsmanager.h"
#include <wx/regex.h>
#include <wx/tokenzr.h>

namespace
{
    wxRegEx reInsert(_T("([0-9]+):.+"));
}


MenuItemsManager::MenuItemsManager(bool autoClearOnDestroy)
    : m_AutoClearOnDestroy(autoClearOnDestroy)
{
    //ctor
}

MenuItemsManager::~MenuItemsManager()
{
    //dtor
    if (m_AutoClearOnDestroy)
    {
        Clear();
    }
}

/** @brief Add a menu item
  *
  * @param parent The menu to append the menu item to
  * @param id The menu item ID (use wxID_SEPARATOR for adding a separator)
  * @param caption The caption for the new menu item
  * @param helptext The help text for the new menu item
  * @return The new menu item or NULL for failure.
  */
wxMenuItem* MenuItemsManager::Add(wxMenu* parent, int id, const wxString& caption, const wxString& helptext)
{
    if (!parent)
        return nullptr;
    wxMenuItem* ni = new wxMenuItem(parent, id, caption, helptext);
    parent->Append(ni);

    Menu_point_item* menu_point = new Menu_point_item(ni,parent);
    m_menu_points.push_back(menu_point);

    return ni;
}

/** @brief Insert a submenu
  *
  * @param parent The menu to append the menu item to
  * @param id The menu item ID
  * @param caption The caption for the new menu item
  * @param helptext The help text for the new menu item
  * @return The new menu item or NULL for failure.
  */
wxMenuItem* MenuItemsManager::InsertSubmenu(wxMenu* parent, int id, int pos, const wxString& caption, const wxString& helptext)
{
    if (!parent)
        return nullptr;

    wxMenu* submenu = new wxMenu();

    wxMenuItem* ni = parent->Insert(pos ,id,caption,submenu,helptext);

    Menu_point_sub_menu* menu_point = new Menu_point_sub_menu(ni,parent);
    m_menu_points.push_back(menu_point);
    return ni;
}


/** @brief Insert a menu item
  *
  * @param parent The menu to insert the menu item to
  * @param index The index where to insert the menu item
  * @param id The menu item ID (use wxID_SEPARATOR for adding a separator)
  * @param caption The caption for the new menu item
  * @param helptext The help text for the new menu item
  * @return The new menu item or NULL for failure.
  */
wxMenuItem* MenuItemsManager::Insert(wxMenu* parent, int index, int id, const wxString& caption, const wxString& helptext)
{
    if (!parent)
        return nullptr;
    wxMenuItem* ni = new wxMenuItem(parent, id, caption, helptext);
    parent->Insert(index, ni);

    Menu_point_item* menu_point = new Menu_point_item(ni,parent);
    m_menu_points.push_back(menu_point);

    return ni;
} // end of Insert


/** @brief Clear all managed menu items
  */
void MenuItemsManager::Clear()
{
    menu_point_list::iterator itr = m_menu_points.end();    // clear it in the reverse order they were created
    for(;itr != m_menu_points.begin();)
    {
        --itr;
        (*itr)->Delete();
        delete (*itr);
    }
    m_menu_points.clear();

} // end of Clear

/** @brief Create menu path from string
  * @param menuPath The full menu path. This can be separated by slashes (/)
  *                 to create submenus (e.g. "MyScripts/ASubMenu/MyItem").
  *                 If the last part of the string ("MyItem" in the example)
  *                 starts with a dash (-) (e.g. "-MyItem") then a menu
  *                 separator is prepended before the actual menu item.
  * @param id The menu item ID (use wxID_SEPARATOR for adding a separator)
  * @return The id of the newly created menu or the id of the old, same menu entry or NULL for failure.
  */
int MenuItemsManager::CreateFromString(const wxString& menuPath, int id)
{
    wxMenuBar* mbar = Manager::Get()->GetAppFrame()->GetMenuBar();
    wxMenu* menu = nullptr;
    wxArrayString path = wxStringTokenize(menuPath,wxT("/"),wxTOKEN_STRTOK);

    if(path.IsEmpty())
        return id;

    for(size_t i = 0; i < path.GetCount();i++)
    {
        wxString current = path[i];
        bool insert = false;
        unsigned long insertIndex = 0;

        if(reInsert.Matches(current))
        {
            wxString indexS = reInsert.GetMatch(current, 1);
            if (indexS.ToULong(&insertIndex, 10))
            {
                current.Remove(0, indexS.Length() + 1); // +1 to remove the ":" too
                insert = true;
            }
        }
        if(i == 0)  // The menu in the menubar
        {
            // The first item is in the menubar...
            int menuPos = mbar->FindMenu(path[0]);
            if (menuPos == wxNOT_FOUND)
            {
                // New menu in menu bar
                menu = new wxMenu();
                int pos = insert ? insertIndex : mbar->GetMenuCount() - 2;
                mbar->Insert(pos, menu, current); // -2 to be inserted before "Settings"

                Menu_point_menubar* menu_point = new Menu_point_menubar(mbar,pos);
                m_menu_points.push_back(menu_point);
            }
            else
                menu = mbar->GetMenu(menuPos);
        }
        else // it is the first menu item after the menu bar
        {
            bool needsSep = current.StartsWith(_T("-"));
            if (needsSep)
                current.Remove(0, 1); // remove dash (-)

            int cur_menu_id = menu->FindItem(current);
            if(cur_menu_id == wxNOT_FOUND)
            {
                if (needsSep)
                {
                    wxMenuItem* item = new wxMenuItem(menu, wxID_SEPARATOR);
                    menu->Insert(insert ? insertIndex : menu->GetMenuItemCount(), item);
                }

                if (current.IsEmpty()) // may be now if it was just a separator (-)
                    break;

                //create menu because it does not exists...
                if(i == path.GetCount()-1)    // this is the last menu item, so insert it
                {
                    wxMenuItem* new_item = Insert(menu,insert ? insertIndex : menu->GetMenuItemCount(),id,current,wxEmptyString);
                    return new_item->GetId();
                }
                else
                {
                    // We need a new sub menu
                    wxMenu* sub_menu = new wxMenu();
                    wxMenuItem* sub_menu_item = InsertSubmenu(menu,wxNewId(),insert ? insertIndex : menu->GetMenuItemCount(),current,wxEmptyString);
                    menu = sub_menu_item->GetSubMenu();
                }
            }
            else
            {
                // The menu exists
                if(i == path.GetCount()-1)    // This is the last Menu item, and the menu exists
                {
                    return menu->FindItem(cur_menu_id)->GetId();
                }
                else
                {
                    wxMenu* sub_menu = menu->FindItem(cur_menu_id)->GetSubMenu();

                    if(sub_menu == nullptr) // A error?
                        cbMessageBox(_("Sub menu is a nullptr"),_("create menu error"));
                    else
                        menu = sub_menu;
                }
            }
        }
    }
    return 0;
} // end of CreateFromString
