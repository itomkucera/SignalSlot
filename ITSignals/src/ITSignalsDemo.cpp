#include <iostream>
#include <string>
#include <vector>

#include "ITSignals.h"

struct Widget : public Disconnector
{
    Widget(const std::string& name) :
        name_{ name }
    {
        focus_in_.Connect([this]() {
            std::cout << name_  << " widget: focus received\n";
        });
    }

    Signal<> focus_in_;

protected:

    std::string name_;

};

struct ListBox : public Widget
{
    ListBox(const std::string& name) :
        Widget(name)
    {
        item_text_changed_.Connect([this](int index, const std::string& text) {
            std::cout << name_ << " listbox: item on index " << index
                << " changed its text to \"" << text << "\"\n";
        });

        selection_changed_.Connect([this](const std::vector<int>& selected_items) {
            std::cout << name_ << " listbox: selected items are:\n";
            for (const auto& i : selected_items)
            {
                std::cout << i << "\n";
            }
        });
    }

    Signal<int, std::string> item_text_changed_;

    Signal<std::vector<int>> selection_changed_;
};

int main()
{
    ListBox* listbox = new ListBox("MyListBox");
    listbox->focus_in_.Emit();
    listbox->item_text_changed_.Emit(3, "NewItem3");
    listbox->selection_changed_.Emit(std::vector<int>{ 0, 2, 99 });

    delete listbox;

    std::cin.get();
}
