#include <iostream>
#include <string>
#include <vector>

// signals header
#include "ITOMSignal.h"

/*
* Dummy struct extending the Disconnector class - see the ITSignals.h
*/
struct Widget : public itom::Disconnector
{
    Widget(const std::string& name) :
        name_{ name }
    {
        // notify when the widget gains a focus
        focus_in_.Connect([this]() {
            std::cout << name_  << " widget: focus received\n";
        });
    }

    std::string GetName()
    {
        return name_;
    }

    // simple focus signal without a param
    itom::Signal<> focus_in_;

protected:

    std::string name_;

};

/*
* Another dummy class higher in the hierarchy
*/
struct ListBox : public Widget
{
    ListBox(const std::string& name) :
        Widget(name)
    {
        // notify what item changed its text and to what value
        item_text_changed_.Connect([this](int index, const std::string& text) {
            std::cout << name_ << " listbox: item on index " << index
                << " changed its text to \"" << text << "\"\n";
        });

        // notify what items are now selected in the multi-select listbox
        selection_changed_.Connect([this](const std::vector<int>& selected_items) {
            std::cout << name_ << " listbox: selected items are:\n";
            for (const auto& i : selected_items)
            {
                std::cout << i << "\n";
            }
        });
    }

    // signal for an item's text change - index and a new text of this item
    itom::Signal<int, std::string> item_text_changed_;

    // signal for a selection change - index vector of the selected items
    itom::Signal<std::vector<int>> selection_changed_;
};

int main()
{
    // create some dummy objects
    ListBox* listbox = new ListBox("MyListBox");
    Widget* w = new Widget("SomeWidget");

    // test the signal disconnection upon w's destruction
    listbox->focus_in_.Connect([w]() {
        std::cout << "ERROR in widget" << w->GetName() << "\n"; },
        w // w's dtor makes this slot to be disconnected
    );

    // itom::Disconnector's virtual dtor terminates the above created connection
    delete w;

    size_t discon_slot_id = listbox->focus_in_.Connect([]() {
        std::cout << "don't print this\n";
    });
    
    // explicitly terminate a connection
    listbox->focus_in_.Disconnect(discon_slot_id);

    // multiple connections to the same signal
    listbox->focus_in_.Connect([]() {
        std::cout << "multiple slot test passed\n";
    });

    // this should execute 2 slots - 4 were connected
    // 1 was disconnected automatically (by Disconnector's dtor)
    // 1 was disconnected explicitly using Signal::Disconnect()
    listbox->focus_in_.Emit(); 

    // test the multi-param signal
    listbox->item_text_changed_.Emit(3, "NewItem3");

    // test the container-param signal
    listbox->selection_changed_.Emit(std::vector<int>{ 0, 2, 99 });

    delete listbox;

    std::cin.get();
}
